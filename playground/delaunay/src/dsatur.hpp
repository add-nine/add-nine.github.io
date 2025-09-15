#pragma once
#include "components.hpp"
#include <vector>
#include <set>
#include <queue>
#include <tuple>
#include <string>
#include <boost/dynamic_bitset.hpp>

namespace dsatur {
    std::vector<int> colorize(const std::vector<Node>& P, const std::vector<std::pair<int,int>> &E) {
        const int N = P.size();
        if (N < 3) return std::vector<int>(N, 0);
        std::vector<std::vector<int>> G(N);
        std::vector<int> deg(N, 0);
        for (const auto& [u, v] : E) {
            G[u].emplace_back(v);
            G[v].emplace_back(u);
            deg[u]++;
            deg[v]++;
        }
        std::vector<int> color(N, -1), sat(N, 0);
        std::vector<boost::dynamic_bitset<>> usd(N);
        std::priority_queue<std::tuple<int,int,int>> que;
        for (int i{}; i < N; i++) que.emplace(sat[i], deg[i], i);
        for (int i{}; i < N; i++) {
            int u = -1;
            while (!que.empty()) {
                auto [clrd, sz, cand] = que.top(); que.pop();
                if (color[cand] != -1 || clrd != sat[cand]) continue;
                u = cand;
                break;
            }
            color[u] = 0;
            while (color[u] < (int)usd[u].size() && usd[u].test(color[u])) color[u]++;
            usd[u].resize(color[u] + 1);
            for (auto v : G[u]) {
                if (color[v] != -1) continue;
                if ((int)usd[v].size() <= color[u]) {
                    usd[v].resize(color[u] + 1);
                }
                if (usd[v].test(color[u])) continue;
                usd[v].set(color[u]);
                sat[v]++;
                que.emplace(sat[v], deg[v], v);
            }
        }
        return color;
    }

    std::vector<std::tuple<int,int,int>> rgb_color(std::vector<int> color) {
        std::sort(color.begin(), color.end());
        color.erase(unique(color.begin(), color.end()), color.end());
        const int K = color.size();
        std::vector<std::tuple<int,int,int>> rgb(K);
        for (auto i : color) {
            auto [h, s, v] = std::make_tuple(360.0 * i / K, 1.0, 1.0);
            auto to_rgb = [&](double h, double s, double v) {
                double c = v * s;
                double x = c * (1 - std::abs(std::fmod(h/60, 2) - 1));
                double m = v - c;
                auto [r, g, b] = [&] -> std::tuple<double, double, double> {
                    if (h < 60) return std::make_tuple(c, x, 0);
                    if (h < 120) return std::make_tuple(x, c, 0);
                    if (h < 180) return std::make_tuple(0, c, x);
                    if (h < 240) return std::make_tuple(0, x, c);
                    if (h < 300) return std::make_tuple(x, 0, c);
                    if (h < 360) return std::make_tuple(c, 0, x);
                }();
                return std::make_tuple((r + m) * 255, (g + m) * 255, (b + m) * 255);
            };
            rgb[i] = to_rgb(h, s, v);
        }
        return rgb;
    }
}