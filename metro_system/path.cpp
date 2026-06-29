#include "path.h"
#include <queue>
#include <map>
#include <set>
#include <algorithm>
#include <climits>

// ========== 内部工具 ==========

int Path::charWidth(const string& s) {
    int w = 0;
    for (size_t i = 0; i < s.length(); i++)
        w += ((unsigned char)s[i] >= 0x80) ? 2 : 1;
    return w;
}

string Path::padTo(const string& s, int targetWidth) {
    int cur = charWidth(s);
    return (cur >= targetWidth) ? s + " " : s + string(targetWidth - cur, ' ');
}

vector<int> Path::callDijkstra(int start, int end, int& time, vector<string>& detail) {
    if (dijkstraFn) return dijkstraFn(graph, start, end, time, detail);
    // 后备: 无Dijkstra时返回空（由调用方处理）
    time = -1;
    return vector<int>();
}

// ================================================
//  C-1: 最少换乘路径
//  算法: 优先队列 — 换乘次数×100000 + 耗时
// ================================================
PathResult Path::minTransferPath(int startId, int endId) {
    PathResult result;
    if (startId == endId) {
        result.totalTime = 0;
        result.transferCount = 0;
        result.stationIds.push_back(startId);
        result.stepDetails.push_back("起点与终点相同，无需规划。");
        return result;
    }

    map<int, int> dist, trans, parent, edgeUsed;
    map<int, bool> visited;
    priority_queue<pair<int, int>, vector<pair<int, int> >,
                   greater<pair<int, int> > > pq;

    for (size_t i = 0; i < graph->stations.size(); i++) {
        int id = graph->stations[i].id;
        dist[id]  = INT_MAX;
        trans[id] = INT_MAX;
    }
    dist[startId]  = 0;
    trans[startId] = 0;
    pq.push(make_pair(0, startId));

    while (!pq.empty()) {
        int priority = pq.top().first;
        int u = pq.top().second;
        pq.pop();

        int curTrans = priority / 100000;
        int curDist  = priority % 100000;

        if (visited.count(u) && visited[u]) continue;
        visited[u] = true;
        if (u == endId) break;

        for (size_t j = 0; j < graph->adj[u].size(); j++) {
            int ei = graph->adj[u][j];
            Edge& e = graph->edges[ei];
            int v = e.to;
            if (visited.count(v) && visited[v]) continue;
            Station* sv = graph->getStation(v);
            if (!sv || !sv->open) continue;

            int nt = curTrans + (e.isTransfer ? 1 : 0);
            int nd = curDist  + e.time;

            if (nt < trans[v] || (nt == trans[v] && nd < dist[v])) {
                trans[v]     = nt;
                dist[v]      = nd;
                parent[v]    = u;
                edgeUsed[v]  = ei;
                pq.push(make_pair(nt * 100000 + nd, v));
            }
        }
    }

    if (!visited.count(endId) || !visited[endId]) return result;

    result.transferCount = trans[endId];
    result.totalTime     = dist[endId];

    vector<int> path;
    for (int cur = endId; cur != startId; cur = parent[cur])
        path.push_back(cur);
    path.push_back(startId);
    reverse(path.begin(), path.end());
    result.stationIds = path;

    string prevLine = "";
    for (size_t i = 0; i < path.size(); i++) {
        Station* s = graph->getStation(path[i]);
        if (!s) continue;
        string curLine = s->line;
        if (i > 0 && edgeUsed.count(path[i])) {
            Edge& e = graph->edges[edgeUsed[path[i]]];
            if (e.isTransfer) {
                result.stepDetails.push_back("  [换乘] " + prevLine + " -> " + curLine
                    + " (步行" + to_string(e.time) + "分钟)");
            } else {
                Station* ps = graph->getStation(path[i - 1]);
                result.stepDetails.push_back("  " + (ps ? ps->name : "?") + " -> " + s->name
                    + "  [" + curLine + "]  " + e.direction + "  " + to_string(e.time) + "分钟");
            }
        }
        prevLine = curLine;
    }
    return result;
}

// ================================================
//  C-2: K条最短时间路径（Yen's算法）
//  调用成员B的Dijkstra作为子过程
// ================================================
vector<PathResult> Path::kShortestTimePaths(int startId, int endId, int K) {
    vector<PathResult> results;
    if (K <= 0) return results;

    // 第一条: 用B的Dijkstra
    int t0;
    vector<string> d0;
    vector<int> p0 = callDijkstra(startId, endId, t0, d0);
    if (p0.empty()) return results;

    PathResult first;
    first.stationIds   = p0;
    first.totalTime    = t0;
    first.stepDetails  = d0;
    first.transferCount = 0;
    for (size_t i = 0; i < d0.size(); i++)
        if (d0[i].find("[换乘]") != string::npos) first.transferCount++;
    results.push_back(first);

    vector<pair<vector<int>, int> > bPool;

    for (int k = 1; k < K; k++) {
        vector<int>& anchor = results[k - 1].stationIds;

        for (size_t spurIdx = 0; spurIdx < anchor.size() - 1; spurIdx++) {
            int spurNode = anchor[spurIdx];
            vector<int> rootPath(anchor.begin(), anchor.begin() + spurIdx);

            int st;
            vector<string> sd;
            vector<int> sp = callDijkstra(spurNode, endId, st, sd);
            if (sp.empty()) continue;

            vector<int> total = rootPath;
            total.insert(total.end(), sp.begin() + 1, sp.end());

            // 验证边
            bool ok = true;
            int fullTime = 0;
            for (size_t j = 0; j + 1 < total.size(); j++) {
                bool edgeFound = false;
                for (size_t m = 0; m < graph->adj[total[j]].size(); m++) {
                    if (graph->edges[graph->adj[total[j]][m]].to == total[j + 1]) {
                        fullTime += graph->edges[graph->adj[total[j]][m]].time;
                        edgeFound = true; break;
                    }
                }
                if (!edgeFound) { ok = false; break; }
            }
            if (!ok || fullTime <= 0) continue;

            // 去重
            bool dup = false;
            for (size_t j = 0; j < results.size(); j++)
                if (results[j].stationIds == total) { dup = true; break; }
            for (size_t j = 0; j < bPool.size(); j++)
                if (bPool[j].first == total) { dup = true; break; }
            if (dup) continue;

            bPool.push_back(make_pair(total, fullTime));
        }

        if (bPool.empty()) break;

        size_t best = 0;
        int bestTime = INT_MAX;
        for (size_t i = 0; i < bPool.size(); i++)
            if (bPool[i].second < bestTime) { bestTime = bPool[i].second; best = i; }

        PathResult pr;
        pr.stationIds = bPool[best].first;
        pr.totalTime  = bPool[best].second;
        pr.transferCount = 0;

        string prevLine = "";
        for (size_t i = 0; i < pr.stationIds.size(); i++) {
            Station* s = graph->getStation(pr.stationIds[i]);
            if (!s) continue;
            string curLine = s->line;
            if (i > 0) {
                Station* ps = graph->getStation(pr.stationIds[i - 1]);
                if (ps && ps->line != curLine) {
                    pr.stepDetails.push_back("  [换乘] " + ps->line + " -> " + curLine);
                    pr.transferCount++;
                } else {
                    pr.stepDetails.push_back("  " + (ps ? ps->name : "?") + " -> " + s->name + "  [" + curLine + "]");
                }
            }
            prevLine = curLine;
        }
        results.push_back(pr);
        bPool.erase(bPool.begin() + best);
    }
    return results;
}

// ================================================
//  C-3: 路径打印（美观输出）
// ================================================
void Path::printPath(const PathResult& result) {
    if (!result.valid()) {
        cout << "\n  " << string(48, '~') << "\n"
             << "  | " << padTo("!! 无 可 达 路 径  !!", 44) << "|\n"
             << "  " << string(48, '~') << "\n";
        return;
    }

    Station* first = graph->getStation(result.stationIds.front());
    Station* last  = graph->getStation(result.stationIds.back());

    cout << "\n  " << string(48, '=') << "\n"
         << "  |" << padTo("   地铁路径规划结果", 44) << "|\n"
         << "  " << string(48, '-') << "\n"
         << "  | 起点: " << padTo(first ? first->name : "?", 16)
         << " [" << (first ? first->line : "") << "]" << padTo("", 10) << "|\n"
         << "  | 终点: " << padTo(last ? last->name : "?", 16)
         << " [" << (last ? last->line : "") << "]" << padTo("", 10) << "|\n"
         << "  " << string(48, '-') << "\n"
         << "  | 总耗时:   " << setw(4) << result.totalTime        << " 分钟" << padTo("", 14) << "|\n"
         << "  | 换乘次数: " << setw(4) << result.transferCount    << " 次"   << padTo("", 16) << "|\n"
         << "  | 途径站点: " << setw(4) << result.stationIds.size() << " 站"   << padTo("", 16) << "|\n"
         << "  " << string(48, '=') << "\n";

    // 乘车路线
    cout << "\n  >>> 乘车路线 <<<\n\n";
    string curLine = "";
    int cnt = 0;
    for (size_t i = 0; i < result.stationIds.size(); i++) {
        Station* s = graph->getStation(result.stationIds[i]);
        if (!s) continue;
        if (s->line != curLine) {
            if (!curLine.empty() && cnt > 0)
                cout << "        [" << string(28, ' ') << "]  (" << cnt << "站)\n\n";
            curLine = s->line; cnt = 0;
            cout << "  " << padTo(curLine, 12);
        }
        cout << (cnt == 0 ? "  " : "  ->  ") << padTo(s->name, 14);
        if (cnt == 0) {
            // 换乘站标注
            string others;
            for (size_t j = 0; j < graph->stations.size(); j++)
                if (graph->stations[j].name == s->name && graph->stations[j].line != s->line)
                    others += (others.empty() ? "" : "/") + graph->stations[j].line;
            if (!others.empty()) cout << " (" << others << ")";
        }
        if (!s->open) cout << " [关闭]";
        cout << "\n"; cnt++;
    }
    if (!curLine.empty() && cnt > 0)
        cout << "        [" << string(28, ' ') << "]  (" << cnt << "站)\n";

    // 换乘详情
    cout << "\n  >>> 换乘详情 <<<\n\n";
    bool hasTransfer = false;
    for (size_t i = 0; i < result.stepDetails.size(); i++)
        if (result.stepDetails[i].find("[换乘]") != string::npos) {
            cout << "  * " << result.stepDetails[i] << "\n"; hasTransfer = true;
        }
    if (!hasTransfer) cout << "    (同线路直达，无需换乘)\n";

    // 完整步骤
    cout << "\n  >>> 完整步骤 <<<\n\n";
    if (result.stepDetails.empty()) {
        cout << "    (同线路直达)\n";
    } else {
        for (size_t i = 0; i < result.stepDetails.size(); i++)
            cout << "  " << (i + 1) << ". " << result.stepDetails[i] << "\n";
    }
    cout << "\n  " << string(48, '=') << "\n";
}

// ================================================
//  C-4: 路径摘要
// ================================================
string Path::pathSummary(const PathResult& result) {
    if (!result.valid()) return "[无可达路径]";
    Station* f = graph->getStation(result.stationIds.front());
    Station* l = graph->getStation(result.stationIds.back());
    return (f ? f->name : "?") + " -> " + (l ? l->name : "?")
           + " | 耗时" + to_string(result.totalTime) + "分钟"
           + " | 换乘" + to_string(result.transferCount) + "次"
           + " | 共" + to_string(result.stationIds.size()) + "站";
}

// ================================================
//  C-5: 路径有效性验证
// ================================================
bool Path::isValidPath(const PathResult& result) {
    if (!result.valid()) return false;
    if (result.stationIds.size() < 2) return true;
    for (size_t i = 0; i + 1 < result.stationIds.size(); i++) {
        Station* from = graph->getStation(result.stationIds[i]);
        Station* to   = graph->getStation(result.stationIds[i + 1]);
        if (!from || !to) return false;
        if (!from->open || !to->open) return false;
        bool ok = false;
        for (size_t j = 0; j < graph->adj[from->id].size(); j++)
            if (graph->edges[graph->adj[from->id][j]].to == to->id) { ok = true; break; }
        if (!ok) return false;
    }
    return true;
}
