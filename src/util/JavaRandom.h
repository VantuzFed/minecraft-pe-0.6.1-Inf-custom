#ifndef BETA_JAVA_RANDOM_H__
#define BETA_JAVA_RANDOM_H__

// Bit-compatible port of java.util.Random (48-bit LCG) from
// Minecraft Beta 1.7.3. Required so the Beta 1.7.3 generator produces
// exactly the same world for the same seed as the original game.
//
// java.util.Random algorithm:
//   seed = (seed ^ 0x5DEECE66DL) & ((1L << 48) - 1)
//   next(bits): seed = (seed * 0x5DEECE66DL + 0xBL) & mask; return seed >>> (48 - bits)

#include <stdint.h>

class JavaRandom {
public:
	JavaRandom() { setSeed(0); }
	explicit JavaRandom(int64_t seed) { setSeed(seed); }

	void setSeed(int64_t seed) {
		_seed = (seed ^ INT64_C(0x5DEECE66D)) & INT64_C(0xFFFFFFFFFFFF);
	}

	int32_t next(int bits) {
		_seed = (_seed * INT64_C(0x5DEECE66D) + INT64_C(0xB)) & INT64_C(0xFFFFFFFFFFFF);
		return (int32_t)(_seed >> (48 - bits));
	}

	int32_t nextInt() { return next(32); }

	// Exact java.util.Random.nextInt(n) including the power-of-two fast path
	// and the rejection loop for other bounds.
	int32_t nextInt(int32_t n) {
		if (n <= 0) return 0;
		if ((n & -n) == n)
			return (int32_t)(((int64_t)n * (int64_t)next(31)) >> 31);
		int32_t bits, val;
		do {
			bits = next(31);
			val = bits % n;
		} while (bits - val + (n - 1) < 0);
		return val;
	}

	int64_t nextLong() {
		return ((int64_t)next(32) << 32) + (int64_t)next(32);
	}

	float nextFloat() {
		// NOTE: Java does float division here (next(24) / 16777216f),
		// NOT double division rounded to float — 1-ulp differences matter.
		return (float)next(24) / 16777216.0f;
	}

	double nextDouble() {
		return (double)(((int64_t)next(26) << 27) + (int64_t)next(27)) / 9007199254740992.0;
	}

	bool nextBoolean() { return next(1) != 0; }

private:
	int64_t _seed;
};

#endif
