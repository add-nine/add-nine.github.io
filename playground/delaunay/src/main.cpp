#include "delaunay.hpp"
#include "dsatur.hpp"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <ranges>
#include <vector>
#include <cmath>
#include <algorithm>

static constexpr int CSS_SIZE = 800;

EM_JS(void, js_setup_canvas, (int css_size), {
    const c = document.getElementById('canvas');
    if (!c) {
        const nc = document.createElement('canvas');
        nc.id = 'canvas';
        document.body.style.margin = '0';
        document.body.style.background = '#111';
        document.body.style.display = 'grid';
        document.body.style.placeItems = 'center';
        document.body.appendChild(nc);
    }
    const canvas = document.getElementById('canvas');
    canvas.style.width = css_size + 'px';
    canvas.style.height = css_size + 'px';
    const dpr = window.devicePixelRatio || 1;
    canvas.width = Math.round(css_size * dpr);
    canvas.height = Math.round(css_size * dpr);
    const ctx = canvas.getContext('2d');
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.fillStyle = '#111';
    ctx.fillRect(0, 0, css_size, css_size);
});

EM_JS(void, js_clear, (int css_size), {
    const ctx = document.getElementById('canvas').getContext('2d');
    ctx.fillStyle = '#111';
    ctx.fillRect(0, 0, css_size, css_size);
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

class Renderer {
public:
    void draw() {
        js_clear(CSS_SIZE);

        const auto& points = mesh.getPoints();
        const auto& edges = mesh.getEdges();
        const auto colors = dsatur::colorize(points, edges);
        const auto rgb = dsatur::rgb_color(colors);

        const auto rgb_string = [](int r, int g, int b) {
            auto R = std::to_string(r);
            auto G = std::to_string(g);
            auto B = std::to_string(b);
            return "rgb("+R+", "+G+", "+B+")";
        };

        // 辺描画
        js_set_stroke("rgb(255, 255, 255)", LINE_WIDTH);
        for (const auto& [u, v] : edges) {
            auto [ux, uy] = canvas_xy(points[u].x, points[u].y);
            auto [vx, vy] = canvas_xy(points[v].x, points[v].y);
            js_draw_line(ux, uy, vx, vy);
        }

        // 点描画
        for (auto [p, c] : std::views::zip(points, colors)) {
            auto [x, y] = canvas_xy(p.x, p.y);
            auto [r, g, b] = rgb[c];
            js_set_fill("rgb(255, 255, 255)");
            js_draw_point(x, y, POINT_RADIUS);
        }
    }
    void onClick(int px, int py) {
        auto [x, y] = world_xy(px, py);
        mesh.addPoint(x, y);
        mesh.build();
    }
private:
    delaunay::DelaunayTriangulation mesh;
    const float LINE_WIDTH = 2.0f;
    const float POINT_RADIUS = 6.0f;
    std::pair<double, double> canvas_xy(double x, double y) {
        return std::make_pair(x * CSS_SIZE, (1.0 - y) * CSS_SIZE);
    }
    std::pair<double, double> world_xy(double x, double y) {
        return std::make_pair(x / CSS_SIZE, 1.0 - y / CSS_SIZE);
    }
};

static Renderer G;

static EM_BOOL on_mouse(int, const EmscriptenMouseEvent* e, void*) {
    G.onClick(e->targetX, e->targetY);
    return EM_TRUE;
}

static void main_loop() {G.draw();}

int main() {
    js_setup_canvas(CSS_SIZE);
    emscripten_set_mousedown_callback("#canvas", nullptr, 0, on_mouse);
    emscripten_set_main_loop(main_loop, 0, 1);
}