#pragma once

#ifndef _MURMURHASH3_H_
#define _MURMURHASH3_H_



#if defined(_MSC_VER) && (_MSC_VER < 1600)

typedef unsigned int uint32_t;

// Other compilers

#else	// defined(_MSC_VER)

#include <stdint.h>

#endif // !defined(_MSC_VER)

//-----------------------------------------------------------------------------

void MurmurHash3_x86_32(const uint32_t* key, uint32_t seed, uint32_t* out);


#endif
