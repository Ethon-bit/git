#include <iostream>
#include <vector>
#include <sstream>
#include <string>
using namespace std;

// 顺序表模板类
template<class ElemType>
class SqList {
public:
    vector<ElemType> data;
    int length;

    SqList() : length(0) {}

    void insert(ElemType val) {
        data.push_back(val);
        length++;
    }

    void print() const {
        for (int i = 0; i < length; i++) {
            if (i > 0) cout << " ";
            cout << data[i];
        }
        cout << "\n";
    }
};

// 基于快速排序思想：把所有负数移到正数前面
template<class ElemType>
void Rearrange(SqList<ElemType>& A) {
    // 先输出排序前
    A.print();
    cout << "\n";

    int low = 0;
    int high = A.length - 1;

    while (low < high) {
        // 从左边找正数（>=0）
        while (low < high && A.data[low] < 0)
            low++;

        // 从右边找负数（<0）
        while (low < high && A.data[high] >= 0)
            high--;

        // 交换并输出中间结果
        if (low < high) {
            swap(A.data[low], A.data[high]);
            A.print();
            low++;
            high--;
        }
    }
}

int main() {
    SqList<int> A;
    int n;
    cin >> n;
    cin.ignore(); // 跳过换行

    string line;
    getline(cin, line);
    istringstream iss(line);
    int val;
    while (iss >> val) {
        A.insert(val);
    }

    Rearrange(A);
    return 0;
}
