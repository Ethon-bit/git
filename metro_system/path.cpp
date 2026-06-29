#include "path.h"

// ========== 辅助: 调整中文字符宽度 ==========
static int charWidth(const string& s) {
    int w = 0;
    for (size_t i = 0; i < s.length(); i++) {
        if ((unsigned char)s[i] >= 0x80) { w += 2; /* 取下一个字节 */ }
        else { w += 1; }
    }
    return w;
}

static string padTo(const string& s, int targetWidth) {
    int cur = charWidth(s);
    if (cur >= targetWidth) return s + " ";
    return s + string(targetWidth - cur, ' ');
}

// ========== Dijkstra 最短时间路径 ==========
vector<int> Path::dijkstra(int start, int end, int& totalTime, vector<string>& details) {
    totalTime = -1;
    details.clear();

    map<int, int> dist, parent, edgeUsed;
    map<int, bool> visited;
    priority_queue<pair<int, int>, vector<pair<int, int> >, greater<pair<int, int> > > pq;

    for (size_t i = 0; i < graph->stations.size(); i++)
        dist[graph->stations[i].id] = INT_MAX;
    dist[start] = 0;
    pq.push(make_pair(0, start));

    while (!pq.empty()) {
        int d = pq.top().first;
        int u = pq.top().second;
        pq.pop();
        if (visited[u]) continue;
        visited[u] = true;
        if (u == end) break;

        for (size_t j = 0; j < graph->adj[u].size(); j++) {
            int ei = graph->adj[u][j];
            Edge& e = graph->edges[ei];
            int v = e.to;
            if (visited.count(v) && visited[v]) continue;
            Station* sv = graph->getStation(v);
            if (!sv || !sv->open) continue;

            int nd = d + e.time;
            if (nd < dist[v]) {
                dist[v] = nd;
                parent[v] = u;
                edgeUsed[v] = ei;
                pq.push(make_pair(nd, v));
            }
        }
    }

    if (dist[end] == INT_MAX) return vector<int>();
    totalTime = dist[end];

    vector<int> path;
    for (int cur = end; cur != start; cur = parent[cur])
        path.push_back(cur);
    path.push_back(start);
    reverse(path.begin(), path.end());

    // 生成步骤详情 — 含线路信息、方向、时间
    string prevLine = "";
    for (size_t i = 0; i < path.size(); i++) {
        Station* s = graph->getStation(path[i]);
        string curLine = s ? s->line : "";
        if (i > 0 && edgeUsed.count(path[i])) {
            Edge& e = graph->edges[edgeUsed[path[i]]];
            if (e.isTransfer) {
                details.push_back("  [换乘] " + prevLine + " -> " + curLine
                                  + " (步行" + to_string(e.time) + "分钟)");
            } else {
                Station* ps = graph->getStation(path[i - 1]);
                details.push_back("  " + (ps ? ps->name : "?") + " -> " + s->name
                                  + "  [" + curLine + "]  " + e.direction
                                  + "  " + to_string(e.time) + "分钟");
            }
        }
        prevLine = curLine;
    }
    return path;
}

// ========== 最短时间路径（公开接口） ==========
PathResult Path::shortestTimePath(int startId, int endId) {
    PathResult result;
    if (startId == endId) {
        result.totalTime = 0;
        result.transferCount = 0;
        result.stationIds.push_back(startId);
        result.stepDetails.push_back("起点与终点相同，无需规划。");
        return result;
    }

    int totalTime;
    vector<string> details;
    vector<int> path = dijkstra(startId, endId, totalTime, details);

    if (path.empty()) return result;

    result.stationIds = path;
    result.totalTime = totalTime;
    result.stepDetails = details;

    // 统计换乘次数
    int transfers = 0;
    for (size_t i = 0; i < details.size(); i++) {
        if (details[i].find("[换乘]") != string::npos) transfers++;
    }
    result.transferCount = transfers;
    return result;
}

// ========== 最少换乘路径（BFS变种 + 优先队列） ==========
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
    priority_queue<pair<int, int>, vector<pair<int, int> >, greater<pair<int, int> > > pq;

    for (size_t i = 0; i < graph->stations.size(); i++) {
        int id = graph->stations[i].id;
        dist[id] = INT_MAX;
        trans[id] = INT_MAX;
    }
    dist[startId] = 0;
    trans[startId] = 0;
    pq.push(make_pair(0, startId));

    while (!pq.empty()) {
        int priority = pq.top().first;
        int u = pq.top().second;
        pq.pop();

        int currentTrans = priority / 100000;
        int currentDist  = priority % 100000;

        if (visited[u]) continue;
        visited[u] = true;
        if (u == endId) break;

        for (size_t j = 0; j < graph->adj[u].size(); j++) {
            int ei = graph->adj[u][j];
            Edge& e = graph->edges[ei];
            int v = e.to;
            if (visited.count(v) && visited[v]) continue;
            Station* sv = graph->getStation(v);
            if (!sv || !sv->open) continue;

            int newTrans = currentTrans + (e.isTransfer ? 1 : 0);
            int newDist  = currentDist + e.time;

            if (newTrans < trans[v] || (newTrans == trans[v] && newDist < dist[v])) {
                trans[v] = newTrans;
                dist[v]  = newDist;
                parent[v] = u;
                edgeUsed[v] = ei;
                pq.push(make_pair(newTrans * 100000 + newDist, v));
            }
        }
    }

    if (trans[endId] == INT_MAX) return result;

    result.transferCount = trans[endId];
    result.totalTime = dist[endId];

    // 回溯路径
    vector<int> path;
    for (int cur = endId; cur != startId; cur = parent[cur])
        path.push_back(cur);
    path.push_back(startId);
    reverse(path.begin(), path.end());
    result.stationIds = path;

    // 生成详细步骤
    string prevLine = "";
    for (size_t i = 0; i < path.size(); i++) {
        Station* s = graph->getStation(path[i]);
        string curLine = s ? s->line : "";
        if (i > 0 && edgeUsed.count(path[i])) {
            Edge& e = graph->edges[edgeUsed[path[i]]];
            if (e.isTransfer) {
                result.stepDetails.push_back(
                    "  [换乘] " + prevLine + " -> " + curLine + " (步行" + to_string(e.time) + "分钟)");
            } else {
                Station* ps = graph->getStation(path[i - 1]);
                result.stepDetails.push_back(
                    "  " + (ps ? ps->name : "?") + " -> " + s->name
                    + "  [" + curLine + "]  " + e.direction + "  " + to_string(e.time) + "分钟");
            }
        }
        prevLine = curLine;
    }
    return result;
}

// ========== K条最短时间路径（Yen's算法 · 完整版） ==========
vector<PathResult> Path::kShortestTimePaths(int startId, int endId, int K) {
    vector<PathResult> results;
    if (K <= 0) return results;

    // 第一条：Dijkstra
    PathResult first = shortestTimePath(startId, endId);
    if (!first.valid()) return results;
    results.push_back(first);

    // B集合（候选路径池）
    vector<pair<vector<int>, int> > bPool; // {path, totalTime}

    for (int k = 1; k < K; k++) {
        vector<int>& anchor = results[k - 1].stationIds;

        // 对 anchor 路径的每一个节点作为偏离点
        for (size_t spurIdx = 0; spurIdx < anchor.size() - 1; spurIdx++) {
            int spurNode = anchor[spurIdx];
            vector<int> rootPath(anchor.begin(), anchor.begin() + spurIdx);

            // 计算 rootTime
            int rootTime = 0;
            string lastLine = "";
            for (size_t j = 0; j + 1 < rootPath.size(); j++) {
                int stationA = rootPath[j], stationB = rootPath[j + 1];
                bool foundEdge = false;
                for (size_t m = 0; m < graph->adj[stationA].size(); m++) {
                    Edge& ei = graph->edges[graph->adj[stationA][m]];
                    if (ei.to == stationB) {
                        rootTime += ei.time;
                        foundEdge = true;
                        break;
                    }
                }
                if (!foundEdge) rootTime = INT_MAX;
            }
            if (rootTime == INT_MAX) continue;

            // 从 spurNode 出发跑 Dijkstra 到终点
            int spurTime;
            vector<string> spurDetail;
            vector<int> spurPath = dijkstra(spurNode, endId, spurTime, spurDetail);

            if (!spurPath.empty()) {
                // 拼接完整路径: rootPath + spurPath[1..]
                vector<int> totalPath = rootPath;
                totalPath.insert(totalPath.end(), spurPath.begin() + 1, spurPath.end());

                // 验证拼接后路径的有效性: 每对相邻节点必须有边连接
                bool pathValid = true;
                for (size_t j = 0; j + 1 < totalPath.size(); j++) {
                    bool edgeFound = false;
                    for (size_t m = 0; m < graph->adj[totalPath[j]].size(); m++) {
                        if (graph->edges[graph->adj[totalPath[j]][m]].to == totalPath[j + 1]) {
                            edgeFound = true; break;
                        }
                    }
                    if (!edgeFound) { pathValid = false; break; }
                }
                if (!pathValid) continue;

                // 计算完整路径的真实总时间
                int fullTime = 0;
                for (size_t j = 0; j + 1 < totalPath.size(); j++) {
                    for (size_t m = 0; m < graph->adj[totalPath[j]].size(); m++) {
                        if (graph->edges[graph->adj[totalPath[j]][m]].to == totalPath[j + 1]) {
                            fullTime += graph->edges[graph->adj[totalPath[j]][m]].time;
                            break;
                        }
                    }
                }
                if (fullTime <= 0) continue;

                // ====== 去重: 对比已找到的结果 ======
                auto pathExists = [](const vector<pair<vector<int>, int> >& pool, const vector<int>& p) {
                    for (size_t n = 0; n < pool.size(); n++)
                        if (pool[n].first == p) return true;
                    return false;
                };

                bool dupInResults = false;
                for (size_t j = 0; j < results.size(); j++) {
                    if (results[j].stationIds == totalPath) { dupInResults = true; break; }
                }
                if (!dupInResults && !pathExists(bPool, totalPath)) {
                    bPool.push_back(make_pair(totalPath, fullTime));
                }
            }
        }

        if (bPool.empty()) break;

        // 从 B 池中选总耗时最小的路径
        size_t bestIdx = 0;
        int bestTime = INT_MAX;
        for (size_t i = 0; i < bPool.size(); i++) {
            if (bPool[i].second < bestTime) {
                bestTime = bPool[i].second;
                bestIdx = i;
            }
        }

        // 构建 PathResult
        PathResult pr;
        pr.stationIds = bPool[bestIdx].first;
        pr.totalTime  = bPool[bestIdx].second;

        // 生成详细步骤
        string prevLine = "";
        for (size_t i = 0; i < pr.stationIds.size(); i++) {
            Station* s = graph->getStation(pr.stationIds[i]);
            if (!s) continue;
            string curLine = s->line;

            if (i > 0) {
                Station* ps = graph->getStation(pr.stationIds[i - 1]);
                bool isXF = (ps && ps->line != curLine);
                if (isXF) {
                    pr.stepDetails.push_back(
                        "  [换乘] " + (ps ? ps->line : "?") + " -> " + curLine);
                } else {
                    pr.stepDetails.push_back(
                        "  " + (ps ? ps->name : "?") + " -> " + s->name + "  [" + curLine + "]");
                }
            }
            prevLine = curLine;
        }

        // 统计换乘
        int transfers = 0;
        for (size_t i = 0; i < pr.stepDetails.size(); i++)
            if (pr.stepDetails[i].find("[换乘]") != string::npos) transfers++;
        pr.transferCount = transfers;

        results.push_back(pr);
        bPool.erase(bPool.begin() + bestIdx);
    }
    return results;
}

// ========== 打印路径（美观输出） ==========
void Path::printPath(const PathResult& result) {
    if (!result.valid()) {
        cout << "\n";
        cout << "  " << string(48, '~') << "\n";
        cout << "  |" << padTo("", 46) << "|\n";
        cout << "  |  " << padTo("!!  无 可 达 路 径  !!", 44) << "|\n";
        cout << "  |" << padTo("", 46) << "|\n";
        cout << "  |  可能原因:" << padTo("", 35) << "|\n";
        cout << "  |   - 起点或终点已关闭" << padTo("", 26) << "|\n";
        cout << "  |   - 网络中存在不连通区域" << padTo("", 22) << "|\n";
        cout << "  |   - 输入站点不存在" << padTo("", 28) << "|\n";
        cout << "  |" << padTo("", 46) << "|\n";
        cout << "  " << string(48, '~') << "\n";
        return;
    }

    Station* first = graph->getStation(result.stationIds.front());
    Station* last  = graph->getStation(result.stationIds.back());

    cout << "\n";
    cout << "  " << string(48, '=') << "\n";
    cout << "  |   地铁路径规划结果" << padTo("", 29) << "|\n";
    cout << "  " << string(48, '-') << "\n";
    cout << "  | 起点: " << padTo((first ? first->name : "?"), 16)
         << "[" << (first ? first->line : "") << "]" << padTo("", 12) << "|\n";
    cout << "  | 终点: " << padTo((last ? last->name : "?"), 16)
         << "[" << (last ? last->line : "") << "]" << padTo("", 12) << "|\n";
    cout << "  " << string(48, '-') << "\n";
    cout << "  | 总耗时:   " << setw(4) << result.totalTime        << " 分钟" << padTo("", 18) << "|\n";
    cout << "  | 换乘次数: " << setw(4) << result.transferCount    << " 次"   << padTo("", 20) << "|\n";
    cout << "  | 途径站点: " << setw(4) << result.stationIds.size() << " 站"   << padTo("", 20) << "|\n";
    cout << "  " << string(48, '=') << "\n";

    // 按线路分段输出
    cout << "\n  >>> 乘车路线 <<<\n\n";
    string currentLine = "";
    int lineStationCount = 0;
    for (size_t i = 0; i < result.stationIds.size(); i++) {
        Station* s = graph->getStation(result.stationIds[i]);
        if (!s) continue;

        if (s->line != currentLine) {
            if (!currentLine.empty() && lineStationCount > 0) {
                cout << "        [" << padTo("", 28) << "]  (" << lineStationCount << "站)\n\n";
            }
            currentLine = s->line;
            lineStationCount = 0;
            cout << "  " << padTo(currentLine, 12);
        }

        if (lineStationCount == 0) {
            cout << "  " << padTo(s->name, 14);
        } else {
            cout << "  ->  " << padTo(s->name, 14);
        }

        if (lineStationCount == 0) {
            // 换乘信息
            set<string> tlines;
            for (size_t j = 0; j < graph->stations.size(); j++) {
                if (graph->stations[j].name == s->name && graph->stations[j].line != s->line)
                    tlines.insert(graph->stations[j].line);
            }
            if (!tlines.empty()) {
                cout << " (";
                int c = 0;
                for (set<string>::iterator ti = tlines.begin(); ti != tlines.end() && c < 3; ++ti, ++c) {
                    if (c > 0) cout << "/";
                    cout << *ti;
                }
                cout << ")";
            }
        }

        if (!s->open) cout << " [关闭]";
        cout << "\n";
        lineStationCount++;
    }
    if (!currentLine.empty() && lineStationCount > 0) {
        cout << "        [" << padTo("", 28) << "]  (" << lineStationCount << "站)\n";
    }

    // 步骤详情
    cout << "\n  >>> 换乘详情 <<<\n\n";
    if (result.stepDetails.empty()) {
        cout << "    (同线路直达，无需换乘)\n";
    } else {
        for (size_t i = 0; i < result.stepDetails.size(); i++) {
            if (result.stepDetails[i].find("[换乘]") != string::npos) {
                cout << "  * " << result.stepDetails[i] << "\n";
            }
        }
    }

    cout << "\n  >>> 完整步骤 <<<\n\n";
    if (result.stepDetails.empty()) {
        cout << "    (同线路直达)\n";
    } else {
        for (size_t i = 0; i < result.stepDetails.size(); i++) {
            cout << "  " << (i + 1) << ". " << result.stepDetails[i] << "\n";
        }
    }
    cout << "\n  " << string(48, '=') << "\n";
}

// ========== 生成路径摘要 ==========
string Path::pathSummary(const PathResult& result) {
    if (!result.valid()) return "[无可达路径]";

    Station* first = graph->getStation(result.stationIds.front());
    Station* last  = graph->getStation(result.stationIds.back());

    string summary = (first ? first->name : "?") + " -> " + (last ? last->name : "?");
    summary += " | 耗时" + to_string(result.totalTime) + "分钟";
    summary += " | 换乘" + to_string(result.transferCount) + "次";
    summary += " | 共" + to_string(result.stationIds.size()) + "站";
    return summary;
}

// ========== 检查路径有效性 ==========
bool Path::isValidPath(const PathResult& result) {
    if (!result.valid()) return false;
    if (result.stationIds.size() < 2) return true; // 起终点相同也算合法

    for (size_t i = 0; i + 1 < result.stationIds.size(); i++) {
        Station* from = graph->getStation(result.stationIds[i]);
        Station* to   = graph->getStation(result.stationIds[i + 1]);
        if (!from || !to) return false;
        if (!from->open || !to->open) return false;

        // 验证边的存在性
        bool edgeExists = false;
        for (size_t j = 0; j < graph->adj[from->id].size(); j++) {
            if (graph->edges[graph->adj[from->id][j]].to == to->id) {
                edgeExists = true; break;
            }
        }
        if (!edgeExists) return false;
    }
    return true;
}
