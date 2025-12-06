#include "bptree.h"
#include <stdio.h>

int main() {
    remove("debug.db");
    initBPTree("debug.db");
    
    unsigned char data[100];
    
    // Insert 100000 keys
    printf("Inserting 100000 keys...\n");
    for (int i = 0; i < 100000; i++) {
        if (i % 10000 == 0) printf("  %d...\n", i);
        sprintf((char*)data, "Data%d", i);
        if (!writeData(i, data)) {
            printf("Failed at key %d\n", i);
            return 1;
        }
    }
    printf("Done inserting\n\n");
    
    // Read them back
    printf("Reading back:\n");
    int found = 0, missing = 0;
    for (int i = 0; i < 100000; i++) {
        if (readData(i, data)) {
            found++;
        } else {
            printf("Missing: %d\n", i);
            missing++;
        }
    }
    
    printf("\nFound: %d, Missing: %d\n", found, missing);
    
    closeBPTree();
    return 0;
}
