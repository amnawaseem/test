#ifndef _SCHISM_RANGE_H_
#define _SCHISM_RANGE_H_

#include <schism/util.h>

void *init_range_manager(uint64_t base, uint64_t size, uint32_t count);
int claim_range_all(void *rm, uint64_t base, uint64_t size);
int claim_range_one(void *rm, uint32_t slot, uint64_t base, uint64_t size);


int find_range_one(void *rm, uint32_t slot, uint64_t size, uint64_t *base_ret);
int find_range_one_aligned(void *rm, uint32_t slot, uint64_t size, uint32_t align, uint64_t *base_ret);
int find_range_one_aligned_padded(void *rm, uint32_t slot, uint64_t size, uint32_t align,
                                uint64_t pad, uint64_t *base_ret);
int find_range_one_aligned_padded_window(void *rm, uint32_t slot, uint64_t size, uint32_t align,
                                uint64_t pad, uint64_t *window, uint64_t *base_ret);


int find_range_all(void *rm, uint64_t size, uint64_t *base_ret);
int find_range_all_aligned(void *rm, uint64_t size, uint32_t align, uint64_t *base_ret);
int find_range_all_aligned_padded(void *rm, uint64_t size, uint32_t align,
                                uint64_t pad, uint64_t *base_ret);
int find_range_all_aligned_padded_window(void *rm, uint64_t size, uint32_t align,
                                uint64_t pad, uint64_t *window, uint64_t *base_ret);

#endif
