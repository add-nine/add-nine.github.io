#pragma once
#include "components.hpp"
#include <vector>
#include <set>
#include <queue>
#include <tuple>
#include <string>
#include <unordered_map>
#include <boost/dynamic_bitset.hpp>

namespace dsatur {
    std::vector<int> color_point(const std::vector<Node>& P, const std::vector<std::pair<int,int>> &E) {
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

    std::vector<int> color_edge(const std::vector<Node>& P, const std::vector<std::pair<int,int>> &E) {
        const int N = P.size();
        const int M = E.size();
        if (N < 3) return std::vector<int>(M, 0);
        std::vector<std::vector<int>> G(N);
        std::vector<int> deg(N, 0);
        for (int i{}; i < M; i++) {
            const auto& [u, v] = E[i];
            G[u].emplace_back(i);
            G[v].emplace_back(i);
            deg[u]++;
            deg[v]++;
        }
        const auto edeg = [&](int i) {
            return deg[E[i].first] + deg[E[i].second] - 2;
        };
        std::vector<int> color(M, -1), sat(M, 0);
        std::vector<boost::dynamic_bitset<>> usd(M);
        std::priority_queue<std::tuple<int,int,int>> que;
        for (int i{}; i < M; i++) que.emplace(sat[i], edeg(i), i);
        for (int i{}; i < M; i++) {
            int j = -1;
            while (!que.empty()) {
                auto [clrd, sz, cand] = que.top(); que.pop();
                if (color[cand] != -1 || clrd != sat[cand]) continue;
                j = cand;
                break;
            }
            color[j] = 0;
            while (color[j] < (int)usd[j].size() && usd[j].test(color[j])) color[j]++;
            usd[j].resize(color[j] + 1);
            for (auto k : G[E[j].first]) {
                if (k == j || color[k] != -1) continue;
                if ((int)usd[k].size() <= color[j]) {
                    usd[k].resize(color[j] + 1);
                }
                if (usd[k].test(color[j])) continue;
                usd[k].set(color[j]);
                sat[k]++;
                que.emplace(sat[k], edeg(k), k);
            }
            for (auto k : G[E[j].second]) {
                if (k == j || color[k] != -1) continue;
                if ((int)usd[k].size() <= color[j]) {
                    usd[k].resize(color[j] + 1);
                }
                if (usd[k].test(color[j])) continue;
                usd[k].set(color[j]);
                sat[k]++;
                que.emplace(sat[k], edeg(k), k);
            }
        }
        return color;
    }

    std::vector<int> color_face(const std::vector<Node>& P, const std::vector<Triangle>& T) {
        const int N = P.size();
        const int K = T.size();
        if (N < 3) return std::vector<int>(K, 0);
        std::vector<std::vector<int>> G(N);
        std::vector<int> nb(N, 0);
        for (int i{}; i < K; i++) {
            const auto& t = T[i];
            for (int j{}; j < 3; j++) {
                G[t.p[j]].emplace_back(i);
                nb[t.p[j]]++;
            }
        }
        const auto tdeg = [&](int i) {
            return nb[T[i].p[0]] + nb[T[i].p[1]] + nb[T[i].p[2]] - 5;
        };
        std::vector<int> color(K, -1), sat(K, 0);
        std::vector<boost::dynamic_bitset<>> usd(K);
        std::priority_queue<std::tuple<int,int,int>> que;
        for (int i{}; i < K; i++) que.emplace(sat[i], tdeg(i), i);
        for (int i{}; i < K; i++) {
            int j = -1;
            while (!que.empty()) {
                auto [clrd, sz, cand] = que.top(); que.pop();
                if (color[cand] != -1 || clrd != sat[cand]) continue;
                j = cand;
                break;
            }
            color[j] = 0;
            while (color[j] < (int)usd[j].size() && usd[j].test(color[j])) color[j]++;
            usd[j].resize(color[j] + 1);
            for (int x{}; x < 3; x++) {
                for (auto k : G[T[j].p[x]]) {
                    if (k == j || color[k] != -1) continue;
                    if ((int)usd[k].size() <= color[j]) {
                        usd[k].resize(color[j] + 1);
                    }
                    if (usd[k].test(color[j])) continue;
                    usd[k].set(color[j]);
                    sat[k]++;
                    que.emplace(sat[k], tdeg(k), k);
                }
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
                    return std::make_tuple(c, 0, x);
                }();
                return std::make_tuple((r + m) * 255, (g + m) * 255, (b + m) * 255);
            };
            rgb[i] = to_rgb(h, s, v);
        }
        return rgb;
    }
}