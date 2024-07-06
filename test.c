#define RSALLOC_IMPLEMENTATION
#include "RSAlloc.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

int main(void) {
    srand(time(NULL));

    RSA_init(0);

    u32* array = RSA_alloc(20 * sizeof(u32));
    if (array == NULL)
        return 0;
    
    size_t i;
    for (i = 0; i < 20; i++) {
        printf(RSA_PRINT_U64 " : %i\n", i, array[i]);
    }

    RSA_free(array);
    
    for (i = 0; i < 10000; i++) {
        u32* array = RSA_alloc((rand() % 100));
        if (array == NULL) {
            printf("failed to alloc array\n");
            return 0;
        }
        
        if (rand() % 2) {
            RSA_free(array);
        }
    }
    
    
    RSA_deInit();
}
