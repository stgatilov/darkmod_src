#ifndef _LROTL_H_
#define _LROTL_H_

/* Left rotate function for Linux */
#ifdef __linux__
inline uint32 _lrotl (uint32 x, int r) {
	return (x << r) | (x >> (sizeof(x) * 8) - r);
}
#endif

#endif /*_LROTL_H_*/
