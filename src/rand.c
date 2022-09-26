#include "rand.h"

// Fair and fast random generation (using xorshift32, with explicit seed)
static uint32_t rand_state = 1;
uint32_t rand() {
	uint32_t x = rand_state;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 5;
	return rand_state = x;
}
float randf() {
	return ((float)rand()) / ((float)(1LL << 32));
}