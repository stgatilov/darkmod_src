#ifndef _TDM_POLYHASH_H_665168_
#define _TDM_POLYHASH_H_665168_

#include <stdint.h>

#ifdef _MSC_VER
#define INLINE __inline
#else
#define INLINE inline
#endif


#ifdef __cplusplus
extern "C" {
#endif

static const uint32_t POLYHASH_MODULO = 0x7FFFFFFF;     //P: Mersenne prime, modulo operations are fast
static const uint32_t POLYHASH_BASE = 103945823;        //B: primitive root modulo POLYHASH_MODULO
//theory: if B is chosen randomly, then any two different windows
//        generate hash collision with chance at most (len/P)

extern uint32_t POLYHASH_NEGATOR;

//compute hash value of the specified bytes array (window)
//note: also precomputes and saves POLYHASH_NEGATOR coefficient
uint32_t polyhash_compute(const uint8_t *data, size_t len);

//recompute hash value after moving window forward by one byte
//"value" is hash of the current window, hash of the next window is returned
//"added" is the byte entering window, "removed" byte is leaving window
//note: POLYHASH_NEGATOR must be precomputed via polyhash_compute beforehand
static INLINE uint32_t polyhash_fast_update(uint32_t value, uint8_t added, uint8_t removed) {
  value = (((uint64_t)value) * POLYHASH_BASE + added + ((uint64_t)removed) * POLYHASH_NEGATOR) % POLYHASH_MODULO;
  return value;
}

#ifdef __cplusplus
}
#endif

#endif
