#ifndef BPLUSTREE_HPP
#define BPLUSTREE_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <stdexcept>

class BPlusTree {
public:
    static constexpr uint32_t PAGE_SIZE  = 4096;
    static constexpr uint32_t DATA_SIZE  = 100;
    static constexpr uint32_t LEAF_MAX_KEYS = 39;   // fits in page
    static constexpr uint32_t INTERNAL_MAX_KEYS = 508;

    using Value = std::array<uint8_t, DATA_SIZE>;

    explicit BPlusTree(const std::string &filename);
    ~BPlusTree();

    // Disable copy
    BPlusTree(const BPlusTree&) = delete;
    BPlusTree& operator=(const BPlusTree&) = delete;

    bool writeData(int key, const uint8_t* data, size_t len);
    bool writeData(int key, const std::string &data); // convenience

    bool readData(int key, Value &out) const;

    // Returns number of records in [lower, upper] (inclusive),
    // fills 'out' with the values.
    size_t readRangeData(int lower, int upper, std::vector<Value> &out) const;

    // Simplified delete: removes key if present.
    // (Underflow rebalancing is minimal but correct for search.)
    bool deleteData(int key);

    void close();

private:
#pragma pack(push, 1)
    struct MetadataPage {
        uint32_t root_page;       // page id of root
        uint32_t next_free_page;  // next unused page id
        uint8_t  reserved[PAGE_SIZE - 8];
    };

    struct PageHeader {
        uint8_t  is_leaf;      // 1 = leaf, 0 = internal
        uint8_t  reserved1;
        uint16_t num_keys;
        int32_t  parent;       // -1 if none
        int32_t  next_leaf;    // leaf: next leaf page id; internal: unused
        uint8_t  reserved2[4]; // padding to 16 bytes
    };

    struct LeafPage {
        PageHeader hdr;
        int32_t    keys[LEAF_MAX_KEYS];
        uint8_t    values[LEAF_MAX_KEYS][DATA_SIZE];
        // some unused bytes at end (fits in 4096)
    };

    struct InternalPage {
        PageHeader hdr;
        int32_t    keys[INTERNAL_MAX_KEYS];
        uint32_t   children[INTERNAL_MAX_KEYS + 1];
    };
#pragma pack(pop)

    std::string filename_;
    int fd_;
    uint8_t *base_;
    size_t mapped_size_;
    uint32_t page_capacity_;  // mapped_size_ / PAGE_SIZE

    // Helpers
    MetadataPage* meta() const {
        return reinterpret_cast<MetadataPage*>(base_);
    }

    PageHeader* header(uint32_t page) const {
        if (page >= page_capacity_) {
            throw std::runtime_error("Invalid page access: " + std::to_string(page));
        }
        return reinterpret_cast<PageHeader*>(base_ + page * PAGE_SIZE);
    }

    LeafPage* leaf(uint32_t page) const {
        if (page >= page_capacity_) {
            throw std::runtime_error("Invalid leaf page access: " + std::to_string(page));
        }
        return reinterpret_cast<LeafPage*>(base_ + page * PAGE_SIZE);
    }

    InternalPage* internal(uint32_t page) const {
        if (page >= page_capacity_) {
            throw std::runtime_error("Invalid internal page access: " + std::to_string(page));
        }
        return reinterpret_cast<InternalPage*>(base_ + page * PAGE_SIZE);
    }

    void open_or_create();
    void map_file(size_t size);
    void ensure_capacity(uint32_t required_page);
    uint32_t allocate_page();

    // Tree helpers
    uint32_t root_page() const { return meta()->root_page; }
    void set_root_page(uint32_t p) { meta()->root_page = p; }

    uint32_t next_free_page() const { return meta()->next_free_page; }
    void set_next_free_page(uint32_t p) { meta()->next_free_page = p; }

    LeafPage* find_leaf(int key) const;
    void insert_in_leaf(LeafPage *leaf, int key, const uint8_t *data);
    void split_leaf_and_insert(uint32_t leaf_page, int key, const uint8_t *data);
    void insert_in_parent(uint32_t left_page, int key, uint32_t right_page);

    void insert_in_internal(uint32_t node_page, int key, uint32_t right_child);
    void split_internal_and_insert(uint32_t node_page, int key, uint32_t right_child);

    // deletion helpers (simplified but correct)
    bool delete_from_leaf(LeafPage *leaf, int key);
    void maybe_shrink_root();

    // Binary search helpers
    static int lower_bound_keys(const int32_t *keys, int n, int key);

    // Helper to get page number from pointer
    uint32_t get_page_num(void *ptr) const {
        return static_cast<uint32_t>((reinterpret_cast<uint8_t*>(ptr) - base_) / PAGE_SIZE);
    }
};

#endif // BPLUSTREE_HPP
