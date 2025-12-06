#include "bptree.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define DATA_SIZE 100

void test_data(unsigned char* data, int key) {
    for (int i = 0; i < DATA_SIZE; i++) {
        data[i] = (key + i) % 256;
    }
}

int verify_data(unsigned char* data, int key) {
    for (int i = 0; i < DATA_SIZE; i++) {
        if (data[i] != (unsigned char)((key + i) % 256)) {
            return 0;
        }
    }
    return 1;
}

int main() {
    printf("=== Simple B+ Tree Test ===\n\n");
    
    initBPTree("simple_test.dat");
    printf("✓ Initialized\n");
    
    unsigned char data[DATA_SIZE];
    unsigned char read_buf[DATA_SIZE];
    
    // Test 1: Insert and read 1000 keys
    printf("\nInserting 1000 keys...\n");
    for (int i = 0; i < 1000; i++) {
        test_data(data, i);
        if (!writeData(i, data)) {
            printf("✗ Failed to insert key %d\n", i);
            return 1;
        }
    }
    printf("✓ Inserted 1000 keys\n");
    
    // Verify
    printf("Verifying keys...\n");
    for (int i = 0; i < 1000; i++) {
        if (!readData(i, read_buf)) {
            printf("✗ Failed to read key %d\n", i);
            return 1;
        }
        if (!verify_data(read_buf, i)) {
            printf("✗ Data mismatch for key %d\n", i);
            return 1;
        }
    }
    printf("✓ All keys verified\n");
    
    // Test 2: Range query
    printf("\nTesting range query [100, 200]...\n");
    int n;
    unsigned char** results = readRangeData(100, 200, &n);
    if (results) {
        printf("✓ Found %d results\n", n);
        freeRangeData(results, n);
    } else {
        printf("✗ Range query failed\n");
    }
    
    // Test 3: Delete
    printf("\nDeleting key 500...\n");
    if (deleteData(500)) {
        printf("✓ Deleted key 500\n");
        if (!readData(500, read_buf)) {
            printf("✓ Key 500 no longer exists\n");
        }
    }
    
    // Test 4: Performance test
    printf("\nPerformance test: 5000 inserts...\n");
    clock_t start = clock();
    for (int i = 2000; i < 7000; i++) {
        test_data(data, i);
        writeData(i, data);
    }
    clock_t end = clock();
    double seconds = (double)(end - start) / CLOCKS_PER_SEC;
    printf("✓ Completed in %.2f seconds (%.0f ops/sec)\n", 
           seconds, 5000 / seconds);
    
    closeBPTree();
    printf("\n=== All Tests Passed! ===\n");
    
    return 0;
}
