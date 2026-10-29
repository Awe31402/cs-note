// race.c：課本圖 9.57 的兩個程式（影片 [37:13]–[44:53]）
//   counter：兩個 thread 各做 x++ 一百萬次（data race）
//   flag：T1：x = 57; y = 1　T2：while (y != 1) {}; t = x
//     PLAIN 版：y 是一般的 int（data race）；ATOMIC 版：y 用 release／acquire（sync 變數，沒有 data race）
#include <pthread.h>
#include <stdio.h>
static long x;
static volatile long counter;                 // volatile：不讓 compiler 把迴圈合成一次加法
#ifdef ATOMIC
static int y;
#define SET_Y() __atomic_store_n(&y, 1, __ATOMIC_RELEASE)
#define GET_Y() __atomic_load_n(&y, __ATOMIC_ACQUIRE)
#else
static int y;
#define SET_Y() (y = 1)
#define GET_Y() (y)
#endif
static void *inc(void *a) { for (int i = 0; i < 1000000; i++) counter++; return a; }
static void *producer(void *a) { x = 57; SET_Y(); return a; }
static void *consumer(void *a) { while (GET_Y() != 1) ; printf("t = x = %ld\n", x); return a; }
int main(void)
{
	pthread_t a, b;
	setvbuf(stdout, 0, _IONBF, 0);
	pthread_create(&a, 0, inc, 0); pthread_create(&b, 0, inc, 0);
	pthread_join(a, 0); pthread_join(b, 0);
	printf("counter = %ld（應該是 2000000）\n", counter);
	pthread_create(&b, 0, consumer, 0); pthread_create(&a, 0, producer, 0);
	pthread_join(a, 0); pthread_join(b, 0);
	return 0;
}
