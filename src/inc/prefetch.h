#ifndef __PREFETCH_H__
#define __PREFETCH_H__

// TODO Documentation

void clear_L2_cache(uint64_t address, uint64_t size);
void prefetch_data(uint64_t address, uint64_t size);
void prefetch_data_prem(uint64_t address, uint64_t size, uint8_t* suspend_prefetch);

#endif