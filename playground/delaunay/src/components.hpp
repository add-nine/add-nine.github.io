#pragma once
#include <vector>
#include <cmath>
#include <cassert>

constexpr double EPS = 1e-12;

struct Node {
    double x{}, y{};
    Node() = default;
    Node(double _x, double _y) : x(_x), y(_y) {}
    Node operator+(const Node& a) const {return Node(x + a.x, y + a.y);}
    Node operator-(const Node& a) const {return Node(x - a.x, y - a.y);}
    Node operator*(const double d) const {return Node(x * d, y * d);}
    Node operator/(const double d) const {assert(std::abs(d) > EPS); return Node(x / d, y / d);}
    bool operator==(const Node& a) const {return std::hypot(x - a.x, y - a.y) < EPS;}
    bool operator<(const Node& a) const {return x == a.x ? y < a.y : x < a.x;}

    friend Node operator*(const double d, const Node& a) {return Node(a.x * d, a.y * d);}
};

inline double dot(const Node& a, const Node& b) {return a.x * b.x + a.y * b.y;}
inline double cross(const Node& a, const Node& b) {return a.x * b.y - a.y * b.x;}
inline bool cw_orient(const Node&a, const Node& b, const Node& c) {return cross(b - a, c - a) < 0;}
inline bool ccw_orient(const Node&a, const Node& b, const Node& c) {return cross(b - a, c - a) > 0;}
inline bool in_circle(const Node& a, const Node& b, const Node& c, const Node& p) {
    return dot(a - p, a - p)*cross(b - p, c - p) + dot(b - p, b - p)*cross(c - p, a - p) + dot(c - p, c - p)*cross(a - p, b - p) > 0;
}

struct Edge {
    uint32_t from, to;
    Edge(const uint32_t u, const uint32_t v) : from(u), to(v) {}
};

struct Triangle {
    uint32_t p[3];
    Triangle(const uint32_t u, const uint32_t v, const uint32_t w) : p{u, v, w} {}

    uint32_t opposite(const uint32_t u, const uint32_t v) const {
        for (auto w : p) {
            if (u == w || v == w) continue;
            return w;
        }
        return UINT32_MAX;
    }
};