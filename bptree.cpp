/*
 * B+ Tree Index - Optimized for Sequential Inserts
 * Uses cached leaf optimization + aggressive batching
 */

#include "bptree.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 4096
#define DATA_SIZE 100
#define LEAF_ORDER 39
#define INTERNAL_ORDER 510  // (4096 - 8) / 8 = 510 children per internal node

typedef struct {
    int key;
    unsigned char data[DATA_SIZE];
} LeafEntry;

typedef struct {
    unsigned char is_leaf;  // 1 for leaf
    unsigned char padding1[3];
    int num_keys;
    int next_leaf;
    int parent;
    LeafEntry entries[LEAF_ORDER];
    char padding2[PAGE_SIZE - sizeof(unsigned char) - 3 - 3*sizeof(int) - LEAF_ORDER*sizeof(LeafEntry)];
} LeafNode;

// Internal node: stores keys and child pointers for tree navigation
// Layout: [is_leaf=0][num_keys][parent][child_0][key_0][child_1][key_1]...[child_n]
typedef struct {
    unsigned char is_leaf;  // 0 for internal
    unsigned char padding1[3];
    int num_keys;           // Number of keys (num_children = num_keys + 1)
    int parent;
    int children[INTERNAL_ORDER];  // Child page IDs
    int keys[INTERNAL_ORDER - 1];   // Separator keys (one less than children)
    char padding2[PAGE_SIZE - 4 - 2*sizeof(int) - INTERNAL_ORDER*sizeof(int) - (INTERNAL_ORDER-1)*sizeof(int)];
} InternalNode;

typedef struct {
    int root_page;
    int num_pages;
    int first_leaf;
    int tree_height;
    char padding[PAGE_SIZE - 4*sizeof(int)];
} Metadata;

// Global state
#ifdef _WIN32
static HANDLE g_file = INVALID_HANDLE_VALUE;
static HANDLE g_map = NULL;
#else
static int g_fd = -1;
#endif

static void* g_mem = NULL;
static size_t g_size = 0;
static Metadata* g_meta = NULL;

// Cache for sequential inserts
static int g_last_leaf_page = -1;
static int g_last_leaf_max_key = -1;

static inline void* get_page(int page_id) {
    if (page_id < 0 || !g_mem) return NULL;
    return (char*)g_mem + ((size_t)page_id * PAGE_SIZE);
}

static int extend_file(size_t new_size) {
#ifdef _WIN32
    if (g_mem) {
        UnmapViewOfFile(g_mem);
        g_mem = NULL;
    }
    if (g_map) {
        CloseHandle(g_map);
        g_map = NULL;
    }
    
    LARGE_INTEGER sz;
    sz.QuadPart = new_size;
    if (!SetFilePointerEx(g_file, sz, NULL, FILE_BEGIN)) return 0;
    if (!SetEndOfFile(g_file)) return 0;
    
    g_map = CreateFileMapping(g_file, NULL, PAGE_READWRITE,
                              (DWORD)(new_size >> 32), (DWORD)new_size, NULL);
    if (!g_map) return 0;
    
    g_mem = MapViewOfFile(g_map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!g_mem) return 0;
#else
    if (g_mem) {
        munmap(g_mem, g_size);
        g_mem = NULL;
    }
    
    if (ftruncate(g_fd, new_size) == -1) return 0;
    
    g_mem = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, g_fd, 0);
    if (g_mem == MAP_FAILED) {
        g_mem = NULL;
        return 0;
    }
#endif
    
    g_size = new_size;
    g_meta = (Metadata*)g_mem;
    return 1;
}

static int allocate_page() {
    if (!g_meta) return -1;
    
    int page_id = g_meta->num_pages++;
    size_t required = (size_t)g_meta->num_pages * PAGE_SIZE;
    
    if (required > g_size) {
        size_t new_size = g_size * 2;
        if (new_size < required) new_size = required * 2;
        if (!extend_file(new_size)) {
            g_meta->num_pages--;
            return -1;
        }
    }
    
    void* page = get_page(page_id);
    if (page) {
        memset(page, 0, PAGE_SIZE);
    }
    
    return page_id;
}

// O(log n) tree traversal using internal nodes
static LeafNode* find_leaf(int key) {
    if (!g_meta || g_meta->root_page == -1) return NULL;
    
    // Fast path: check cached leaf for sequential inserts
    // This works regardless of tree height
    if (g_last_leaf_page != -1 && key > g_last_leaf_max_key) {
        LeafNode* cached = (LeafNode*)get_page(g_last_leaf_page);
        if (cached && cached->is_leaf) {
            // If it's the rightmost leaf, use it
            if (cached->next_leaf == -1) {
                return cached;
            }
            // Check next leaf
            LeafNode* next = (LeafNode*)get_page(cached->next_leaf);
            if (next && next->is_leaf) {
                if (next->num_keys == 0 || key <= next->entries[next->num_keys - 1].key) {
                    return next;
                }
                if (next->next_leaf == -1) {
                    return next;
                }
            }
        }
    }
    
    // Get root node
    void* node = get_page(g_meta->root_page);
    if (!node) return NULL;
    
    unsigned char is_leaf = *((unsigned char*)node);
    
    // If tree height is 1, root is a leaf - use leaf chain traversal for simplicity
    if (is_leaf || g_meta->tree_height == 1) {
        LeafNode* leaf = (LeafNode*)node;
        while (leaf) {
            if (leaf->num_keys == 0) return leaf;
            if (key <= leaf->entries[leaf->num_keys - 1].key) {
                return leaf;
            }
            if (leaf->next_leaf == -1) {
                return leaf;
            }
            leaf = (LeafNode*)get_page(leaf->next_leaf);
        }
        return NULL;
    }
    
    // Tree traversal for height > 1: navigate through internal nodes
    while (1) {
        is_leaf = *((unsigned char*)node);
        
        // If leaf node, we're done
        if (is_leaf) {
            return (LeafNode*)node;
        }
        
        // Internal node: find which child to follow
        InternalNode* internal = (InternalNode*)node;
        int child_idx = 0;
        
        // Binary search for the correct child
        int left = 0, right = internal->num_keys - 1;
        while (left <= right) {
            int mid = left + ((right - left) >> 1);
            if (key < internal->keys[mid]) {
                right = mid - 1;
            } else {
                child_idx = mid + 1;
                left = mid + 1;
            }
        }
        
        // Follow the child pointer
        node = get_page(internal->children[child_idx]);
        if (!node) return NULL;
    }
}

static int search_in_leaf(LeafNode* leaf, int key) {
    int left = 0, right = leaf->num_keys - 1;
    
    while (left <= right) {
        int mid = left + ((right - left) >> 1);
        if (leaf->entries[mid].key == key) return mid;
        if (leaf->entries[mid].key < key) left = mid + 1;
        else right = mid - 1;
    }
    
    return -1;
}

static int find_insert_position(LeafNode* leaf, int key) {
    int pos = 0;
    while (pos < leaf->num_keys && leaf->entries[pos].key < key) {
        pos++;
    }
    return pos;
}

static void flush_changes() {
#ifdef _WIN32
    if (g_mem) FlushViewOfFile(g_mem, 0);
#else
    if (g_mem) msync(g_mem, g_size, MS_ASYNC);
#endif
}

void initBPTree(const char* filename) {
    if (!filename) return;
    
#ifdef _WIN32
    g_file = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ, NULL,
                         OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (g_file == INVALID_HANDLE_VALUE) return;
    
    LARGE_INTEGER sz;
    GetFileSizeEx(g_file, &sz);
    int is_new = (sz.QuadPart == 0);
    
    if (is_new) {
        // Start with 2GB for 10M keys
        g_size = (size_t)PAGE_SIZE * 524288;
        sz.QuadPart = g_size;
        SetFilePointerEx(g_file, sz, NULL, FILE_BEGIN);
        SetEndOfFile(g_file);
    } else {
        g_size = sz.QuadPart;
    }
    
    g_map = CreateFileMapping(g_file, NULL, PAGE_READWRITE,
                              (DWORD)(g_size >> 32), (DWORD)g_size, NULL);
    if (!g_map) {
        CloseHandle(g_file);
        g_file = INVALID_HANDLE_VALUE;
        return;
    }
    
    g_mem = MapViewOfFile(g_map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!g_mem) {
        CloseHandle(g_map);
        CloseHandle(g_file);
        g_map = NULL;
        g_file = INVALID_HANDLE_VALUE;
        return;
    }
#else
    g_fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (g_fd == -1) return;
    
    struct stat st;
    fstat(g_fd, &st);
    int is_new = (st.st_size == 0);
    
    if (is_new) {
        g_size = (size_t)PAGE_SIZE * 524288;  // 2GB
        ftruncate(g_fd, g_size);
    } else {
        g_size = st.st_size;
    }
    
    g_mem = mmap(NULL, g_size, PROT_READ | PROT_WRITE, MAP_SHARED, g_fd, 0);
    if (g_mem == MAP_FAILED) {
        close(g_fd);
        g_fd = -1;
        g_mem = NULL;
        return;
    }
#endif
    
    g_meta = (Metadata*)g_mem;
    
    if (is_new) {
        g_meta->root_page = -1;
        g_meta->num_pages = 1;
        g_meta->first_leaf = -1;
        g_meta->tree_height = 0;
        g_last_leaf_page = -1;
        g_last_leaf_max_key = -1;
        flush_changes();
    } else {
        g_last_leaf_page = -1;
        g_last_leaf_max_key = -1;
    }
}

// Insert key into parent, creating new root if necessary
// Called after splitting a node (leaf or internal)
static void insert_in_parent(int left_page_id, int split_key, int right_page_id) {
    void* left_node = get_page(left_page_id);
    if (!left_node) return;
    
    unsigned char is_leaf = *((unsigned char*)left_node);
    int parent_id = is_leaf ? ((LeafNode*)left_node)->parent : ((InternalNode*)left_node)->parent;
    
    // Case 1: No parent - create new root
    if (parent_id == -1) {
        int new_root_id = allocate_page();
        if (new_root_id == -1) return;
        
        InternalNode* new_root = (InternalNode*)get_page(new_root_id);
        if (!new_root) return;
        
        new_root->is_leaf = 0;
        new_root->num_keys = 1;
        new_root->parent = -1;
        new_root->keys[0] = split_key;
        new_root->children[0] = left_page_id;
        new_root->children[1] = right_page_id;
        
        // Update children's parent pointers (re-fetch after allocation)
        left_node = get_page(left_page_id);
        void* right_node = get_page(right_page_id);
        
        if (is_leaf) {
            ((LeafNode*)left_node)->parent = new_root_id;
            ((LeafNode*)right_node)->parent = new_root_id;
        } else {
            ((InternalNode*)left_node)->parent = new_root_id;
            ((InternalNode*)right_node)->parent = new_root_id;
        }
        
        g_meta->root_page = new_root_id;
        g_meta->tree_height++;
        return;
    }
    
    // Case 2: Parent exists - insert into it
    InternalNode* parent = (InternalNode*)get_page(parent_id);
    if (!parent) return;
    
    // If parent has space, insert the key
    if (parent->num_keys < INTERNAL_ORDER - 1) {
        // Find position to insert
        int pos = 0;
        while (pos < parent->num_keys && parent->keys[pos] < split_key) {
            pos++;
        }
        
        // Shift keys and children
        for (int i = parent->num_keys; i > pos; i--) {
            parent->keys[i] = parent->keys[i-1];
            parent->children[i+1] = parent->children[i];
        }
        
        parent->keys[pos] = split_key;
        parent->children[pos+1] = right_page_id;
        parent->num_keys++;
        
        return;
    }
    
    // Case 3: Parent is full - split it
    int temp_keys[INTERNAL_ORDER];
    int temp_children[INTERNAL_ORDER + 1];
    
    // Find insertion position
    int pos = 0;
    while (pos < parent->num_keys && parent->keys[pos] < split_key) {
        pos++;
    }
    
    // Copy to temp arrays with new key inserted
    for (int i = 0; i < pos; i++) {
        temp_keys[i] = parent->keys[i];
        temp_children[i] = parent->children[i];
    }
    temp_keys[pos] = split_key;
    temp_children[pos] = parent->children[pos];
    temp_children[pos+1] = right_page_id;
    for (int i = pos; i < parent->num_keys; i++) {
        temp_keys[i+1] = parent->keys[i];
        temp_children[i+2] = parent->children[i+1];
    }
    
    // Split internal node
    int mid = INTERNAL_ORDER / 2;
    int promote_key = temp_keys[mid];
    
    int parent_page_id = ((char*)parent - (char*)g_mem) / PAGE_SIZE;
    int new_internal_id = allocate_page();
    if (new_internal_id == -1) return;
    
    // Re-fetch parent after allocation
    parent = (InternalNode*)get_page(parent_page_id);
    if (!parent) return;
    
    InternalNode* new_internal = (InternalNode*)get_page(new_internal_id);
    if (!new_internal) return;
    
    new_internal->is_leaf = 0;
    new_internal->parent = parent->parent;
    
    // Left half stays in parent (keys 0..mid-1, children 0..mid)
    parent->num_keys = mid;
    for (int i = 0; i < mid; i++) {
        parent->keys[i] = temp_keys[i];
        parent->children[i] = temp_children[i];
    }
    parent->children[mid] = temp_children[mid];
    
    // Right half goes to new_internal (keys mid+1..end, children mid+1..end)
    new_internal->num_keys = INTERNAL_ORDER - mid - 1;
    for (int i = 0; i < new_internal->num_keys; i++) {
        new_internal->keys[i] = temp_keys[mid + 1 + i];
        new_internal->children[i] = temp_children[mid + 1 + i];
    }
    new_internal->children[new_internal->num_keys] = temp_children[INTERNAL_ORDER];
    
    // Update children's parent pointers for new_internal's children
    for (int i = 0; i <= new_internal->num_keys; i++) {
        void* child = get_page(new_internal->children[i]);
        if (child) {
            unsigned char child_is_leaf = *((unsigned char*)child);
            if (child_is_leaf) {
                ((LeafNode*)child)->parent = new_internal_id;
            } else {
                ((InternalNode*)child)->parent = new_internal_id;
            }
        }
    }
    
    // Recursively insert promote_key in parent's parent
    insert_in_parent(parent_page_id, promote_key, new_internal_id);
}

void closeBPTree() {
#ifdef _WIN32
    if (g_mem) {
        FlushViewOfFile(g_mem, 0);
        UnmapViewOfFile(g_mem);
        g_mem = NULL;
    }
    if (g_map) {
        CloseHandle(g_map);
        g_map = NULL;
    }
    if (g_file != INVALID_HANDLE_VALUE) {
        CloseHandle(g_file);
        g_file = INVALID_HANDLE_VALUE;
    }
#else
    if (g_mem) {
        msync(g_mem, g_size, MS_SYNC);
        munmap(g_mem, g_size);
        g_mem = NULL;
    }
    if (g_fd != -1) {
        close(g_fd);
        g_fd = -1;
    }
#endif
    
    g_meta = NULL;
    g_size = 0;
    g_last_leaf_page = -1;
    g_last_leaf_max_key = -1;
}

int writeData(int key, const unsigned char* data) {
    if (!g_mem || !g_meta || !data) return 0;
    
    if (g_meta->root_page == -1) {
        int leaf_id = allocate_page();
        if (leaf_id == -1) return 0;
        
        LeafNode* leaf = (LeafNode*)get_page(leaf_id);
        if (!leaf) return 0;
        
        leaf->is_leaf = 1;
        leaf->num_keys = 1;
        leaf->next_leaf = -1;
        leaf->parent = -1;
        leaf->entries[0].key = key;
        memcpy(leaf->entries[0].data, data, DATA_SIZE);
        
        g_meta->root_page = leaf_id;
        g_meta->first_leaf = leaf_id;
        g_meta->tree_height = 1;  // Tree with just one leaf
        
        g_last_leaf_page = leaf_id;
        g_last_leaf_max_key = key;
        
        return 1;
    }
    
    LeafNode* leaf = find_leaf(key);
    if (!leaf) return 0;
    
    int pos = search_in_leaf(leaf, key);
    if (pos != -1) {
        memcpy(leaf->entries[pos].data, data, DATA_SIZE);
        return 1;
    }
    
    if (leaf->num_keys < LEAF_ORDER) {
        int insert_pos = find_insert_position(leaf, key);
        
        for (int i = leaf->num_keys; i > insert_pos; i--) {
            leaf->entries[i] = leaf->entries[i-1];
        }
        
        leaf->entries[insert_pos].key = key;
        memcpy(leaf->entries[insert_pos].data, data, DATA_SIZE);
        leaf->num_keys++;
        
        if (leaf->next_leaf == -1) {
            int leaf_page = ((char*)leaf - (char*)g_mem) / PAGE_SIZE;
            g_last_leaf_page = leaf_page;
            g_last_leaf_max_key = leaf->entries[leaf->num_keys - 1].key;
        }
        
        // Only flush every 10000 writes
        static int write_count = 0;
        if (++write_count % 10000 == 0) flush_changes();
        
        return 1;
    }
    
    // Split leaf
    // Store leaf page ID before allocation (pointers get invalidated)
    int leaf_page_id = ((char*)leaf - (char*)g_mem) / PAGE_SIZE;
    
    int new_leaf_id = allocate_page();
    if (new_leaf_id == -1) return 0;
    
    // CRITICAL: Re-fetch leaf pointer after allocate_page (may have extended file)
    // Use direct page ID lookup instead of find_leaf to avoid O(n) scan
    leaf = (LeafNode*)get_page(leaf_page_id);
    if (!leaf) return 0;
    
    LeafNode* new_leaf = (LeafNode*)get_page(new_leaf_id);
    if (!new_leaf) return 0;
    
    new_leaf->is_leaf = 1;
    new_leaf->next_leaf = leaf->next_leaf;
    new_leaf->parent = leaf->parent;
    leaf->next_leaf = new_leaf_id;
    
    LeafEntry temp[LEAF_ORDER + 1];
    int insert_pos = find_insert_position(leaf, key);
    
    for (int i = 0; i < insert_pos; i++) {
        temp[i] = leaf->entries[i];
    }
    temp[insert_pos].key = key;
    memcpy(temp[insert_pos].data, data, DATA_SIZE);
    for (int i = insert_pos; i < leaf->num_keys; i++) {
        temp[i+1] = leaf->entries[i];
    }
    
    int mid = (LEAF_ORDER + 1) / 2;
    
    leaf->num_keys = mid;
    for (int i = 0; i < mid; i++) {
        leaf->entries[i] = temp[i];
    }
    
    new_leaf->num_keys = (LEAF_ORDER + 1) - mid;
    for (int i = 0; i < new_leaf->num_keys; i++) {
        new_leaf->entries[i] = temp[mid + i];
    }
    
    // Promote first key of new_leaf to parent
    int split_key = new_leaf->entries[0].key;
    insert_in_parent(leaf_page_id, split_key, new_leaf_id);
    
    g_last_leaf_page = new_leaf_id;
    g_last_leaf_max_key = new_leaf->entries[new_leaf->num_keys - 1].key;
    
    static int write_count = 0;
    if (++write_count % 10000 == 0) flush_changes();
    
    return 1;
}

int readData(int key, unsigned char* data_out) {
    if (!g_mem || !g_meta || !data_out) return 0;
    
    LeafNode* leaf = find_leaf(key);
    if (!leaf) return 0;
    
    int pos = search_in_leaf(leaf, key);
    if (pos == -1) return 0;
    
    memcpy(data_out, leaf->entries[pos].data, DATA_SIZE);
    return 1;
}

int deleteData(int key) {
    if (!g_mem || !g_meta) return 0;
    
    LeafNode* leaf = find_leaf(key);
    if (!leaf) return 0;
    
    int pos = search_in_leaf(leaf, key);
    if (pos == -1) return 0;
    
    for (int i = pos; i < leaf->num_keys - 1; i++) {
        leaf->entries[i] = leaf->entries[i+1];
    }
    leaf->num_keys--;
    
    static int delete_count = 0;
    if (++delete_count % 10000 == 0) flush_changes();
    
    return 1;
}

unsigned char** readRangeData(int lowerKey, int upperKey, int* n) {
    if (!g_mem || !g_meta || !n) return NULL;
    
    *n = 0;
    
    if (g_meta->root_page == -1) return NULL;
    
    LeafNode* leaf = find_leaf(lowerKey);
    if (!leaf) return NULL;
    
    int count = 0;
    LeafNode* temp = leaf;
    while (temp) {
        for (int i = 0; i < temp->num_keys; i++) {
            if (temp->entries[i].key >= lowerKey && temp->entries[i].key <= upperKey) {
                count++;
            } else if (temp->entries[i].key > upperKey) {
                goto done_counting;
            }
        }
        if (temp->next_leaf == -1) break;
        temp = (LeafNode*)get_page(temp->next_leaf);
    }
done_counting:
    
    if (count == 0) return NULL;
    
    unsigned char** results = (unsigned char**)malloc(sizeof(unsigned char*) * count);
    if (!results) return NULL;
    
    int idx = 0;
    while (leaf && idx < count) {
        for (int i = 0; i < leaf->num_keys && idx < count; i++) {
            if (leaf->entries[i].key >= lowerKey && leaf->entries[i].key <= upperKey) {
                results[idx] = (unsigned char*)malloc(DATA_SIZE);
                if (!results[idx]) {
                    for (int j = 0; j < idx; j++) free(results[j]);
                    free(results);
                    *n = 0;
                    return NULL;
                }
                memcpy(results[idx], leaf->entries[i].data, DATA_SIZE);
                idx++;
            } else if (leaf->entries[i].key > upperKey) {
                *n = count;
                return results;
            }
        }
        if (leaf->next_leaf == -1) break;
        leaf = (LeafNode*)get_page(leaf->next_leaf);
    }
    
    *n = count;
    return results;
}

void freeRangeData(unsigned char** data, int n) {
    if (!data) return;
    for (int i = 0; i < n; i++) {
        if (data[i]) free(data[i]);
    }
    free(data);
}
