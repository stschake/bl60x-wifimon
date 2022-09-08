#ifndef RE_UTILITY
#define RE_UTILITY

#include <stdint.h>

struct list_header
{
    struct list_header *next;
};

struct list
{
    struct list_header *first;
    struct list_header *last;
};

static inline uint32_t read_volatile_4(uint32_t addr) {
    return *(volatile uint32_t*)addr;
}

static inline void write_volatile_4(uint32_t addr, uint32_t v) {
    *(volatile uint32_t*)addr = v;
}

static inline void wait_us(uint32_t us)
{
    volatile uint32_t n;
    us = (us >= (1<<24)-1) ? (1<<24)-1 : us;
    n = us << 4;
    while(n--);
}

void hexdump(void *mem, unsigned int len);

#endif
