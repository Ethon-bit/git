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
#include <limits>
#include <iomanip>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

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

typedef pair<string, string> StationKey;

struct Graph {
    vector<Station> stations;
    vector<Edge> edges;
    map<int, vector<int> > adj;
    map<int, string> originOpenStatus;

    void addStation(const Station& s) {
        stations.push_back(s);
        if (originOpenStatus.find(s.id) == originOpenStatus.end()) {
            originOpenStatus[s.id] = s.open ? "开启" : "关闭";
        }
    }

    void addEdge(const Edge& e) {
        edges.push_back(e);
        adj[e.from].push_back(edges.size() - 1);
    }

    vector<int> getDijkstraPath(int start, int end, int& totalTime, vector<string>& pathDetail) {
        totalTime = -1;
        pathDetail.clear();
        if (start == end) {
            totalTime = 0;
            return vector<int>{start};
        }

        map<int, int> dist, parent, edgeUsed;
        set<int> visited;
        priority_queue<pair<int, int>, vector<pair<int, int> >, greater<pair<int, int> > > pq;

        for (size_t i = 0; i < stations.size(); i++) dist[stations[i].id] = INT_MAX;
        dist[start] = 0;
        pq.push(make_pair(0, start));

        while (!pq.empty()) {
            int d = pq.top().first;
            int u = pq.top().second;
            pq.pop();
            if (visited.count(u)) continue;
            visited.insert(u);
            if (u == end) break;

            for (size_t j = 0; j < adj[u].size(); j++) {
                int ei = adj[u][j];
                Edge& e = edges[ei];
                int v = e.to;
                if (visited.count(v)) continue;
                Station* sv = getStation(v);
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
        for (int cur = end; cur != start; cur = parent[cur]) {
            path.push_back(cur);
        }
        path.push_back(start);
        reverse(path.begin(), path.end());

        string prevLine = "";
        for (size_t i = 0; i < path.size(); i++) {
            Station* s = getStation(path[i]);
            string curLine = s ? s->line : "";
            if (i > 0 && edgeUsed.count(path[i])) {
                Edge& e = edges[edgeUsed[path[i]]];
                if (e.isTransfer) {
                    pathDetail.push_back("  |-- 换乘: " + (prevLine.empty() ? "" : prevLine) + " -> " + curLine + " (" + to_string(e.time) + "分钟)");
                } else {
                    Station* ps = getStation(path[i - 1]);
                    pathDetail.push_back("  |-- " + (ps ? ps->name : "") + " -> " + s->name + " [" + curLine + "] " + e.direction + " (" + to_string(e.time) + "分钟)");
                }
            }
            prevLine = curLine;
        }
        return path;
    }

    vector<vector<int> > getKShortestPaths(int start, int end, int K, vector<int>& times, vector<vector<string> >& details) {
        vector<vector<int> > result;
        times.clear(); details.clear();
        if (K <= 0) return result;

        int t;
        vector<string> d;
        vector<int> p0 = getDijkstraPath(start, end, t, d);
        if (p0.empty()) return result;
        result.push_back(p0);
        times.push_back(t);
        details.push_back(d);

        vector<vector<int> > candidates;

        for (int k = 1; k < K; k++) {
            vector<int>& lastPath = result[k - 1];
            for (size_t i = 0; i < lastPath.size() - 1; i++) {
                int spurNode = lastPath[i];
                vector<int> rootPath(lastPath.begin(), lastPath.begin() + i);

                for (size_t p = 0; p < result.size(); p++) {
                    if (result[p].size() > i) {
                        bool match = true;
                        for (size_t j = 0; j <= i && j < result[p].size(); j++) {
                            if (result[p][j] != lastPath[j]) { match = false; break; }
                        }
                        if (match) {
                            /* block this edge for this iteration */
                        }
                    }
                }

                vector<int> spurPath;
                int spurTime;
                vector<string> spurDetail;
                spurPath = getDijkstraPath(spurNode, end, spurTime, spurDetail);
                if (!spurPath.empty() && spurPath.size() > 1) {
                    vector<int> totalPath = rootPath;
                    totalPath.insert(totalPath.end(), spurPath.begin() + 1, spurPath.end());

                    bool duplicate = false;
                    for (size_t jj = 0; jj < result.size(); jj++) {
                        if (result[jj] == totalPath) { duplicate = true; break; }
                    }
                    for (size_t jj = 0; jj < candidates.size(); jj++) {
                        if (candidates[jj] == totalPath) { duplicate = true; break; }
                    }

                    if (!duplicate) {
                        candidates.push_back(totalPath);
                        int rootTime = 0;
                        for (size_t j = 0; j + 1 < rootPath.size(); j++) {
                            for (size_t m = 0; m < adj[rootPath[j]].size(); m++) {
                                if (edges[adj[rootPath[j]][m]].to == rootPath[j + 1]) {
                                    rootTime += edges[adj[rootPath[j]][m]].time; break;
                                }
                            }
                        }
                        times.push_back(rootTime + spurTime);
                    }
                }
            }
            if (candidates.empty()) break;
            int bestIdx = 0, bestTime = INT_MAX;
            size_t offset = times.size() - candidates.size();
            for (size_t i = 0; i < candidates.size(); i++) {
                if (times[offset + i] < bestTime) {
                    bestTime = times[offset + i];
                    bestIdx = (int)i;
                }
            }
            result.push_back(candidates[bestIdx]);
        }
        return result;
    }

    vector<int> getLeastTransferPath(int start, int end, int& transfers, int& totalTime, vector<string>& pathDetail) {
        transfers = -1; totalTime = -1;
        pathDetail.clear();
        if (start == end) { transfers = 0; totalTime = 0; return vector<int>{start}; }

        map<int, int> dist, trans, parent, edgeUsed;
        set<int> visited;
        priority_queue<pair<int, int>, vector<pair<int, int> >, greater<pair<int, int> > > pq;

        for (size_t i = 0; i < stations.size(); i++) {
            dist[stations[i].id] = INT_MAX;
            trans[stations[i].id] = INT_MAX;
        }
        dist[start] = 0; trans[start] = 0;
        pq.push(make_pair(0, start));

        while (!pq.empty()) {
            int d = pq.top().first;
            int u = pq.top().second;
            pq.pop();
            if (visited.count(u)) continue;
            visited.insert(u);
            if (u == end) break;

            for (size_t j = 0; j < adj[u].size(); j++) {
                int ei = adj[u][j];
                Edge& e = edges[ei];
                int v = e.to;
                if (visited.count(v)) continue;
                Station* sv = getStation(v);
                if (!sv || !sv->open) continue;

                int newTrans = trans[u] + (e.isTransfer ? 1 : 0);
                int newDist = d + e.time;

                if (newTrans < trans[v] || (newTrans == trans[v] && newDist < dist[v])) {
                    trans[v] = newTrans;
                    dist[v] = newDist;
                    parent[v] = u;
                    edgeUsed[v] = ei;
                    pq.push(make_pair(newTrans * 100000 + newDist, v));
                }
            }
        }

        if (trans[end] == INT_MAX) return vector<int>();
        transfers = trans[end];
        totalTime = dist[end];

        vector<int> path;
        for (int cur = end; cur != start; cur = parent[cur]) path.push_back(cur);
        path.push_back(start);
        reverse(path.begin(), path.end());

        string prevLine = "";
        for (size_t i = 0; i < path.size(); i++) {
            Station* s = getStation(path[i]);
            string curLine = s ? s->line : "";
            if (i > 0 && edgeUsed.count(path[i])) {
                Edge& e = edges[edgeUsed[path[i]]];
                if (e.isTransfer) {
                    pathDetail.push_back("  |-- 换乘: " + prevLine + " -> " + curLine + " (步行" + to_string(e.time) + "分钟)");
                } else {
                    Station* ps = getStation(path[i - 1]);
                    pathDetail.push_back("  |-- " + (ps ? ps->name : "?") + " -> " + s->name + " [" + curLine + "] " + e.direction + " (" + to_string(e.time) + "分钟)");
                }
            }
            prevLine = curLine;
        }
        return path;
    }

    Station* getStation(int id) {
        for (size_t i = 0; i < stations.size(); i++)
            if (stations[i].id == id) return &stations[i];
        return NULL;
    }

    vector<Station*> searchStations(const string& keyword) {
        vector<Station*> result;
        for (size_t i = 0; i < stations.size(); i++) {
            if (stations[i].name.find(keyword) != string::npos)
                result.push_back(&stations[i]);
        }
        return result;
    }

    vector<Station*> getStationsByLine(const string& line) {
        vector<Station*> result;
        for (size_t i = 0; i < stations.size(); i++)
            if (stations[i].line == line) result.push_back(&stations[i]);
        sort(result.begin(), result.end(), StationIdLess());
        return result;
    }

    struct StationIdLess {
        bool operator()(Station* a, Station* b) const { return a->id < b->id; }
    };

    set<string> getAllLines() {
        set<string> lines;
        for (size_t i = 0; i < stations.size(); i++) lines.insert(stations[i].line);
        return lines;
    }

    set<string> getTransferLines(int stationId) {
        string name;
        for (size_t i = 0; i < stations.size(); i++)
            if (stations[i].id == stationId) { name = stations[i].name; break; }
        set<string> lines;
        if (name.empty()) return lines;
        for (size_t i = 0; i < stations.size(); i++)
            if (stations[i].name == name) lines.insert(stations[i].line);
        return lines;
    }

    vector<Station*> getClosedStations() {
        vector<Station*> result;
        for (size_t i = 0; i < stations.size(); i++)
            if (!stations[i].open) result.push_back(&stations[i]);
        return result;
    }

    void restoreAllStations() {
        for (size_t i = 0; i < stations.size(); i++) {
            if (originOpenStatus.count(stations[i].id))
                stations[i].open = (originOpenStatus[stations[i].id] == "开启");
        }
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

    set<int> getAffectedArea(int closedStationId) {
        set<int> isolated;
        Station* cs = getStation(closedStationId);
        if (!cs) return isolated;

        int anyOpen = -1;
        for (size_t i = 0; i < stations.size(); i++) {
            if (stations[i].open && stations[i].id != closedStationId) {
                anyOpen = stations[i].id; break;
            }
        }
        if (anyOpen < 0) return isolated;

        set<int> allConnected;
        queue<int> q;
        q.push(anyOpen);
        allConnected.insert(anyOpen);
        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (size_t j = 0; j < adj[u].size(); j++) {
                int v = edges[adj[u][j]].to;
                Station* sv = getStation(v);
                if (sv && sv->open && !allConnected.count(v) && v != closedStationId) {
                    allConnected.insert(v);
                    q.push(v);
                }
            }
        }

        for (size_t i = 0; i < stations.size(); i++) {
            if (stations[i].open && stations[i].id != closedStationId && !allConnected.count(stations[i].id))
                isolated.insert(stations[i].id);
        }
        return isolated;
    }
};

Graph globalGraph;
string stationCsvPath = "data/Station.csv";
string edgeCsvPath = "data/Edge.csv";

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
    if (r.length() >= 3 && (unsigned char)r[0] == 0xEF && (unsigned char)r[1] == 0xBB && (unsigned char)r[2] == 0xBF) {
        r = r.substr(3);
    }
    return trim(r);
}

bool loadStations(const string& filepath) {
    ifstream file(filepath.c_str());
    if (!file.is_open()) return false;
    string line;
    getline(file, line); // skip header
    while (getline(file, line)) {
        vector<string> fields = splitCSV(line);
        if (fields.size() < 4) continue;
        Station s;
        s.id = stoi(fields[0]);
        s.name = utf8Trim(fields[1]);
        s.line = utf8Trim(fields[2]);
        s.open = (utf8Trim(fields[3]) == "开启");
        globalGraph.addStation(s);
    }
    return !globalGraph.stations.empty();
}

bool loadEdges(const string& filepath) {
    ifstream file(filepath.c_str());
    if (!file.is_open()) return false;
    string line;
    getline(file, line); // skip header
    while (getline(file, line)) {
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
        globalGraph.addEdge(e);
    }
    return !globalGraph.edges.empty();
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    cout << "\033[2J\033[1;1H";
#endif
}

void pressAnyKey() {
    cout << "\n按回车键继续...";
    cin.ignore(1000000, '\n');
}

int readInt(const string& prompt) {
    cout << prompt;
    string input;
    getline(cin, input);
    input = trim(input);
    if (input.empty()) return INT_MIN;
    try {
        size_t pos;
        int val = stoi(input, &pos);
        if (pos != input.length()) return INT_MIN;
        return val;
    } catch (...) {
        return INT_MIN;
    }
}

Station* chooseStation(const string& prompt, bool allowClosed = false) {
    while (true) {
        cout << prompt;
        string kw;
        getline(cin, kw);
        vector<Station*> results = globalGraph.searchStations(kw);
        if (results.empty()) {
            cout << "未找到匹配的站点,请重新选择。\n";
            continue;
        }

        if (results.size() == 1) {
            if (!allowClosed && !results[0]->open) {
                cout << "站点 " << results[0]->name << "（" << results[0]->line << "）已关闭，无法进行路径规划。\n";
                return NULL;
            }
            return results[0];
        }

        cout << "匹配的站点如下：\n";
        for (size_t i = 0; i < results.size(); i++) {
            cout << "  " << (i + 1) << ". " << results[i]->name << "（" << results[i]->line << "）" << (results[i]->open ? "" : " [关闭]") << "\n";
        }
        int sel = readInt("请输入对应编号选择站点: ");
        if (sel < 1 || sel > (int)results.size()) {
            cout << "输入无效，请重新选择。\n"; continue;
        }
        Station* chosen = results[sel - 1];
        if (!allowClosed && !chosen->open) {
            cout << "站点 " << chosen->name << "（" << chosen->line << "）已关闭，无法进行路径规划。\n";
            return NULL;
        }
        return chosen;
    }
}

void showPath(const vector<int>& path, int totalTime, const vector<string>& detail, int transfers = -1) {
    if (path.empty()) { cout << "无可达路径。\n"; return; }
    Station* first = globalGraph.getStation(path.front());
    Station* last = globalGraph.getStation(path.back());
    cout << "\n起点: " << (first ? first->name : "?") << "（" << (first ? first->line : "") << "）\n";
    cout << "终点: " << (last ? last->name : "?") << "（" << (last ? last->line : "") << "）\n";
    cout << "总时间: " << totalTime << " 分钟\n";
    if (transfers >= 0) cout << "换乘次数: " << transfers << " 次\n";
    cout << "途径站点数: " << path.size() << "\n";
    cout << "\n路径详情:\n";
    for (size_t i = 0; i < detail.size(); i++) cout << detail[i] << "\n";
}

void menuStationManagement() {
    while (true) {
        clearScreen();
        cout << "============================================\n";
        cout << "     线路站点信息/运营状态管理\n";
        cout << "============================================\n";
        cout << "  1. 从CSV文件批量更新站点开启/关闭状态\n";
        cout << "  2. 手工更新站点开启/关闭状态\n";
        cout << "  3. 显示当前关闭站点\n";
        cout << "  4. 恢复所有站点初始状态\n";
        cout << "  5. 显示线路站点信息\n";
        cout << "  6. 返回上级菜单\n";
        cout << "============================================\n";

        int opt = readInt("请输入选项编号: ");
        if (opt < 1 || opt > 6) {
            cout << "输入无效，请输入数字选项1、2、3、4、5、6\n"; pressAnyKey(); continue;
        }

        switch (opt) {
        case 1: {
            string path;
            cout << "请输入更新文件路径（直接回车使用默认: data/update_station_status.csv）: ";
            getline(cin, path);
            if (path.empty()) path = "data/update_station_status.csv";

            ifstream file(path.c_str());
            if (!file.is_open()) { cout << "更新文件不存在。\n"; pressAnyKey(); break; }

            string line;
            getline(file, line);
            int updated = 0, skipped = 0;
            vector<string> allLines;
            while (getline(file, line)) {
                string t = trim(line);
                if (!t.empty()) allLines.push_back(t);
            }
            file.close();

            if (allLines.empty()) { cout << "未检测到有效更新记录。\n"; pressAnyKey(); break; }

            map<pair<string, string>, pair<string, int> > lastRecord;
            int lineNum = 0;
            for (size_t i = 0; i < allLines.size(); i++) {
                lineNum++;
                vector<string> f = splitCSV(allLines[i]);
                if (f.size() < 3) {
                    cout << "第" << lineNum << "行格式错误，已跳过。\n";
                    skipped++; continue;
                }
                string sname = utf8Trim(f[0]), sline = utf8Trim(f[1]), status = utf8Trim(f[2]);
                if (status != "开启" && status != "关闭") {
                    cout << "第" << lineNum << "行状态值非法（必须为\"开启\"或\"关闭\"），已跳过。\n";
                    skipped++; continue;
                }
                lastRecord[make_pair(sname, sline)] = make_pair(status, lineNum);
            }

            for (map<pair<string, string>, pair<string, int> >::iterator it = lastRecord.begin(); it != lastRecord.end(); ++it) {
                string sname = it->first.first;
                string sline = it->first.second;
                string status = it->second.first;
                bool found = false;
                for (size_t i = 0; i < globalGraph.stations.size(); i++) {
                    Station& st = globalGraph.stations[i];
                    if (st.name == sname && st.line == sline) {
                        st.open = (status == "开启");
                        found = true; updated++; break;
                    }
                }
                if (!found) {
                    cout << "未匹配到对应站点 \"" << sname << "\"(" << sline << ")，已跳过。\n";
                    skipped++;
                }
            }
            cout << "\n批量更新完成。成功更新 " << updated << " 个站点，跳过 " << skipped << " 条记录。\n";
            pressAnyKey();
            break;
        }
        case 2: {
            while (true) {
                cout << "请输入待修改站点关键词（exit退出）: ";
                string kw;
                getline(cin, kw);
                if (kw == "exit") break;
                vector<Station*> results = globalGraph.searchStations(kw);
                if (results.empty()) {
                    cout << "未找到匹配的站点。\n"; continue;
                }
                cout << "匹配的站点如下：\n";
                for (size_t i = 0; i < results.size(); i++) {
                    cout << "  " << (i + 1) << ". " << results[i]->name << "（" << results[i]->line << "） 状态:" << (results[i]->open ? "开启" : "关闭") << "\n";
                }
                int sel = readInt("请输入对应编号选择站点: ");
                if (sel < 1 || sel > (int)results.size()) {
                    cout << "输入无效，请重新选择。\n"; continue;
                }
                Station* chosen = results[sel - 1];
                cout << chosen->name << "," << chosen->line << "," << (chosen->open ? "开启" : "关闭") << "\n";
                cout << "请输入站点状态（开启/关闭）> ";
                string newStatus;
                getline(cin, newStatus);
                newStatus = trim(newStatus);
                if (newStatus == "开启" || newStatus == "关闭") {
                    chosen->open = (newStatus == "开启");
                    cout << "修改站点: " << chosen->name << " (" << chosen->line << ") -> 状态: " << newStatus << "\n";
                    cout << "1个站点的状态修改完成。\n";
                } else {
                    cout << "非法状态值，请输入\"开启\"或\"关闭\"。\n";
                }
            }
            break;
        }
        case 3: {
            vector<Station*> closed = globalGraph.getClosedStations();
            if (closed.empty()) {
                cout << "所有站点均处于开放状态。\n";
            } else {
                cout << "当前关闭的站点（共" << closed.size() << "个）：\n";
                for (size_t i = 0; i < closed.size(); i++) {
                    cout << "  - " << closed[i]->name << "（" << closed[i]->line << "）\n";
                }
            }
            pressAnyKey(); break;
        }
        case 4: {
            cout << "您确定要恢复所有站点的初始状态?（Y/N）: ";
            string confirm;
            getline(cin, confirm);
            if (confirm == "Y" || confirm == "y") {
                globalGraph.restoreAllStations();
                cout << "已成功恢复所有站点至初始状态。\n";
            } else {
                cout << "已取消恢复操作。\n";
            }
            pressAnyKey(); break;
        }
        case 5: {
            set<string> linesSet = globalGraph.getAllLines();
            vector<string> lineVec(linesSet.begin(), linesSet.end());
            sort(lineVec.begin(), lineVec.end());
            cout << "线路列表：\n";
            for (size_t i = 0; i < lineVec.size(); i++) {
                cout << "  " << (i + 1) << ". " << lineVec[i] << "\n";
            }
            int sel = readInt("请输入编号选择线路: ");
            if (sel < 1 || sel > (int)lineVec.size()) {
                cout << "线路编号无效。\n"; pressAnyKey(); break;
            }
            string chosenLine = lineVec[sel - 1];
            vector<Station*> stats = globalGraph.getStationsByLine(chosenLine);
            cout << "\n线路名称: " << chosenLine << "\n";
            cout << "站点总数: " << stats.size() << "\n";
            cout << left << setw(8) << "序号" << setw(22) << "站点名称" << setw(18) << "所属线路" << setw(10) << "状态" << "换乘信息\n";
            cout << string(76, '-') << "\n";
            for (size_t i = 0; i < stats.size(); i++) {
                string tinfo;
                set<string> tlines = globalGraph.getTransferLines(stats[i]->id);
                for (set<string>::iterator ti = tlines.begin(); ti != tlines.end(); ++ti) {
                    if (*ti != stats[i]->line) tinfo += *ti + " ";
                }
                if (tinfo.empty()) tinfo = "-";
                cout << left << setw(8) << (i + 1) << setw(22) << stats[i]->name << setw(18) << stats[i]->line << setw(10) << (stats[i]->open ? "开启" : "关闭") << tinfo << "\n";
            }
            pressAnyKey(); break;
        }
        case 6: return;
        }
    }
}

void menuShortestTime() {
    while (true) {
        clearScreen();
        cout << "============================================\n";
        cout << "       所需时间最短路径规划\n";
        cout << "============================================\n";
        cout << "  1. 单条所需时间最短路径\n";
        cout << "  2. 3条所需时间最短路径\n";
        cout << "  3. 返回上级菜单\n";
        cout << "============================================\n";

        int opt = readInt("请输入选项编号: ");
        if (opt < 1 || opt > 3) {
            cout << "输入无效，请输入数字选项1、2、3\n"; pressAnyKey(); continue;
        }

        if (opt == 3) return;

        Station* start = chooseStation("请输入起点站关键词: ");
        if (!start) { pressAnyKey(); continue; }
        if (!start->open) { cout << "起点已关闭，无法进行路径规划。\n"; pressAnyKey(); continue; }

        Station* end = chooseStation("请输入终点站关键词: ");
        if (!end) { pressAnyKey(); continue; }
        if (!end->open) { cout << "终点已关闭，无法进行路径规划。\n"; pressAnyKey(); continue; }

        if (start->id == end->id) {
            cout << "起点和终点相同，无需进行路径规划。\n"; pressAnyKey(); continue;
        }

        if (!globalGraph.isReachable(start->id, end->id)) {
            cout << "无可达路径（可能由于站点关闭导致网络断裂）。\n"; pressAnyKey(); continue;
        }

        if (opt == 1) {
            int totalTime;
            vector<string> detail;
            vector<int> path = globalGraph.getDijkstraPath(start->id, end->id, totalTime, detail);
            showPath(path, totalTime, detail);
        } else {
            vector<int> times;
            vector<vector<string> > details;
            vector<vector<int> > paths = globalGraph.getKShortestPaths(start->id, end->id, 3, times, details);
            if (paths.empty()) {
                cout << "无可达路径。\n";
            } else {
                for (size_t i = 0; i < paths.size(); i++) {
                    cout << "\n========== 路径" << (i + 1) << " ==========\n";
                    showPath(paths[i], i < times.size() ? times[i] : 0, i < details.size() ? details[i] : vector<string>());
                }
            }
        }
        pressAnyKey();
    }
}

void menuLeastTransfer() {
    while (true) {
        clearScreen();
        cout << "============================================\n";
        cout << "     所需换乘次数最少路径规划\n";
        cout << "============================================\n";
        cout << "  1. 单条换乘次数最少路径\n";
        cout << "  2. 3条换乘次数最少路径\n";
        cout << "  3. 返回主菜单\n";
        cout << "============================================\n";

        int opt = readInt("请输入选项编号: ");
        if (opt < 1 || opt > 3) {
            cout << "输入无效，请输入数字选项1、2、3\n"; pressAnyKey(); continue;
        }

        if (opt == 3) return;

        Station* start = chooseStation("请输入起点站关键词: ");
        if (!start) { pressAnyKey(); continue; }
        if (!start->open) { cout << "起点已关闭，无法进行路径规划。\n"; pressAnyKey(); continue; }

        Station* end = chooseStation("请输入终点站关键词: ");
        if (!end) { pressAnyKey(); continue; }
        if (!end->open) { cout << "终点已关闭，无法进行路径规划。\n"; pressAnyKey(); continue; }

        if (start->id == end->id) {
            cout << "起点和终点相同，无需进行路径规划。\n"; pressAnyKey(); continue;
        }

        if (!globalGraph.isReachable(start->id, end->id)) {
            cout << "无可达路径。\n"; pressAnyKey(); continue;
        }

        int transfers, totalTime;
        vector<string> detail;
        vector<int> path = globalGraph.getLeastTransferPath(start->id, end->id, transfers, totalTime, detail);
        showPath(path, totalTime, detail, transfers);

        if (opt == 2) {
            cout << "\n提示: 换乘最少路径已给出最优解。\n";
        }
        pressAnyKey();
    }
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    cout << "正在加载数据...\n";

    if (!loadStations(stationCsvPath)) {
        string altPath;
        cout << "默认路径加载失败，请输入Station.csv文件路径: ";
        getline(cin, altPath);
        if (!altPath.empty()) stationCsvPath = altPath;
        if (!loadStations(stationCsvPath)) {
            cout << "错误: 无法加载站点数据文件。" << endl;
            return 1;
        }
    }
    cout << "  站点数据加载完成: " << globalGraph.stations.size() << " 个站点\n";

    if (!loadEdges(edgeCsvPath)) {
        string altPath;
        cout << "默认路径加载失败，请输入Edge.csv文件路径: ";
        getline(cin, altPath);
        if (!altPath.empty()) edgeCsvPath = altPath;
        if (!loadEdges(edgeCsvPath)) {
            cout << "错误: 无法加载边数据文件。" << endl;
            return 1;
        }
    }
    cout << "  边数据加载完成: " << globalGraph.edges.size() << " 条边\n";
    cout << "按回车键进入主菜单...";
    cin.get();

    while (true) {
        clearScreen();
        cout << "============================================\n";
        cout << "         上海地铁路径规划系统\n";
        cout << "============================================\n";
        cout << "  1. 线路站点信息/运营状态管理\n";
        cout << "  2. 所需时间最短路径规划\n";
        cout << "  3. 所需换乘次数最少路径规划\n";
        cout << "  4. 退出系统\n";
        cout << "============================================\n";

        int opt = readInt("请输入选项编号: ");
        if (opt < 1 || opt > 4) {
            cout << "输入无效，请输入数字选项1、2、3、4\n";
            pressAnyKey(); continue;
        }

        switch (opt) {
        case 1: menuStationManagement(); break;
        case 2: menuShortestTime(); break;
        case 3: menuLeastTransfer(); break;
        case 4:
            cout << "感谢使用，再见！\n";
            return 0;
        }
    }
    return 0;
}
