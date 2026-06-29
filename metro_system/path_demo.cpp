#include <fstream>
#include <string>
#include <queue>
#include <map>
#include <algorithm>
#include <climits>
#include "path.h"
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

// =====================================================
// 模拟：成员A提供的 Graph（通过外部数据文件加载）
// 模拟：成员B提供的 Dijkstra 实现
// 测试：成员C的 Path 模块
// =====================================================

Graph globalGraph;

// ---------- 成员B: Dijkstra ----------
vector<int> memberB_Dijkstra(Graph* g, int start, int end,
                             int& totalTime, vector<string>& details) {
    totalTime = -1;
    details.clear();

    map<int, int> dist, parent, edgeUsed;
    map<int, bool> visited;
    priority_queue<pair<int, int>, vector<pair<int, int> >,
                   greater<pair<int, int> > > pq;

    for (size_t i = 0; i < g->stations.size(); i++)
        dist[g->stations[i].id] = INT_MAX;
    dist[start] = 0;
    pq.push(make_pair(0, start));

    while (!pq.empty()) {
        int d = pq.top().first;
        int u = pq.top().second;
        pq.pop();
        if (visited.count(u) && visited[u]) continue;
        visited[u] = true;
        if (u == end) break;
        for (size_t j = 0; j < g->adj[u].size(); j++) {
            int ei = g->adj[u][j];
            Edge& e = g->edges[ei];
            int v = e.to;
            if (visited.count(v) && visited[v]) continue;
            Station* sv = g->getStation(v);
            if (!sv || !sv->open) continue;
            int nd = d + e.time;
            if (nd < dist[v]) {
                dist[v] = nd; parent[v] = u; edgeUsed[v] = ei;
                pq.push(make_pair(nd, v));
            }
        }
    }

    if (!visited.count(end) || !visited[end]) return vector<int>();
    totalTime = dist[end];

    vector<int> path;
    for (int cur = end; cur != start; cur = parent[cur]) path.push_back(cur);
    path.push_back(start);
    reverse(path.begin(), path.end());

    string prevLine = "";
    for (size_t i = 0; i < path.size(); i++) {
        Station* s = g->getStation(path[i]);
        string curLine = s ? s->line : "";
        if (i > 0 && edgeUsed.count(path[i])) {
            Edge& e = g->edges[edgeUsed[path[i]]];
            if (e.isTransfer)
                details.push_back("  [换乘] " + prevLine + " -> " + curLine
                    + " (步行" + to_string(e.time) + "分钟)");
            else {
                Station* ps = g->getStation(path[i - 1]);
                details.push_back("  " + (ps ? ps->name : "?") + " -> " + s->name
                    + "  [" + curLine + "]  " + e.direction + "  " + to_string(e.time) + "分钟");
            }
        }
        prevLine = curLine;
    }
    return path;
}

// ---------- 简易CSV解析 ----------
vector<string> splitCSV(const string& line) {
    vector<string> f; string field;
    for (size_t i = 0; i < line.length(); i++)
        if (line[i] == ',') { f.push_back(field); field.clear(); }
        else field += line[i];
    f.push_back(field);
    return f;
}

string trim(const string& s) {
    size_t a = 0, b = s.length();
    while (a < b && (unsigned char)s[a] <= 32) a++;
    while (b > a && (unsigned char)s[b - 1] <= 32) b--;
    return s.substr(a, b - a);
}

string utf8Trim(const string& s) {
    string r = s;
    if (r.length() >= 3 && (unsigned char)r[0] == 0xEF
        && (unsigned char)r[1] == 0xBB && (unsigned char)r[2] == 0xBF)
        r = r.substr(3);
    return trim(r);
}

bool loadData(const string& stationFile, const string& edgeFile) {
    ifstream fs(stationFile.c_str());
    if (!fs.is_open()) return false;
    string line; getline(fs, line);
    while (getline(fs, line)) {
        vector<string> f = splitCSV(line);
        if (f.size() < 4) continue;
        Station s = { stoi(f[0]), utf8Trim(f[1]), utf8Trim(f[2]),
                      utf8Trim(f[3]) == "开启" };
        globalGraph.stations.push_back(s);
    }
    fs.close();

    ifstream fe(edgeFile.c_str());
    if (!fe.is_open()) return false;
    getline(fe, line);
    while (getline(fe, line)) {
        vector<string> f = splitCSV(line);
        if (f.size() < 5) continue;
        Edge e = { stoi(f[0]), stoi(f[1]), utf8Trim(f[2]),
                   utf8Trim(f[3]), stoi(f[4]), false };
        e.isTransfer = (e.type == "换乘" || e.type == "虚拟换乘" || e.type == "站内换乘");
        globalGraph.edges.push_back(e);
        globalGraph.adj[e.from].push_back(globalGraph.edges.size() - 1);
    }
    fe.close();
    return true;
}

// ========== 主程序 ==========
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    cout << string(56, '=') << "\n"
         << "  成员 C: Path 路径规划模块 — 独立演示\n"
         << string(56, '=') << "\n\n"
         << "  模块分工:\n"
         << "    成员A → Graph/Station 数据层\n"
         << "    成员B → Dijkstra 最短时间路径算法\n"
         << "    成员C → Path 路径模块 (本演示)\n"
         << "\n  成员C负责:\n"
         << "    1. minTransferPath()   最少换乘路径\n"
         << "    2. kShortestTimePaths() K条最优路径\n"
         << "    3. printPath()         美观输出\n"
         << "    4. pathSummary()       路径摘要\n"
         << "    5. isValidPath()       有效性验证\n\n";

    if (!loadData("data/Station.csv", "data/Edge.csv")) {
        cout << "错误: 数据加载失败\n"; return 1;
    }
    cout << "  数据: " << globalGraph.stations.size() << "站, "
         << globalGraph.edges.size() << "边\n\n";

    // 创建Path实例，注入B的Dijkstra
    Path pathFinder(&globalGraph);
    pathFinder.setDijkstra(memberB_Dijkstra);

    // ---- 演示1: 最少换乘路径 ----
    cout << string(56, '-') << "\n"
         << "  演示1: 最少换乘路径 (上海南站 → 虹桥火车站)\n"
         << string(56, '-') << "\n";
    PathResult r1 = pathFinder.minTransferPath(124, 202);
    pathFinder.printPath(r1);
    cout << "  摘要: " << pathFinder.pathSummary(r1) << "\n";

    // ---- 演示2: K条最短时间路径 ----
    cout << "\n" << string(56, '-') << "\n"
         << "  演示2: 3条最短时间路径 (莘庄 → 人民广场)\n"
         << string(56, '-') << "\n";
    vector<PathResult> ks = pathFinder.kShortestTimePaths(128, 116, 3);
    for (size_t i = 0; i < ks.size(); i++) {
        cout << "\n  ------- 第 " << (i + 1) << " 条 -------\n";
        pathFinder.printPath(ks[i]);
    }

    // ---- 演示3: 验证 ----
    cout << "\n" << string(56, '-') << "\n"
         << "  演示3: 路径有效性验证\n"
         << string(56, '-') << "\n";
    cout << "  路径1: " << (pathFinder.isValidPath(r1) ? "✓" : "✗") << "\n";
    if (ks.size() > 0)
        cout << "  K路径1: " << (pathFinder.isValidPath(ks[0]) ? "✓" : "✗") << "\n";

    cout << "\n" << string(56, '=') << "\n"
         << "  成员 C 模块全部功能验证完毕\n"
         << string(56, '=') << "\n";
    return 0;
}
