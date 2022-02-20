#ifndef BitVector256_h
#define BitVector256_h

#include <stdint.h> // uint16_t
#include <cstring> // memset

class BitVector256 {
public:
	static const unsigned VECTOR_SIZE = 16;

	BitVector256() {
		clearAll();
	}

	inline void set(unsigned x, unsigned y) {
		if (x < VECTOR_SIZE && y < VECTOR_SIZE)
			vector[x] |= 1 << y;
	}

	inline void clear(unsigned x, unsigned y) {
		if (x < VECTOR_SIZE && y < VECTOR_SIZE)
			vector[x] &= ~(1 << y);
	}

	inline bool get(unsigned x, unsigned y) const {
		if (x < VECTOR_SIZE && y < VECTOR_SIZE)
			return (vector[x] & 1 << y) != 0;

		return 0;
	}

	inline void clearAll() {
		memset(vector, 0, sizeof(vector));
	}

	inline void setAll() {
		memset(vector, ~0, sizeof(vector));
	}

protected:
	uint16_t vector[(VECTOR_SIZE * VECTOR_SIZE) / (8 * sizeof(uint16_t))];
};

#endif