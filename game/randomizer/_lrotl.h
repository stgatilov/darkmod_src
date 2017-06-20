#ifndef _LROTL_H_
#define _LROTL_H_

//note: this code is already nuked anyway
inline uint32 my_lrotl (uint32 x, int r) {
	return (x << r) | (x >> ((sizeof(x) * 8) - r));
}

#endif /*_LROTL_H_*/
