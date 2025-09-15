#pragma once
#include "components.hpp"
#include <vector>
#include <list>
#include <unordered_map>
#include <tuple>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <numeric>
#include <cstdint>

namespace delaunay {

class DelaunayTriangulation {
    static inline uint64_t eidx(const uint32_t from, const uint32_t to) {
        return ((uint64_t)from << 32) | to;
    }
    Triangle make_triangle(uint32_t u, uint32_t v, uint32_t w) {
        if (!ccw_orient(points[u], points[v], points[w])) std::swap(u, v);
        return Triangle(u, v, w);
    }
public:
    uint32_t addPoint(double x, double y) {
        up_to_date = false;
        points.emplace_back(x, y);
        return (uint32_t)points.size() - 1;
    }
    void clear() {
        points.clear();
        triangles.clear();
        hull.clear();
        hull_id.clear();
        up_to_date = false;
    }
    void build() {
        if (up_to_date) return;
        up_to_date = true;
        if ((int)points.size() < 3) return;
        triangles.clear();
        hull.clear();
        hull_id.clear();
        uint32_t N = points.size();
        std::vector<uint32_t> ord(N);
        std::iota(ord.begin(), ord.end(), 0);
        std::sort(ord.begin(), ord.end(), [&](uint32_t i, uint32_t j) {return points[i] < points[j];});
        initHull(ord[0], ord[1], ord[2]);
        for (uint32_t i{3}; i < N; i++) {
            expandHull(ord[i]);
            LawsonLegalization();
        }
    }
    const std::vector<Triangle>& getTriangles() const {return triangles;}
    const std::vector<Node>& getPoints() const {return points;}
    std::vector<std::pair<int,int>> getEdges() const {
        std::vector<std::pair<int,int>> E;
        for (const auto& tri : triangles) {
            for (uint32_t i{}; i < 3; i++) {
                E.emplace_back(std::minmax(tri.p[i%3], tri.p[(i+1)%3]));
            }
        }
        std::sort(E.begin(), E.end());
        E.erase(std::unique(E.begin(), E.end()), E.end());
        return E;
    }
private:
    std::tuple<uint32_t, uint32_t, uint32_t> addTriangle(const uint32_t u, const uint32_t v, const uint32_t w) {
        assert(std::max({u, v, w}) < (uint32_t)points.size());
        Triangle tri = make_triangle(u, v, w);
        triangles.emplace_back(tri);
        return std::make_tuple(tri.p[0], tri.p[1], tri.p[2]);
    }
    void addHullEdge(const std::list<Edge>::iterator it, const uint32_t p) {
        assert((int)hull.size() > 1);
        auto [from, to] = *it;
        auto nit = std::next(it);
        if (nit == hull.end()) nit = hull.begin();
        hull.erase(it);
        hull_id.erase(eidx(from, to));
        auto jt1 = hull_id.find(eidx(p, from));
        if (jt1 != hull_id.end()) {
            hull.erase(jt1->second);
            hull_id.erase(jt1);
        }
        else {
            auto it1 = hull.insert(nit, Edge(from, p));
            hull_id[eidx(from, p)] = it1;
        }
        auto jt2 = hull_id.find(eidx(to, p));
        if (jt2 != hull_id.end()) {
            hull.erase(jt2->second);
            hull_id.erase(jt2);
        }
        else {
            auto it2 = hull.insert(nit, Edge(p, to));
            hull_id[eidx(p, to)] = it2;
        }
    }
    void initHull(const uint32_t i0, const uint32_t i1, const uint32_t i2) {
        auto [p0, p1, p2] = addTriangle(i0, i1, i2);
        auto e0 = hull.insert(hull.end(), Edge(p0, p1));
        hull_id[eidx(p0, p1)] = e0;
        auto e1 = hull.insert(hull.end(), Edge(p1, p2));
        hull_id[eidx(p1, p2)] = e1;
        auto e2 = hull.insert(hull.end(), Edge(p2, p0));
        hull_id[eidx(p2, p0)] = e2;
    }
    void expandHull(const uint32_t p) {
        if (hull.empty()) return;
        const auto tmp = hull;
        for (auto [from, to] : tmp) {
            auto it = hull_id.find(eidx(from, to));
            if (it == hull_id.end()) continue;
            if (!ccw_orient(points[from], points[to], points[p])) {
                addHullEdge(it->second, p); addTriangle(from, to, p);
            }
        }
    }
    void LawsonLegalization() {
        const auto e_id = [&](uint32_t u, uint32_t v) -> uint64_t {
            return eidx(std::min(u, v), std::max(u, v));
        };
        const auto unpack = [](uint64_t id) -> std::pair<uint32_t, uint32_t> {
            return std::make_pair(id >> 32, id & 0xffffffffu);
        };
        std::unordered_map<uint64_t, std::pair<int, int>> e_to_t;
        auto tag = [&](uint64_t key, uint32_t t) {
            if (!e_to_t.count(key)) e_to_t[key] = std::make_pair(-1, -1);
            (e_to_t[key].first == -1 ? e_to_t[key].first : e_to_t[key].second) = t;
        };
        auto detag = [&](uint64_t key, uint32_t t) {
            auto it = e_to_t.find(key);
            if (it == e_to_t.end()) return;
            auto& ts = it->second;
            if (ts.first == t) ts.first = -1;
            if (ts.second == t) ts.second = -1;
            if (ts.first == -1 && ts.second == -1) e_to_t.erase(it);
        };
        for (uint32_t t{}; t < (uint32_t)triangles.size(); t++) {
            const auto& tri = triangles[t];
            for (uint32_t i{}; i < 3; i++) {
                tag(e_id(tri.p[i], tri.p[(i+1)%3]), t);
            }
        }
        std::vector<uint64_t> st;
        for (auto& [e, ts] : e_to_t) {
            if (ts.first == -1 || ts.second == -1) continue;
            st.emplace_back(e);
        }
        while (!st.empty()) {
            uint64_t e = st.back(); st.pop_back();
            auto it = e_to_t.find(e);
            if (it == e_to_t.end()) continue;
            auto [tl, tr] = e_to_t[e];
            if (tl == -1 || tr == -1) continue;
            auto [u, v] = unpack(e);
            uint32_t wl = triangles[tl].opposite(u, v), wr = triangles[tr].opposite(u, v);
            if (wl >= (uint32_t)points.size() || wr >= (uint32_t)points.size()) continue;
            if (!ccw_orient(points[u], points[v], points[wl])) std::swap(u, v);
            if (in_circle(points[u], points[v], points[wl], points[wr])) {
                auto add_edge = [&](uint32_t t) {
                    const auto& tri = triangles[t];
                    for (uint32_t i{}; i < 3; i++) {
                        tag(e_id(tri.p[i], tri.p[(i+1)%3]), t);
                    }
                };
                auto remove_edge = [&](uint32_t t) {
                    const auto& tri = triangles[t];
                    for (uint32_t i{}; i < 3; i++) {
                        detag(e_id(tri.p[i], tri.p[(i+1)%3]), t);
                    }
                };
                remove_edge(tl);
                remove_edge(tr);
                triangles[tl] = make_triangle(wl, wr, u);
                triangles[tr] = make_triangle(wl, wr, v);
                add_edge(tl);
                add_edge(tr);
                
                for (auto a : {u, v}) {
                    for (auto b : {wl, wr}) {
                        auto it = e_to_t.find(e_id(a, b));
                        if (it == e_to_t.end()) continue;
                        if (it->second.first == -1 || it->second.second == -1) continue;
                        st.emplace_back(e_id(a, b));
                    }
                }
            }
        }
    }
private:
    std::vector<Node> points;
    std::vector<Triangle> triangles;
    std::list<Edge> hull;
    std::unordered_map<uint64_t, std::list<Edge>::iterator> hull_id;
    bool up_to_date = false;
};

}