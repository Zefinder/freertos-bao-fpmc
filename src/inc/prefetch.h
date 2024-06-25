#ifndef __PREFETCH_H__
#define __PREFETCH_H__

// TODO Documentation

void clear_L2_cache(uint64_t address, uint64_t size);
void clear_L2_cache_CISW(uint64_t first_way, uint64_t last_way, uint64_t first_set, uint64_t last_set);
void prefetch_data(uint64_t address, uint64_t size);
void prefetch_data_prem(uint64_t address, uint64_t size, volatile uint8_t* suspend_prefetch);

#endif