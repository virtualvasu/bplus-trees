#include "bptree.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

void fill_data(unsigned char* data, int key) {
    snprintf((char*)data, 100, "Data for key %d - filled with test content", key);
}

int main() {
    printf("=== LARGE SCALE PERFORMANCE TEST ===\n\n");
    
    remove("large_test.db");
    initBPTree("large_test.db");
    
    // Test 1: Insert 100K keys
    printf("[1] Inserting 100,000 keys...\n");
    unsigned char data[100];
    clock_t start = clock();
    
    for (int i = 0; i < 100000; i++) {
        fill_data(data, i);
        if (!writeData(i, data)) {
            printf("    ERROR: Failed to insert key %d\n", i);
            break;
        }
        
        if ((i + 1) % 10000 == 0) {
            double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
            printf("    Progress: %d keys in %.2f sec (%.0f ops/sec)\n",
                   i + 1, elapsed, (i + 1) / elapsed);
        }
    }
    
    double insert_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("    ✓ Inserted 100,000 keys in %.2f seconds\n", insert_time);
    printf("    ✓ Throughput: %.0f inserts/sec\n\n", 100000.0 / insert_time);
    
    // Test 2: Random reads
    printf("[2] Reading 50,000 random keys...\n");
    start = clock();
    int success = 0;
    unsigned char read_buf[100];
    
    for (int i = 0; i < 50000; i++) {
        int key = rand() % 100000;
        if (readData(key, read_buf)) {
            success++;
        }
    }
    
    double read_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("    ✓ Read %d/%d keys in %.2f seconds\n", success, 50000, read_time);
    printf("    ✓ Throughput: %.0f reads/sec\n\n", 50000.0 / read_time);
    
    // Test 3: Range queries
    printf("[3] Range query tests...\n");
    
    start = clock();
    int n1;
    unsigned char** results1 = readRangeData(1000, 2000, &n1);
    double range1_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("    Range [1000-2000]: %d results in %.3f sec\n", n1, range1_time);
    if (results1) freeRangeData(results1, n1);
    
    start = clock();
    int n2;
    unsigned char** results2 = readRangeData(50000, 60000, &n2);
    double range2_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("    Range [50000-60000]: %d results in %.3f sec\n", n2, range2_time);
    if (results2) freeRangeData(results2, n2);
    
    start = clock();
    int n3;
    unsigned char** results3 = readRangeData(0, 10000, &n3);
    double range3_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("    Range [0-10000]: %d results in %.3f sec\n\n", n3, range3_time);
    if (results3) freeRangeData(results3, n3);
    
    // Test 4: Updates
    printf("[4] Updating 10,000 keys...\n");
    start = clock();
    
    for (int i = 0; i < 10000; i++) {
        fill_data(data, i + 1000000);  // New data
        writeData(i * 10, data);
    }
    
    double update_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("    ✓ Updated 10,000 keys in %.2f seconds\n", update_time);
    printf("    ✓ Throughput: %.0f updates/sec\n\n", 10000.0 / update_time);
    
    closeBPTree();
    
    // Test 5: Persistence
    printf("[5] Testing persistence...\n");
    start = clock();
    initBPTree("large_test.db");
    double reopen_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("    ✓ Reopened database in %.3f seconds\n", reopen_time);
    
    // Verify some random keys
    int verified = 0;
    for (int i = 0; i < 1000; i++) {
        int key = rand() % 100000;
        if (readData(key, read_buf)) {
            verified++;
        }
    }
    printf("    ✓ Verified %d/1000 random keys\n\n", verified);
    
    closeBPTree();
    
    printf("=== ALL TESTS COMPLETED ===\n");
    printf("Total throughput: %.0f ops/sec\n", 
           160000.0 / (insert_time + read_time + update_time));
    
    return 0;
}
