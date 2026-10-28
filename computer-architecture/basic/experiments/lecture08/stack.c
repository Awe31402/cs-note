#include <stdio.h>
/* 每一層遞迴印出區域變數的位址與 return address：stack 往低位址長 */
__attribute__((noinline)) int factorial(int num)
{
	int local = num;
	printf("num=%2d  &local=%p  ret=%p\n", num, (void *)&local, __builtin_return_address(0));
	if (num <= 1)
		return 1;
	return num * factorial(num - 1);
}

int main(void)
{
	printf("result = %d\n", factorial(5));
	return 0;
}
