#include "nvm-common.h"

long long update[100];
long long numa = 0;
int internal_num = 0;
int internal_node_num = 0;
long long lock_awaiting_times = 0;

long long split_time = 0;
long long flush_times = 0;


long long to_buffer_num = 0;
long long flush_num = 0;
long long flush_5_nodes = 0;
long long lt_split_time = 0;
long long leaf_time = 0;
long long insert_time = 0;
long long leaf_search_time = 0;
long long leaf_search_times = 0;
long long new_obj_time = 0;
long long operation_time = 0;
long long persist_time = 0;
long long search_per_thread = 0;
long long leaf_search_per_thread = 0;
long long leaf_prepare_per_thread = 0;
long long insert_per_thread = 0;
long long split_per_thread = 0;
long long multi_thread_one_node = 0;
long long one_insert_num = 0;
long long thread_from_data[32];
long long back_flushes = 0;
long long total_flushes = 0;
long long second_buffer_fail = 0;


uint64_t data_from = 0;
uint64_t data_to = 0;
int statistic_tid = 3;


#define CPU_FREQ_MHZ (1994)
#define DELAY_IN_NS (1000)
#define CACHE_LINE_SIZE 64
#define QUERY_NUM 25
#define IS_FORWARD(c) (c % 2 == 0)
#define CROSS_NUM 32
/*
--------------------------------------------------------------------------------

            nvm tools

--------------------------------------------------------------------------------
*/


#define ALIGN(addr, alignment) ((char *)((unsigned long)(addr) & ~((alignment) - 1)))
#define CACHELINE_ALIGN(addr) ALIGN(addr, 64)

int nvm_dram_alloc(void **ptr, size_t align, size_t size)
{
    // assert(size < 1073741824UL);
    // int ret = posix_memalign(ptr, align, size);
    *ptr = nvmpool_alloc(size);
    // *ptr = nvmpool_alloc(size);
    // ptr = alloc(size);
    return 0;
}

void clflush_then_sfence(volatile void *p)
{
    volatile char *ptr = CACHELINE_ALIGN(p);
    asm volatile("clwb %0": :"m"(*ptr));
    sfence();
}

void clflush_len(volatile void *data, int len)
{
    volatile char *ptr = CACHELINE_ALIGN(data);
    for (; ptr < (char *)data + len; ptr += CACHE_LINE_SIZE)
    {
#ifdef COUNT_CLFLUSH
        clflush_count.fetch_add(1);
#endif
        asm volatile("clwb %0"
                     : "+m"(*(volatile char *)ptr));
    }
#ifdef COUNT_CLFLUSH
    sfence();
    sfence_count.fetch_add(1);
#endif
}

void clflush_len_no_fence(volatile void *data, int len) {
    volatile char *ptr = CACHELINE_ALIGN(data);
    for (; ptr < (char *)data + len; ptr += CACHE_LINE_SIZE)
    {
#ifdef COUNT_CLFLUSH
        clflush_count.fetch_add(1);
#endif
        asm volatile("clwb %0"
        : "+m"(*(volatile char *)ptr));
        // sfence();
    }
    sfence();
}

static inline unsigned char hashcode1B(int64_t x) {
    x ^= x>>32;
    x ^= x>>16;
    x ^= x>>8;
    return (unsigned char)(x&0x0ffULL);
}


static inline void cpu_pause() { __asm__ volatile("pause" ::: "memory"); }
static inline unsigned long read_tsc(void) {
    unsigned long var;
    unsigned int hi, lo;

    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    var = ((unsigned long long int)hi << 32) | lo;

    return var;
}

unsigned long write_latency_in_ns = 1300;
int clflush_cnt = 0;
int node_cnt = 0;

// using namespace std;

int a = 0;

inline void mfence() { asm volatile("mfence" ::: "memory"); }


inline void clflush(char *data, int len) {
    volatile char *ptr = (char *)((unsigned long)data & ~(CACHE_LINE_SIZE - 1));
    mfence();
    for (; ptr < data + len; ptr += CACHE_LINE_SIZE) {
        // unsigned long etsc =
        //         read_tsc() + (unsigned long)(write_latency_in_ns * CPU_FREQ_MHZ / 1000);
        asm volatile("clflush %0" : "+m"(*(volatile char *)ptr));
        // while (read_tsc() < etsc)
        //     cpu_pause();
        //++clflush_cnt;
    }
    mfence();
}




