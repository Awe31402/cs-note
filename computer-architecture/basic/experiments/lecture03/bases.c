// bases.c：影片和課本的進位轉換例子，用 C 的 printf 檢查；另外示範 C 的 0 開頭就是八進位
#include <stdio.h>

static void bin(unsigned v, int n) { for (int i = n - 1; i >= 0; i--) putchar('0' + ((v >> i) & 1)); }

int main(void)
{
	printf("19      = "); bin(19, 5); printf("（二進位）\n");
	printf("0x32    = %d\n", 0x32);
	printf("042     = %d   ← C 的 042 是八進位\n", 042);
	printf("42      = %d   ← 沒有 0 才是十進位\n", 42);
	printf("110001010b = 0%o（八進位） 課本 Example 9\n", 0x18A);
	printf("110000101011b = 0x%X 課本 Example 10\n", 0xC2B);
	printf("1110b   = %d = 0x%X\n", 0xE, 0xE);
	printf("sizeof: short %zu, int %zu, long long %zu bytes\n", sizeof(short), sizeof(int), sizeof(long long));
	printf("unsigned short 最大 %u，unsigned int 最大 %u\n", (unsigned short)~0u, ~0u);
	return 0;
}
