#ifndef PATH_H
#define PATH_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <algorithm>
#include <climits>
#include <iomanip>
using namespace std;

// 前向声明（依赖成员 A 的 Graph/Station）
struct Station {
    int id;
    string name;
    string line;
    bool open;
};

struct Edge {
    int from;
    int to;
    string type;
    string direction;
    int time;
    int transferTime;
    bool isTransfer;
};

struct Graph {
    vector<Station> stations;
    vector<Edge> edges;
    map<int, vector<int> > adj;
    map<int, string> originOpenStatus;

    Station* getStation(int id) {
        for (size_t i = 0; i < stations.size(); i++)
            if (stations[i].id == id) return &stations[i];
        return NULL;
    }

    bool isReachable(int start, int end) {
        set<int> visited;
        vector<int> stack;
        stack.push_back(start);
        while (!stack.empty()) {
            int u = stack.back(); stack.pop_back();
            if (visited.count(u)) continue;
            visited.insert(u);
            if (u == end) return true;
            for (size_t j = 0; j < adj[u].size(); j++) {
                int v = edges[adj[u][j]].to;
                Station* sv = getStation(v);
                if (sv && sv->open && !visited.count(v)) stack.push_back(v);
            }
        }
        return false;
    }
};

// ========== 成员 C: Path 模块 ==========

struct PathResult {
    vector<int> stationIds;           // 路径站点ID序列
    int totalTime;                    // 总耗时（分钟）
    int transferCount;                // 换乘次数
    vector<string> stepDetails;       // 每一步的详细信息

    PathResult() : totalTime(-1), transferCount(-1) {}
    bool valid() const { return !stationIds.empty(); }
};

class Path {
public:
    // 构造函数：绑定图
    Path(Graph* g) : graph(g) {}

    // 核心功能1: 最少换乘路径规划（BFS+优先队列）
    PathResult minTransferPath(int startId, int endId);

    // 核心功能2: K条最短时间路径（Yen's算法）
    vector<PathResult> kShortestTimePaths(int startId, int endId, int K);

    // 核心功能3: 单条最短时间路径（Dijkstra）
    PathResult shortestTimePath(int startId, int endId);

    // 核心功能4: 打印路径信息（控制台输出）
    void printPath(const PathResult& result);

    // 核心功能5: 生成路径摘要
    string pathSummary(const PathResult& result);

    // 辅助: 检查路径有效性
    bool isValidPath(const PathResult& result);

private:
    Graph* graph;

    // Dijkstra 内部实现
    vector<int> dijkstra(int start, int end, int& totalTime, vector<string>& details);
};

#endif
