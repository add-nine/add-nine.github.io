#include "delaunay.hpp"
#include "dsatur.hpp"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <ranges>
#include <vector>
#include <cmath>
#include <algorithm>

EM_JS(int, js_canvas_css_w, (), {
    const c = document.getElementById('canvas');
    return Math.round(c.getBoundingClientRect().width);
});

EM_JS(int, js_canvas_css_h, (), {
    const c = document.getElementById('canvas');
    return Math.round(c.getBoundingClientRect().height);
});

EM_JS(void, js_sync_canvas_resolution, (), {
    const c = document.getElementById('canvas');
    const cssW = Math.round(c.getBoundingClientRect().width);
    const cssH = Math.round(c.getBoundingClientRect().height);
    const dpr  = window.devicePixelRatio || 1;
    c.width  = Math.max(1, Math.round(cssW * dpr));
    c.height = Math.max(1, Math.round(cssH * dpr));
    const ctx = c.getContext('2d');
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
});

EM_JS(void, js_clear, (), {
    const c = document.getElementById('canvas');
    const ctx = c.getContext('2d');
    const cssW = Math.round(c.getBoundingClientRect().width);
    const cssH = Math.round(c.getBoundingClientRect().height);
    ctx.fillStyle = '#111';
    ctx.fillRect(0, 0, cssW, cssH);
});

EM_JS(void, js_setup_controls, (), {
    const call = (m) => Module.ccall('set_color_mode', null, ['number'], [m]);
    const p = document.getElementById('p-btn');
    const e = document.getElementById('e-btn');
    const f = document.getElementById('f-btn');
    const clr = document.getElementById('clear-btn');
    if (p) p.onclick = () => call(0);
    if (e) e.onclick = () => call(1);
    if (f) f.onclick = () => call(2);
    if (clr) clr.onclick = () => call(-1);
});

EM_JS(void, js_set_dump, (const char* txt), {
    const el = document.getElementById('dump');
    if (el) el.value = UTF8ToString(txt);
});

EM_JS(void, js_set_stats, (const char* txt), {
    const el = document.getElementById('stats');
    if (el) el.innerHTML = UTF8ToString(txt);
});

EM_JS(void, js_setup_copy, (), {
    const btn = document.getElementById('copy-btn');
    const el  = document.getElementById('dump');
    if (btn && el) {
        btn.onclick = async () => {
            try {
                await navigator.clipboard.writeText(el.value);
                btn.textContent = "コピー済み";
                setTimeout(() => (btn.textContent = "コピー"), 1000);
            } catch(e) {
                el.focus(); el.select();
                document.execCommand('copy');
            }
        };
    }
});

EM_JS(void, js_set_stroke, (const char* color, float width), {
    const ctx = document.getElementById('canvas').getContext('2d');
    ctx.strokeStyle = UTF8ToString(color);
    ctx.lineWidth = width;
    ctx.lineJoin = 'round';
    ctx.lineCap = 'round';
});

EM_JS(void, js_draw_line, (double x1, double y1, double x2, double y2), {
    const ctx = document.getElementById('canvas').getContext('2d');
    ctx.beginPath();
    ctx.moveTo(x1, y1);
    ctx.lineTo(x2, y2);
    ctx.stroke();
});

EM_JS(void, js_set_fill, (const char* color), {
    const ctx = document.getElementById('canvas').getContext('2d');
    ctx.fillStyle = UTF8ToString(color);
});

EM_JS(void, js_draw_point, (double x, double y, double r), {
    const ctx = document.getElementById('canvas').getContext('2d');
    ctx.beginPath();
    ctx.arc(x, y, r, 0, 2*Math.PI);
    ctx.fill();
});

EM_JS(void, js_fill_triangle, (double x1, double y1, double x2, double y2, double x3, double y3), {
    const ctx = document.getElementById('canvas').getContext('2d');
    ctx.beginPath();
    ctx.moveTo(x1, y1);
    ctx.lineTo(x2, y2);
    ctx.lineTo(x3, y3);
    ctx.closePath();
    ctx.fill();
});

enum class ColorMode {
    p = 0,
    e = 1,
    f = 2
};
static int color_mode = -1;
extern "C" {
    EMSCRIPTEN_KEEPALIVE void set_color_mode(int m) {color_mode = m;}
    EMSCRIPTEN_KEEPALIVE void sync_canvas() {js_sync_canvas_resolution();}
}

static int maxdeg;
static int maxclr;
static std::string color_box;

class Renderer {
public:
    void draw() {
        js_clear();
        color_box.clear();

        const auto& points = mesh.getPoints();
        const auto& edges = mesh.getEdges();
        const auto& triangles = mesh.getTriangles();

        maxdeg = [&]{
            std::vector<int> deg(points.size(), 0);
            for (const auto& [u, v] : edges) {
                deg[u]++;
                deg[v]++;
            }
            return *std::max_element(deg.begin(), deg.end());
        }();

        const auto rgb_string = [](int r, int g, int b) {
            auto R = std::to_string(r);
            auto G = std::to_string(g);
            auto B = std::to_string(b);
            return "rgb("+R+", "+G+", "+B+")";
        };
        const auto rgba_string = [](int r, int g, int b, double a) {
            auto R = std::to_string(r);
            auto G = std::to_string(g);
            auto B = std::to_string(b);
            auto A = std::to_string(a);
            return "rgba("+R+", "+G+", "+B+", "+A+")";
        };
        auto add_color_box = [&](const std::string& s) {
            color_box += "<span class='color-box' style='background-color:" + s + "'></span>";
        };
        const auto draw_edge = [&](const int colored) {
            if (colored) {
                const auto e_color = dsatur::color_edge(points, edges);
                const auto rgb = dsatur::rgb_color(e_color);
                maxclr = rgb.size();
                for (const auto& [r, g, b] : rgb) {
                    add_color_box(rgb_string(r, g, b));
                }
                for (const auto& [e, c] : std::views::zip(edges, e_color)) {
                    const auto& [u, v] = e;
                    const auto& [r, g, b] = rgb[c];
                    auto [ux, uy] = canvas_xy(points[u].x, points[u].y);
                    auto [vx, vy] = canvas_xy(points[v].x, points[v].y);
                    js_set_stroke(rgb_string(r, g, b).c_str(), LINE_WIDTH);
                    js_draw_line(ux, uy, vx, vy);
                }
            }
            else {
                js_set_stroke("rgb(255,255,255)", LINE_WIDTH);
                for (const auto& [u, v] : edges) {
                    auto [ux, uy] = canvas_xy(points[u].x, points[u].y);
                    auto [vx, vy] = canvas_xy(points[v].x, points[v].y);
                    js_draw_line(ux, uy, vx, vy);
                }
            }
        };
        const auto draw_point = [&](const int colored) {
            if (colored) {
                const auto p_color = dsatur::color_point(points, edges);
                const auto rgb = dsatur::rgb_color(p_color);
                for (const auto& [r, g, b] : rgb) {
                    add_color_box(rgb_string(r, g, b));
                }
                maxclr = rgb.size();
                for (const auto& [p, c] : std::views::zip(points, p_color)) {
                    const auto& [r, g, b] = rgb[c];
                    const auto& [x, y] = canvas_xy(p.x, p.y);
                    js_set_fill(rgb_string(r, g, b).c_str());
                    js_draw_point(x, y, POINT_RADIUS);
                }
            }
            else {
                js_set_fill("rgb(255,255,255)");
                for (const auto& p : points) {
                    auto [x, y] = canvas_xy(p.x, p.y);
                    js_draw_point(x, y, POINT_RADIUS);
                }
            }
        };
        if ((ColorMode)color_mode == ColorMode::f) {
            const auto t_color = dsatur::color_face(points, triangles);
            const auto rgb = dsatur::rgb_color(t_color);
            maxclr = rgb.size();
            for (const auto& [r, g, b] : rgb) {
                add_color_box(rgba_string(r, g, b, 0.3));
            }
            for (const auto& [tri, c] : std::views::zip(triangles, t_color)) {
                const auto& [r, g, b] = rgb[c];
                auto [x1, y1] = canvas_xy(points[tri.p[0]].x, points[tri.p[0]].y);
                auto [x2, y2] = canvas_xy(points[tri.p[1]].x, points[tri.p[1]].y);
                auto [x3, y3] = canvas_xy(points[tri.p[2]].x, points[tri.p[2]].y);
                js_set_fill(rgba_string(r, g, b, 0.3).c_str());
                js_fill_triangle(x1, y1, x2, y2, x3, y3);
            }
        }
        draw_edge(((ColorMode)color_mode == ColorMode::e ? 1 : 0));
        draw_point(((ColorMode)color_mode == ColorMode::p ? 1 : 0));
        if (color_mode == -1) maxclr = 0;

        update_stats();
    }
    void onClick(int px, int py) {
        auto [x, y] = world_xy(px, py);
        mesh.addPoint(x, y);
        mesh.build();
        update_info();
    }
private:
    delaunay::DelaunayTriangulation mesh;
    const float LINE_WIDTH = 2.0f;
    const float POINT_RADIUS = 6.0f;
    std::pair<double, double> canvas_xy(double x, double y) {
        const double W = js_canvas_css_w();
        const double H = js_canvas_css_h();
        return std::make_pair(x * W, (1.0 - y) * H);
    }
    std::pair<double, double> world_xy(double x, double y) {
        const double W = js_canvas_css_w();
        const double H = js_canvas_css_h();
        return std::make_pair(x / W, 1.0 - y / H);
    }
    void update_info() {
        const auto& points = mesh.getPoints();
        const int N = points.size();
        const auto& edges  = mesh.getEdges();
        const int M = edges.size();
        std::string s;
        s += std::to_string(N);
        s += ' ';
        s += std::to_string(M);
        s += '\n';
        for (const auto& [u, v] : edges) {
            s += std::to_string(u);
            s += ' ';
            s += std::to_string(v);
            s += '\n';
        }
        js_set_dump(s.c_str());
    }
    void update_stats() {
        std::string info = "最大次数: " + std::to_string(maxdeg) + "<br>使用色数: " + std::to_string(maxclr) + " " + color_box;
        js_set_stats(info.c_str());
    }
};

static Renderer G;

static EM_BOOL on_mouse(int, const EmscriptenMouseEvent* e, void*) {
    G.onClick(e->targetX, e->targetY);
    return EM_TRUE;
}

static void main_loop() {G.draw();}

int main() {
    js_sync_canvas_resolution();
    js_setup_controls();
    js_setup_copy();
    EM_ASM({
        window.addEventListener('resize', () => {
            Module.ccall('sync_canvas', null, [], []);
        });
    });
    emscripten_set_mousedown_callback("#canvas", nullptr, 0, on_mouse);
    emscripten_set_main_loop(main_loop, 0, 1);
}