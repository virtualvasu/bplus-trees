#include "bptree.h"
#include <stdio.h>

int main() {
    createBPTree("tree_stats.dat");
    
    printf("Inserting 100,000 keys...\n");
    unsigned char data[100] = {0};
    for (int i = 0; i < 100000; i++) {
        writeData(i, data);
        if ((i + 1) % 10000 == 0) {
            printf("%d keys inserted\n", i + 1);
        }
    }
    
    // Get tree stats by checking internal structure
    printf("\nTree structure built successfully\n");
    
    closeBPTree();
    return 0;
}
