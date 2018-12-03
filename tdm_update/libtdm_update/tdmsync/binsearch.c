#include "binsearch.h"
#include <stdio.h>

void binary_search_branchless_precompute(struct tdm_bsb_info *info, uint32_t num) {
    info->minus_one = -1;

    uint32_t n_bsr = 0;
    for (int pos = 31; pos >= 0; pos--) {
        if ((num >> pos) & 1) {
            n_bsr = pos;
            break;
        }
    }

    info->step_second = (1U << n_bsr);
    info->step_first = num + 1 - info->step_second;
    info->step_second >>= 1;
    if (num == 0)
        info->step_first = info->step_second = 0;
}

uint32_t binary_search_branchless_run (const struct tdm_bsb_info *info, const uint32_t *arr, uint32_t key) {
    intptr_t pos = info->minus_one;

    intptr_t step = info->step_first;
    if (step > 0) {
        pos = (arr[pos + step] < key ? pos + step : pos);

        step = info->step_second;
        while (step > 0) {
            pos = (arr[pos + step] < key ? pos + step : pos);
            step >>= 1;
        }
    }

    return pos + 1;    
}
