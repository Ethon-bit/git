#include "path.h"

// ========== Dijkstra 最短时间路径 ==========
vector<int> Path::dijkstra(int start, int end, int& totalTime, vector<string>& details) {
    totalTime = -1;
    details.clear();

    map<int, int> dist, parent, edgeUsed;
    set<int> visited;
    priority_queue<pair<int, int>, vector<pair<int, int> >, greater<pair<int, int> > > pq;

    for (size_t i = 0; i < graph->stations.size(); i++)
        dist[graph->stations[i].id] = INT_MAX;
    dist[start] = 0;
    pq.push(make_pair(0, start));

    while (!pq.empty()) {
        int d = pq.top().first;
        int u = pq.top().second;
        pq.pop();
        if (visited.count(u)) continue;
        visited.insert(u);
        if (u == end) break;

        for (size_t j = 0; j < graph->adj[u].size(); j++) {
            int ei = graph->adj[u][j];
            Edge& e = graph->edges[ei];
            int v = e.to;
            if (visited.count(v)) continue;
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

    // 生成步骤详情
    string prevLine = "";
    for (size_t i = 0; i < path.size(); i++) {
        Station* s = graph->getStation(path[i]);
        string curLine = s ? s->line : "";
        if (i > 0 && edgeUsed.count(path[i])) {
            Edge& e = graph->edges[edgeUsed[path[i]]];
            if (e.isTransfer) {
                details.push_back("  |-- [换乘] " + prevLine + " -> " + curLine + " (步行" + to_string(e.time) + "分钟)");
            } else {
                Station* ps = graph->getStation(path[i - 1]);
                details.push_back("  |-- " + (ps ? ps->name : "?") + " -> " + s->name + " [" + curLine + "] " + e.direction + " (" + to_string(e.time) + "分钟)");
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
        return result;
    }

    map<int, int> dist, trans, parent, edgeUsed;
    set<int> visited;
    priority_queue<pair<int, int>, vector<pair<int, int> >, greater<pair<int, int> > > pq;

    for (size_t i = 0; i < graph->stations.size(); i++) {
        dist[graph->stations[i].id] = INT_MAX;
        trans[graph->stations[i].id] = INT_MAX;
    }
    dist[startId] = 0;
    trans[startId] = 0;
    pq.push(make_pair(0, startId));

    while (!pq.empty()) {
        int priority = pq.top().first;
        int u = pq.top().second;
        pq.pop();

        // priority = trans[u] * 100000 + dist[u]
        int currentTrans = priority / 100000;
        int currentDist = priority % 100000;

        if (visited.count(u)) continue;
        visited.insert(u);
        if (u == endId) break;

        for (size_t j = 0; j < graph->adj[u].size(); j++) {
            int ei = graph->adj[u][j];
            Edge& e = graph->edges[ei];
            int v = e.to;
            if (visited.count(v)) continue;
            Station* sv = graph->getStation(v);
            if (!sv || !sv->open) continue;

            int newTrans = currentTrans + (e.isTransfer ? 1 : 0);
            int newDist = currentDist + e.time;

            if (newTrans < trans[v] || (newTrans == trans[v] && newDist < dist[v])) {
                trans[v] = newTrans;
                dist[v] = newDist;
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
                    "  |-- [换乘: " + prevLine + " -> " + curLine + "] (步行" + to_string(e.time) + "分钟)");
            } else {
                Station* ps = graph->getStation(path[i - 1]);
                result.stepDetails.push_back(
                    "  |-- " + (ps ? ps->name : "?") + " -> " + s->name + " [" + curLine + "] " + e.direction + " (" + to_string(e.time) + "分钟)");
            }
        }
        prevLine = curLine;
    }
    return result;
}

// ========== K条最短时间路径（Yen's算法） ==========
vector<PathResult> Path::kShortestTimePaths(int startId, int endId, int K) {
    vector<PathResult> results;
    if (K <= 0) return results;

    // 第一条：直接用Dijkstra
    PathResult first = shortestTimePath(startId, endId);
    if (!first.valid()) return results;
    results.push_back(first);

    // Yen's 算法主体
    vector<vector<int> > candidatePaths;
    vector<int> candidateTimes;

    for (int k = 1; k < K; k++) {
        vector<int>& lastPath = results[k - 1].stationIds;

        for (size_t i = 0; i < lastPath.size() - 1; i++) {
            int spurNode = lastPath[i];
            vector<int> rootPath(lastPath.begin(), lastPath.begin() + i);

            // 在当前已找到的路径中，如果与当前rootPath匹配，则"封锁"下一条边
            // 简化版：跳过已存在的重复路径
            int spurTime;
            vector<string> spurDetail;
            vector<int> spurPath = dijkstra(spurNode, endId, spurTime, spurDetail);

            if (!spurPath.empty() && spurPath.size() > 1) {
                vector<int> totalPath = rootPath;
                totalPath.insert(totalPath.end(), spurPath.begin() + 1, spurPath.end());

                // 去重
                bool duplicate = false;
                for (size_t j = 0; j < results.size(); j++) {
                    if (results[j].stationIds == totalPath) { duplicate = true; break; }
                }
                for (size_t j = 0; j < candidatePaths.size(); j++) {
                    if (candidatePaths[j] == totalPath) { duplicate = true; break; }
                }

                if (!duplicate) {
                    int rootTime = 0;
                    for (size_t j = 0; j + 1 < rootPath.size(); j++) {
                        for (size_t m = 0; m < graph->adj[rootPath[j]].size(); m++) {
                            if (graph->edges[graph->adj[rootPath[j]][m]].to == rootPath[j + 1]) {
                                rootTime += graph->edges[graph->adj[rootPath[j]][m]].time;
                                break;
                            }
                        }
                    }
                    candidatePaths.push_back(totalPath);
                    candidateTimes.push_back(rootTime + spurTime);
                }
            }
        }

        if (candidatePaths.empty()) break;

        // 选最优候选
        int bestIdx = 0, bestTime = INT_MAX;
        for (size_t i = 0; i < candidatePaths.size(); i++) {
            if (candidateTimes[i] < bestTime) {
                bestTime = candidateTimes[i];
                bestIdx = (int)i;
            }
        }

        PathResult pr;
        pr.stationIds = candidatePaths[bestIdx];
        pr.totalTime = candidateTimes[bestIdx];
        pr.transferCount = 0;
        for (size_t i = 0; i < pr.stationIds.size(); i++) {
            // 简化：用名称生成步骤
            Station* s = graph->getStation(pr.stationIds[i]);
            if (i > 0) {
                Station* prev = graph->getStation(pr.stationIds[i - 1]);
                pr.stepDetails.push_back("  |-- " + (prev ? prev->name : "?") + " -> " + (s ? s->name : "?") + " [" + (s ? s->line : "?") + "]");
            }
        }
        results.push_back(pr);

        // 移除已选的候选
        candidatePaths.erase(candidatePaths.begin() + bestIdx);
        candidateTimes.erase(candidateTimes.begin() + bestIdx);
    }
    return results;
}

// ========== 打印路径 ==========
void Path::printPath(const PathResult& result) {
    if (!result.valid()) {
        cout << "\n" << string(50, '=') << "\n";
        cout << "  无可达路径\n";
        cout << "  可能原因:\n";
        cout << "    1. 起点或终点已关闭\n";
        cout << "    2. 网络中存在不连通区域\n";
        cout << "    3. 输入站点不存在\n";
        cout << string(50, '=') << "\n";
        return;
    }

    Station* first = graph->getStation(result.stationIds.front());
    Station* last = graph->getStation(result.stationIds.back());

    cout << "\n" << string(50, '=') << "\n";
    cout << "  地铁路径规划结果\n";
    cout << string(50, '=') << "\n";
    cout << left << setw(14) << "  起点站:" << (first ? first->name : "?") << " (" << (first ? first->line : "") << ")\n";
    cout << left << setw(14) << "  终点站:" << (last ? last->name : "?") << " (" << (last ? last->line : "") << ")\n";
    cout << string(50, '-') << "\n";
    cout << left << setw(14) << "  总耗时:" << result.totalTime << " 分钟\n";
    cout << left << setw(14) << "  换乘次数:" << result.transferCount << " 次\n";
    cout << left << setw(14) << "  途径站点:" << result.stationIds.size() << " 站\n";
    cout << string(50, '-') << "\n";
    cout << "  详细路径:\n";

    string currentLine = "";
    int lineStationCount = 0;
    for (size_t i = 0; i < result.stationIds.size(); i++) {
        Station* s = graph->getStation(result.stationIds[i]);
        if (!s) continue;

        if (s->line != currentLine) {
            if (!currentLine.empty()) {
                cout << "    (" << lineStationCount << "站)\n\n";
            }
            currentLine = s->line;
            lineStationCount = 0;
            cout << "  [" << currentLine << "]\n";
        }

        cout << "    " << setw(3) << (++lineStationCount) << ". " << setw(18) << s->name;
        if (!s->open) cout << " [已关闭]";
        cout << "\n";
    }
    if (!currentLine.empty()) {
        cout << "    (" << lineStationCount << "站)\n";
    }

    cout << string(50, '-') << "\n";
    cout << "  步骤详情:\n";
    for (size_t i = 0; i < result.stepDetails.size(); i++) {
        cout << result.stepDetails[i] << "\n";
    }
    cout << string(50, '=') << "\n";
}

// ========== 生成路径摘要 ==========
string Path::pathSummary(const PathResult& result) {
    if (!result.valid()) return "无可达路径";

    Station* first = graph->getStation(result.stationIds.front());
    Station* last = graph->getStation(result.stationIds.back());

    string summary = (first ? first->name : "?") + " -> " + (last ? last->name : "?");
    summary += " | " + to_string(result.totalTime) + "分钟";
    summary += " | 换乘" + to_string(result.transferCount) + "次";
    summary += " | " + to_string(result.stationIds.size()) + "站";
    return summary;
}

// ========== 检查路径有效性 ==========
bool Path::isValidPath(const PathResult& result) {
    if (!result.valid()) return false;
    if (result.stationIds.size() < 2) return false;

    for (size_t i = 0; i + 1 < result.stationIds.size(); i++) {
        Station* from = graph->getStation(result.stationIds[i]);
        Station* to = graph->getStation(result.stationIds[i + 1]);
        if (!from || !to) return false;
        if (!from->open || !to->open) return false;
    }
    return true;
}
