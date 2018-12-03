#ifndef _TDM_BINARY_SEARCH_203876_H_
#define _TDM_BINARY_SEARCH_203876_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tdm_bsb_info {
    intptr_t minus_one;     //constant -1
    intptr_t step_first;    //step on first iteration
    intptr_t step_second;   //step on all subsequent iterations (power of two)
};

//precompute "info" data from the size "num" of sorted array
void binary_search_branchless_precompute(struct tdm_bsb_info *info, uint32_t num);

//run fast binary search over sorted array (note: "info" must be precomputed with array size)
//returns index of the first element greater or equal than "key"
uint32_t binary_search_branchless_run (const struct tdm_bsb_info *info, const uint32_t *arr, uint32_t key);

#ifdef __cplusplus
}
#endif

#endif
