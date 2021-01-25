#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

void* mymalloc(size_t size) {
    void* p;
    p = sbrk(0);

    if (sbrk(size) == (void*) -1) return 0;

    return p;
}

int main(void) {
    int* ptr = NULL;

    ptr = mymalloc(sizeof(int) * 10);

    for (int i = 0; i < 10; i++) {
        ptr[i] = i;
        printf("%d\n", ptr[i]);
    }

    return 0;
}
