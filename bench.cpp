#include "bplustree.hpp"
#include <iostream>
#include <chrono>
#include <random>
#include <filesystem>

using namespace std;

template<typename T>
inline double seconds(T start, T end) {
    return chrono::duration<double>(end - start).count();
}

int main() {
    const char *fname = "bench_index.idx";

    if (filesystem::exists(fname)) filesystem::remove(fname);

    BPlusTree tree(fname);

    const int N = 10000000;  // number of records to benchmark (1 crore)

    // --------------------------
    // INSERT BENCHMARK
    // --------------------------
    cout << "Starting insert benchmark with " << N << " records...\n";
    auto t1 = chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        if (i % 10000000 == 0) cout << "  Inserted " << i << " records...\n";
        string s = "Record " + to_string(i);
        tree.writeData(i, s);
    }
    auto t2 = chrono::high_resolution_clock::now();
    cout << "Insert " << N << " keys: " << seconds(t1, t2) << " seconds\n";

    // --------------------------
    // POINT QUERY BENCHMARK
    // --------------------------
    cout << "Starting point queries...\n";
    mt19937 rng(123);
    uniform_int_distribution<int> dist(0, N - 1);

    auto t3 = chrono::high_resolution_clock::now();
    BPlusTree::Value out;
    for (int i = 0; i < N; ++i) {
        int k = dist(rng);
        tree.readData(k, out);
    }
    auto t4 = chrono::high_resolution_clock::now();
    cout << "Random point queries (" << N << "): " << seconds(t3, t4) << " seconds\n";

    // --------------------------
    // RANGE QUERY BENCHMARK
    // --------------------------
    auto t5 = chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        int low = dist(rng);
        int high = low + 50;
        vector<BPlusTree::Value> rangeOut;
        tree.readRangeData(low, high, rangeOut);
    }
    auto t6 = chrono::high_resolution_clock::now();
    cout << "1000 range scans (50 keys each): " << seconds(t5, t6) << " seconds\n";

    // --------------------------
    // DELETE BENCHMARK
    // --------------------------
    auto t7 = chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        tree.deleteData(i);
    }
    auto t8 = chrono::high_resolution_clock::now();
    cout << "Delete " << N << " keys: " << seconds(t7, t8) << " seconds\n";

    tree.close();
}
