#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include <stdint.h>
/* 產生隨機的 normal float 對，用硬體（SSE）在四種捨入模式下相加，印出 16 進位 */
static uint32_t bits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
static float rnd(void)
{
	uint32_t u = ((uint32_t)rand() << 16) ^ rand();
	u = (u & 0x807FFFFF) | ((110 + rand() % 30) << 23);   /* 指數 100..149，避免 denormal／overflow */
	float f; memcpy(&f, &u, 4); return f;
}
int main(void)
{
	int modes[] = { FE_TOWARDZERO, FE_UPWARD, FE_DOWNWARD, FE_TONEAREST };
	srand(7);
	for (int i = 0; i < 100000; i++) {
		volatile float a = rnd(), b = rnd();
		printf("%08x %08x", bits(a), bits(b));
		for (int m = 0; m < 4; m++) {
			fesetround(modes[m]);
			volatile float c = a * b;
			printf(" %08x", bits(c));
		}
		fesetround(FE_TONEAREST);
		printf("\n");
	}
	return 0;
}
