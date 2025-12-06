#include "bplustree.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace {
    constexpr size_t INITIAL_PAGES = 65536; // ~256MB initial
}

int BPlusTree::lower_bound_keys(const int32_t *keys, int n, int key) {
    int lo = 0, hi = n;
    while (lo < hi) {
        int mid = (lo + hi) >> 1;
        if (key <= keys[mid]) hi = mid;
        else lo = mid + 1;
    }
    return lo;
}

BPlusTree::BPlusTree(const std::string &filename)
    : filename_(filename),
      fd_(-1),
      base_(nullptr),
      mapped_size_(0),
      page_capacity_(0)
{
    open_or_create();
}

BPlusTree::~BPlusTree() {
    close();
}

void BPlusTree::open_or_create() {
    bool exists = (access(filename_.c_str(), F_OK) == 0);

    fd_ = ::open(filename_.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd_ < 0) throw std::runtime_error("Failed to open file");

    if (!exists) {
        // new file: allocate initial space
        size_t size = INITIAL_PAGES * PAGE_SIZE;
        if (ftruncate(fd_, size) != 0) {
            ::close(fd_);
            throw std::runtime_error("ftruncate failed");
        }
        map_file(size);

        // init metadata
        MetadataPage *m = meta();
        std::memset(m, 0, sizeof(MetadataPage));
        m->root_page = 1;
        m->next_free_page = 2;

        // init root leaf page
        LeafPage *root = leaf(1);
        std::memset(root, 0, sizeof(LeafPage));
        root->hdr.is_leaf = 1;
        root->hdr.num_keys = 0;
        root->hdr.parent = -1;
        root->hdr.next_leaf = -1;
    } else {
        struct stat st{};
        if (fstat(fd_, &st) != 0) {
            ::close(fd_);
            throw std::runtime_error("fstat failed");
        }
        if (st.st_size == 0) {
            size_t size = INITIAL_PAGES * PAGE_SIZE;
            if (ftruncate(fd_, size) != 0) {
                ::close(fd_);
                throw std::runtime_error("ftruncate failed");
            }
            map_file(size);
            MetadataPage *m = meta();
            std::memset(m, 0, sizeof(MetadataPage));
            m->root_page = 1;
            m->next_free_page = 2;
            LeafPage *root = leaf(1);
            std::memset(root, 0, sizeof(LeafPage));
            root->hdr.is_leaf = 1;
            root->hdr.num_keys = 0;
            root->hdr.parent = -1;
            root->hdr.next_leaf = -1;
        } else {
            map_file(st.st_size);
        }
    }
}

void BPlusTree::map_file(size_t size) {
    if (base_ != nullptr) {
        munmap(base_, mapped_size_);
    }
    base_ = reinterpret_cast<uint8_t*>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0));
    if (base_ == MAP_FAILED) {
        base_ = nullptr;
        ::close(fd_);
        throw std::runtime_error("mmap failed");
    }
    mapped_size_ = size;
    page_capacity_ = mapped_size_ / PAGE_SIZE;
}

void BPlusTree::ensure_capacity(uint32_t required_page) {
    if (required_page < page_capacity_) return;
    // grow: double size
    size_t new_pages = page_capacity_ * 2;
    if (new_pages <= required_page) new_pages = required_page + 128;
    size_t new_size = new_pages * PAGE_SIZE;
    if (ftruncate(fd_, new_size) != 0) {
        throw std::runtime_error("ftruncate (grow) failed");
    }
    map_file(new_size);
}

uint32_t BPlusTree::allocate_page() {
    uint32_t p = next_free_page();
    ensure_capacity(p);
    set_next_free_page(p + 1);
    // zero page
    std::memset(base_ + p * PAGE_SIZE, 0, PAGE_SIZE);
    return p;
}

bool BPlusTree::writeData(int key, const uint8_t* data, size_t len) {
    if (key < 0) return false;

    uint8_t buf[DATA_SIZE];
    if (len >= DATA_SIZE) {
        std::memcpy(buf, data, DATA_SIZE);
    } else {
        std::memcpy(buf, data, len);
        std::memset(buf + len, 0, DATA_SIZE - len);
    }

    LeafPage *lf = find_leaf(key);
    uint32_t leaf_page = get_page_num(lf);
    
    // fast path: if not full, insert in-place
    if (lf->hdr.num_keys < LEAF_MAX_KEYS) {
        insert_in_leaf(lf, key, buf);
    } else {
        split_leaf_and_insert(leaf_page, key, buf);
    }
    return true;
}

bool BPlusTree::writeData(int key, const std::string &data) {
    return writeData(key, reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

BPlusTree::LeafPage* BPlusTree::find_leaf(int key) const {
    uint32_t page = root_page();
    PageHeader *h = header(page);

    while (h->is_leaf == 0) {
        InternalPage *node = internal(page);
        int n = node->hdr.num_keys;
        int idx = 0;

        // optimized: check last key first for typical ascending inserts
        if (n > 0 && key >= node->keys[n - 1]) {
            idx = n;
        } else {
            // Binary search for upper_bound (first key > search key)
            int lo = 0, hi = n;
            while (lo < hi) {
                int mid = (lo + hi) >> 1;
                if (key < node->keys[mid]) hi = mid;
                else lo = mid + 1;
            }
            idx = lo;
        }
        
        // Validate child page number before accessing
        if (idx > n) {
            throw std::runtime_error("Invalid child index: " + std::to_string(idx) + " with " + std::to_string(n) + " keys");
        }
        page = node->children[idx];
        if (page == 0 || page >= page_capacity_) {
            throw std::runtime_error("Invalid child page in internal node: " + std::to_string(page) + " (capacity: " + std::to_string(page_capacity_) + ")");
        }
        h = header(page);
    }
    return leaf(page);
}

void BPlusTree::insert_in_leaf(LeafPage *lf, int key, const uint8_t *data) {
    int n = lf->hdr.num_keys;
    int pos = lower_bound_keys(lf->keys, n, key);

    // if key exists, update
    if (pos < n && lf->keys[pos] == key) {
        std::memcpy(lf->values[pos], data, DATA_SIZE);
        return;
    }

    // shift right
    for (int i = n; i > pos; --i) {
        lf->keys[i] = lf->keys[i - 1];
        std::memcpy(lf->values[i], lf->values[i - 1], DATA_SIZE);
    }

    lf->keys[pos] = key;
    std::memcpy(lf->values[pos], data, DATA_SIZE);
    lf->hdr.num_keys = n + 1;
}

void BPlusTree::split_leaf_and_insert(uint32_t leaf_page_num, int key, const uint8_t *data) {
    // temp arrays of size LEAF_MAX_KEYS + 1
    int temp_keys[LEAF_MAX_KEYS + 1];
    uint8_t temp_vals[LEAF_MAX_KEYS + 1][DATA_SIZE];

    LeafPage *lf = leaf(leaf_page_num);
    int n = lf->hdr.num_keys;
    int32_t old_parent = lf->hdr.parent;
    int32_t old_next = lf->hdr.next_leaf;
    int pos = lower_bound_keys(lf->keys, n, key);

    int i = 0, j = 0;
    for (; i < n; ++i, ++j) {
        if (j == pos) ++j;
        temp_keys[j] = lf->keys[i];
        std::memcpy(temp_vals[j], lf->values[i], DATA_SIZE);
    }
    temp_keys[pos] = key;
    std::memcpy(temp_vals[pos], data, DATA_SIZE);

    int total = n + 1;
    int split = total / 2; // left count

    // Allocate new page FIRST (may cause remapping)
    uint32_t new_page = allocate_page();
    
    // Re-get pointers after potential remapping
    lf = leaf(leaf_page_num);
    LeafPage *right = leaf(new_page);
    
    // left node
    lf->hdr.num_keys = split;
    for (i = 0; i < split; ++i) {
        lf->keys[i] = temp_keys[i];
        std::memcpy(lf->values[i], temp_vals[i], DATA_SIZE);
    }

    // right node
    right->hdr.is_leaf = 1;
    right->hdr.parent  = old_parent;
    right->hdr.next_leaf = old_next;
    int right_count = total - split;
    right->hdr.num_keys = right_count;
    for (i = 0; i < right_count; ++i) {
        right->keys[i] = temp_keys[split + i];
        std::memcpy(right->values[i], temp_vals[split + i], DATA_SIZE);
    }

    // link leaves
    lf->hdr.next_leaf = new_page;

    int separator = right->keys[0];
    insert_in_parent(leaf_page_num,
                     separator,
                     new_page);
}

void BPlusTree::insert_in_parent(uint32_t left_page, int key, uint32_t right_page) {
    PageHeader *left_hdr = header(left_page);
    if (left_hdr->parent == -1) {
        // new root
        uint32_t new_root_page = allocate_page();
        InternalPage *root = internal(new_root_page);
        std::memset(root, 0, sizeof(InternalPage));
        root->hdr.is_leaf = 0;
        root->hdr.num_keys = 1;
        root->hdr.parent = -1;
        root->children[0] = left_page;
        root->children[1] = right_page;
        root->keys[0] = key;

        // update parents
        header(left_page)->parent = new_root_page;
        header(right_page)->parent = new_root_page;

        set_root_page(new_root_page);
        return;
    }

    uint32_t parent_page = left_hdr->parent;
    insert_in_internal(parent_page, key, right_page);
}

void BPlusTree::insert_in_internal(uint32_t node_page, int key, uint32_t right_child) {
    InternalPage *node = internal(node_page);
    int n = node->hdr.num_keys;
    
    if (static_cast<uint32_t>(n) >= INTERNAL_MAX_KEYS) {
        // Node is already full, need to split first
        split_internal_and_insert(node_page, key, right_child);
        return;
    }
    
    int pos = lower_bound_keys(node->keys, n, key);

    // shift children & keys
    for (int i = n; i > pos; --i) {
        node->keys[i] = node->keys[i - 1];
    }
    for (int i = n + 1; i > pos + 1; --i) {
        node->children[i] = node->children[i - 1];
    }

    node->keys[pos] = key;
    node->children[pos + 1] = right_child;
    node->hdr.num_keys = n + 1;

    header(right_child)->parent = static_cast<int32_t>(node_page);
}

void BPlusTree::split_internal_and_insert(uint32_t node_page, int key, uint32_t right_child) {
    // Create temporary arrays to hold all keys and children including the new one
    int temp_keys[INTERNAL_MAX_KEYS + 1];
    uint32_t temp_children[INTERNAL_MAX_KEYS + 2];
    
    InternalPage *node = internal(node_page);
    int n = node->hdr.num_keys;
    int32_t old_parent = node->hdr.parent;
    int pos = lower_bound_keys(node->keys, n, key);
    
    // Copy existing keys and children to temp arrays, leaving space for new entry
    int i = 0, j = 0;
    for (; i < n; ++i, ++j) {
        if (j == pos) ++j;
        temp_keys[j] = node->keys[i];
    }
    temp_keys[pos] = key;
    
    i = 0; j = 0;
    for (; i <= n; ++i, ++j) {
        if (j == pos + 1) ++j;
        temp_children[j] = node->children[i];
    }
    temp_children[pos + 1] = right_child;
    
    int total = n + 1;
    int mid = total / 2;
    int up_key = temp_keys[mid];
    
    // Create new right sibling (may cause remapping)
    uint32_t new_page = allocate_page();
    
    // Re-get pointers after potential remapping
    node = internal(node_page);
    InternalPage *right = internal(new_page);
    std::memset(right, 0, PAGE_SIZE);
    right->hdr.is_leaf = 0;
    right->hdr.parent = old_parent;
    
    // Distribute to left (original node)
    node->hdr.num_keys = mid;
    for (i = 0; i < mid; ++i) {
        node->keys[i] = temp_keys[i];
    }
    for (i = 0; i <= mid; ++i) {
        node->children[i] = temp_children[i];
    }
    
    // Distribute to right (new node)
    int right_keys = total - mid - 1;
    right->hdr.num_keys = right_keys;
    for (i = 0; i < right_keys; ++i) {
        right->keys[i] = temp_keys[mid + 1 + i];
    }
    for (i = 0; i <= right_keys; ++i) {
        right->children[i] = temp_children[mid + 1 + i];
        header(right->children[i])->parent = static_cast<int32_t>(new_page);
    }
    
    // Update parent pointers for left's children
    for (i = 0; i <= mid; ++i) {
        header(node->children[i])->parent = static_cast<int32_t>(node_page);
    }

    // insert up_key into parent
    insert_in_parent(node_page, up_key, new_page);
}

bool BPlusTree::readData(int key, Value &out) const {
    if (root_page() == 0) return false;
    LeafPage *lf = find_leaf(key);
    int n = lf->hdr.num_keys;
    int pos = lower_bound_keys(lf->keys, n, key);
    if (pos < n && lf->keys[pos] == key) {
        std::memcpy(out.data(), lf->values[pos], DATA_SIZE);
        return true;
    }
    return false;
}

size_t BPlusTree::readRangeData(int lower, int upper, std::vector<Value> &out) const {
    out.clear();
    if (root_page() == 0) return 0;

    LeafPage *lf = find_leaf(lower);
    while (lf != nullptr) {
        int n = lf->hdr.num_keys;
        for (int i = 0; i < n; ++i) {
            int k = lf->keys[i];
            if (k < lower) continue;
            if (k > upper) return out.size();
            Value v{};
            std::memcpy(v.data(), lf->values[i], DATA_SIZE);
            out.push_back(v);
        }
        if (lf->hdr.next_leaf == -1) break;
        lf = leaf(lf->hdr.next_leaf);
    }
    return out.size();
}

bool BPlusTree::delete_from_leaf(LeafPage *lf, int key) {
    int n = lf->hdr.num_keys;
    int pos = lower_bound_keys(lf->keys, n, key);
    if (pos >= n || lf->keys[pos] != key) return false;

    // shift left
    for (int i = pos; i < n - 1; ++i) {
        lf->keys[i] = lf->keys[i + 1];
        std::memcpy(lf->values[i], lf->values[i + 1], DATA_SIZE);
    }
    lf->hdr.num_keys = n - 1;
    return true;
}

bool BPlusTree::deleteData(int key) {
    if (root_page() == 0) return false;
    LeafPage *lf = find_leaf(key);
    if (!delete_from_leaf(lf, key)) return false;

    maybe_shrink_root();
    // NOTE: For simplicity, we do not implement full underflow rebalancing here.
    // The tree remains correct for search; pages may be underfull but valid.
    return true;
}

void BPlusTree::maybe_shrink_root() {
    uint32_t r = root_page();
    PageHeader *rh = header(r);
    if (rh->is_leaf) {
        // if empty, we could keep single empty root leaf
        return;
    }
    InternalPage *root = internal(r);
    if (root->hdr.num_keys == 0) {
        // make only child the new root
        uint32_t child_page = root->children[0];
        set_root_page(child_page);
        header(child_page)->parent = -1;
    }
}

void BPlusTree::close() {
    if (base_) {
        msync(base_, mapped_size_, MS_SYNC);
        munmap(base_, mapped_size_);
        base_ = nullptr;
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}
