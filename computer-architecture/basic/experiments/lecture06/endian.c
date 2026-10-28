#include <stdio.h>
int main(void)
{
	unsigned int x = 0x87654321;
	unsigned char *p = (unsigned char *)&x;
	printf("addr+0..3: %02x %02x %02x %02x\n", p[0], p[1], p[2], p[3]);
	printf("bswap: %08x\n", __builtin_bswap32(x));
	return 0;
}
