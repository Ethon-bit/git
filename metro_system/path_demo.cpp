#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "path.h"
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

// 外部图（由成员 A 提供）
extern Graph globalGraph;

// 简易CSV行分割
vector<string> splitCSV(const string& line) {
    vector<string> fields;
    string field;
    for (size_t i = 0; i < line.length(); i++) {
        if (line[i] == ',') { fields.push_back(field); field.clear(); }
        else { field += line[i]; }
    }
    fields.push_back(field);
    return fields;
}

string trim(const string& s) {
    size_t start = 0, end = s.length();
    while (start < end && (unsigned char)s[start] <= 32) start++;
    while (end > start && (unsigned char)s[end - 1] <= 32) end--;
    return s.substr(start, end - start);
}

string utf8Trim(const string& s) {
    string r = s;
    if (r.length() >= 3 && (unsigned char)r[0] == 0xEF && (unsigned char)r[1] == 0xBB && (unsigned char)r[2] == 0xBF)
        r = r.substr(3);
    return trim(r);
}

bool loadStations(const string& path) {
    ifstream f(path.c_str());
    if (!f.is_open()) return false;
    string line;
    getline(f, line);
    while (getline(f, line)) {
        vector<string> fields = splitCSV(line);
        if (fields.size() < 4) continue;
        Station s;
        s.id = stoi(fields[0]);
        s.name = utf8Trim(fields[1]);
        s.line = utf8Trim(fields[2]);
        s.open = (utf8Trim(fields[3]) == "开启");
        globalGraph.stations.push_back(s);
        globalGraph.originOpenStatus[s.id] = s.open ? "开启" : "关闭";
    }
    return !globalGraph.stations.empty();
}

bool loadEdges(const string& path) {
    ifstream f(path.c_str());
    if (!f.is_open()) return false;
    string line;
    getline(f, line);
    while (getline(f, line)) {
        vector<string> fields = splitCSV(line);
        if (fields.size() < 5) continue;
        Edge e;
        e.from = stoi(fields[0]);
        e.to = stoi(fields[1]);
        e.type = utf8Trim(fields[2]);
        e.direction = utf8Trim(fields[3]);
        e.time = stoi(fields[4]);
        e.isTransfer = (e.type == "换乘" || e.type == "虚拟换乘" || e.type == "站内换乘");
        e.transferTime = e.isTransfer ? e.time : 0;
        globalGraph.edges.push_back(e);
        globalGraph.adj[e.from].push_back(globalGraph.edges.size() - 1);
    }
    return !globalGraph.edges.empty();
}

Graph globalGraph;

Station* chooseStation(const string& prompt, Path& path) {
    while (true) {
        cout << prompt;
        string kw;
        getline(cin, kw);
        vector<Station*> results;
        for (size_t i = 0; i < globalGraph.stations.size(); i++) {
            if (globalGraph.stations[i].name.find(kw) != string::npos)
                results.push_back(&globalGraph.stations[i]);
        }
        if (results.empty()) {
            cout << "  未找到匹配的站点,请重新输入。\n";
            continue;
        }
        if (results.size() == 1) return results[0];
        cout << "  匹配的站点:\n";
        for (size_t i = 0; i < results.size(); i++)
            cout << "    " << (i + 1) << ". " << results[i]->name << " (" << results[i]->line << ")\n";
        cout << "  请选择编号: ";
        int sel;
        cin >> sel;
        cin.ignore(1000000, '\n');
        if (sel >= 1 && sel <= (int)results.size()) return results[sel - 1];
        cout << "  输入无效。\n";
    }
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    cout << string(56, '=') << "\n";
    cout << "  成员 C: Path 路径规划模块 - 功能演示\n";
    cout << string(56, '=') << "\n\n";
    cout << "  负责功能:\n";
    cout << "    1. minTransferPath()   - 最少换乘路径规划\n";
    cout << "    2. shortestTimePath()  - 最短时间路径规划\n";
    cout << "    3. kShortestTimePaths()- K条最优路径\n";
    cout << "    4. printPath()         - 路径可视化打印\n";
    cout << "    5. pathSummary()       - 路径摘要生成\n";
    cout << "    6. isValidPath()       - 路径有效性验证\n";
    cout << "\n";

    cout << "正在加载地铁网络数据...\n";
    if (!loadStations("data/Station.csv")) {
        cout << "错误: 无法加载站点数据\n";
        return 1;
    }
    if (!loadEdges("data/Edge.csv")) {
        cout << "错误: 无法加载边数据\n";
        return 1;
    }
    cout << "  加载完成: " << globalGraph.stations.size() << " 个站点, " << globalGraph.edges.size() << " 条边\n\n";

    Path pathFinder(&globalGraph);

    // 演示1: 最短时间路径
    cout << string(56, '-') << "\n";
    cout << "  演示1: 最短时间路径规划 (人民广场 -> 浦东国际机场)\n";
    cout << string(56, '-') << "\n";
    int startId = 116, endId = 230; // 人民广场(1号线) -> 浦东1号2号航站楼(2号线)
    PathResult r1 = pathFinder.shortestTimePath(startId, endId);
    pathFinder.printPath(r1);
    cout << "  摘要: " << pathFinder.pathSummary(r1) << "\n";

    // 演示2: 最少换乘路径
    cout << "\n" << string(56, '-') << "\n";
    cout << "  演示2: 最少换乘路径规划 (富锦路 -> 迪士尼)\n";
    cout << string(56, '-') << "\n";
    PathResult r2 = pathFinder.minTransferPath(101, 1101);
    pathFinder.printPath(r2);
    cout << "  摘要: " << pathFinder.pathSummary(r2) << "\n";

    // 演示3: K条最短路径
    cout << "\n" << string(56, '-') << "\n";
    cout << "  演示3: 3条最短时间路径 (上海南站 -> 虹桥火车站)\n";
    cout << string(56, '-') << "\n";
    vector<PathResult> kpaths = pathFinder.kShortestTimePaths(124, 202, 3);
    for (size_t i = 0; i < kpaths.size(); i++) {
        cout << "\n  --- 路径 " << (i + 1) << " ---\n";
        pathFinder.printPath(kpaths[i]);
    }

    // 演示4: 路径有效性验证
    cout << "\n" << string(56, '-') << "\n";
    cout << "  演示4: 路径有效性验证\n";
    cout << string(56, '-') << "\n";
    cout << "  路径1有效性: " << (pathFinder.isValidPath(r1) ? "通过" : "不通过") << "\n";
    cout << "  路径2有效性: " << (pathFinder.isValidPath(r2) ? "通过" : "不通过") << "\n";

    cout << "\n" << string(56, '=') << "\n";
    cout << "  成员 C 模块演示完毕\n";
    cout << string(56, '=') << "\n";
    return 0;
}
