#include"murmurhash.h"
uint32_t rotl32(uint32_t x, char r)
{
	return (x << r) | (x >> (32 - r));
}

uint32_t fmix32(uint32_t h)
{
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}
void MurmurHash3_x86_32(const uint32_t *key, uint32_t seed, uint32_t *out)
{

	uint32_t h1 = seed;

	const uint32_t c1 = 0xcc9e2d51;
	const uint32_t c2 = 0x1b873593;


	uint32_t k1 = *key;

	k1 *= c1;
	k1 = rotl32(k1, 15);
	k1 *= c2;

	h1 ^= k1;
	h1 = rotl32(h1, 13);
	h1 = h1 * 5 + 0xe6546b64;

	h1 ^= 4;

	h1 = fmix32(h1);

	*(uint32_t*)out = h1;
}
