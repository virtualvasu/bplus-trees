#include "bplustree.hpp"
#include <iostream>
#include <filesystem>

int main() {
    const char *fname = "index_cpp.idx";
    if (std::filesystem::exists(fname)) {
        std::filesystem::remove(fname);
    }

    BPlusTree tree(fname);

    // Insert some records
    std::cout << "Inserting records...\n";
    for (int i = 1; i <= 1000; ++i) {
        std::string s = "Record " + std::to_string(i);
        tree.writeData(i, s);
    }

    // Read a few
    std::cout << "Reading some records...\n";
    for (int k : {1, 10, 500, 1000}) {
        BPlusTree::Value v;
        if (tree.readData(k, v)) {
            std::string s(reinterpret_cast<char*>(v.data()), BPlusTree::DATA_SIZE);
            std::cout << "Key " << k << ": " << s.substr(0, 40) << "\n";
        } else {
            std::cout << "Key " << k << ": NOT FOUND\n";
        }
    }

    // Range query
    std::cout << "Range [100, 120]:\n";
    std::vector<BPlusTree::Value> vec;
    size_t cnt = tree.readRangeData(100, 120, vec);
    std::cout << "Found " << cnt << " records\n";

    // Delete a few
    std::cout << "Deleting keys 10, 500...\n";
    tree.deleteData(10);
    tree.deleteData(500);

    BPlusTree::Value v;
    std::cout << "Check key 10: " << (tree.readData(10, v) ? "STILL THERE" : "DELETED") << "\n";
    std::cout << "Check key 500: " << (tree.readData(500, v) ? "STILL THERE" : "DELETED") << "\n";

    tree.close();
    std::cout << "Done.\n";
    return 0;
}
