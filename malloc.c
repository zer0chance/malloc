#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct s_block *t_block;

void* base = NULL;

struct s_block 
{
    size_t  size;
    t_block next;
    t_block prev;
    int     free;
    void*   ptr;
    char    data[1];
};

const size_t BLOCK_SIZE = sizeof(struct s_block) - sizeof(char*);
#define align4(x) (((((x) - 1) >> 2) << 2) + 4)

t_block find_block(t_block last, size_t size)
{
    t_block b = base;

    while (b != NULL && !(b->free && b->size <= size))
    {
        last = b;
        b = b->next;
    }

    return b;
}

t_block extend_heap(t_block last, size_t size)
{
    int sb;
    t_block b;
    b = (t_block) sbrk(0);
    sb = (int) sbrk(BLOCK_SIZE + size);
    
    if (sb < 0) return NULL;

    b->size = size;
    b->free = 0;
    b->next = NULL;
    b->prev = last;
    b->ptr  = b->data;
    if (last) last->next = b;

    return b;
}

void split_block(t_block b, size_t size)
{
    t_block new;

    new       = (t_block) b->data + size;
    new->size = b->size - size - BLOCK_SIZE;
    new->next = b->next;
    new->prev = b;
    new->free = 1;
    new->ptr  = new->data;
    b->size   = size;
    b->next   = new;
    if (new->next) new->next->prev = new;
}

void* __malloc(size_t size) {
    t_block b, last;
    size_t s = align4(size);

    if (base == NULL) {
        b = extend_heap(NULL, s);
        if (b == NULL) return NULL;
        base = b;
    } else {
        last = base;
        b = find_block(last, s);
        if (b != NULL) {
            if (b->size - s > BLOCK_SIZE + 4)
                split_block(b, s);
            b->free = 0;
        } else {
            b = extend_heap(last, s);
            if (b == NULL) return NULL;
        }
    }

    return b->data;
}

void* __calloc(size_t number, size_t size) {
    size_t *new;
    size_t s4;

    new = __malloc(number * size);
    
    if (new != NULL) {
       s4 = align4(number * size) << 2;
       for (int i = 0; i < s4; i++) new[i] = 0;
    }

    return new;
}

t_block fusion(t_block b) {
    if (b->next != NULL && b->next->free) {
        b->size += b->next->size + BLOCK_SIZE;
        b->next = b->next->next;

        if (b->next != NULL) {
            b->next->prev = b;
        }
    }
    
    return b;
}

t_block get_block(void* p) {
    char* tmp = p;
    tmp -= BLOCK_SIZE;
    return (t_block) tmp;
}

int valid_adress(void* p) {
    if (base != NULL) {
        if (p > base && p < sbrk(0)) 
            return (p == get_block(p)->ptr);
    }

    return 0;
}

void __free(void* p) {
    t_block b;
    if (valid_adress(p)) {
        b = get_block(p);
        b->free = 1;

        if (b->prev != NULL && b->prev->free) {
            fusion(b->prev);
        }

        if (b->next != NULL) {
            fusion(b);
        } else {
            if (b->prev != NULL) 
                b->prev->next = NULL;
            else 
                base = NULL;
            brk(b);
        }
    }
}

void copy_block(t_block src, t_block dst) {
    int* sdata;
    int* ddata;

    sdata = (int*) src->data;
    ddata = (int*) dst->data;

    for (int i = 0; i * 4 < src->size && i * 4 < dst->size; i++) {
        ddata[i] = sdata[i];
    }
}

void* __realloc(void* p, size_t size) {
    size_t s;
    t_block b, new;
    void* newp;

    if (p == NULL)
        return __malloc(size);
    if (valid_adress(p)) {
        s = align4(size);
        b = get_block(p);

        if (b->size >= s) {
            if (b->size - s >= BLOCK_SIZE + 4)
                split_block(b, s);
        } else {
            if (b->next != NULL && b->next->free && b->size + BLOCK_SIZE + b->next->size >= s) {
                fusion(b);
                if (b->size - s >= BLOCK_SIZE + 4)
                    split_block(b, s);
            } else {
                newp = __malloc(s);
                if (newp == NULL) return NULL;

                new = get_block(newp);
                copy_block(b, new);
                __free(p);
                return newp;
            }
        }
        return p;
    }
    return NULL;
}

int main(void) {
    int* ptr = NULL;
    int* rptr = NULL;

    printf("__calloc 10 ints:\n");

    ptr = __calloc(10, sizeof(int));
    if (ptr == NULL) {
        printf("Failed to __calloc!\n");
        exit(-1);
    }

    for (int i = 0; i < 10; i++) {
        ptr[i] = i;
        printf("%d\n", ptr[i]);
    }

    printf("__realloc to 20 ints:\n");

    rptr = __realloc(ptr, sizeof(int) * 20);
    if (rptr == NULL) {
        printf("Failed to __realloc!\n");
        exit(-1);
    }

    for (int i = 10; i < 20; i++) {
        rptr[i] = i;
        printf("%d\n", rptr[i]);
    }

    __free(ptr);
    __free(rptr);

    return 0;
}

