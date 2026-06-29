#ifndef PATH_H
#define PATH_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <iomanip>
using namespace std;

// 来自成员A的 Graph 和 Station —— 直接引用，不重复定义
struct Station { int id; string name; string line; bool open; };
struct Edge   { int from, to; string type, direction; int time; bool isTransfer; };
struct Graph  {
    vector<Station> stations;
    vector<Edge> edges;
    map<int, vector<int> > adj;

    Station* getStation(int id) {
        for (size_t i = 0; i < stations.size(); i++)
            if (stations[i].id == id) return &stations[i];
        return NULL;
    }
};

// ================================================
// 成员 C —— 路径规划模块
// 依赖: 成员A的Graph / 成员B的Dijkstra
// ================================================

struct PathResult {
    vector<int>    stationIds;
    int            totalTime;
    int            transferCount;
    vector<string> stepDetails;

    PathResult() : totalTime(-1), transferCount(-1) {}
    bool valid() const { return !stationIds.empty(); }
};

// 成员B提供的接口（Dijkstra最短时间路径）
// 返回按顺序排列的站点ID列表
typedef vector<int> (*DijkstraFn)(Graph*, int start, int end,
                                  int& totalTime, vector<string>& details);

class Path {
public:
    Path(Graph* g) : graph(g), dijkstraFn(NULL) {}

    // 注入成员B的Dijkstra实现（由调用方注册）
    void setDijkstra(DijkstraFn fn) { dijkstraFn = fn; }

    // -------- 成员C核心功能 --------

    // 最少换乘路径规划
    PathResult minTransferPath(int startId, int endId);

    // K条最短时间路径（Yen's算法，基于成员B的Dijkstra）
    vector<PathResult> kShortestTimePaths(int startId, int endId, int K);

    // 打印路径（美观输出）
    void printPath(const PathResult& result);

    // 路径摘要
    string pathSummary(const PathResult& result);

    // 路径有效性验证
    bool isValidPath(const PathResult& result);

private:
    Graph*      graph;
    DijkstraFn  dijkstraFn;

    // 内部辅助
    vector<int> callDijkstra(int start, int end, int& time, vector<string>& detail);
    string padTo(const string& s, int w);
    int charWidth(const string& s);
};

#endif
