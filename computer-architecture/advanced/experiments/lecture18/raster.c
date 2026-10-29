// raster.c：在 CPU 上（只用一個核心）做影片 §rasterizer → fragment → pixel engine 的最小版本：
//   2000 個隨機三角形 → 每個像素用 barycentric 座標內插顏色（Gouraud shading）和深度 z → z-buffer 決定誰看得見
//   畫面 1920 × 1080，量一張要多久、每秒幾張
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define W 1920
#define H 1080
#define T 2000
static float zbuf[W * H];
static unsigned rgb[W * H];
typedef struct { float x, y, z, r, g, b; } V;
static float edge(V a, V b, float x, float y) { return (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x); }
int main(int argc, char **argv)
{
	static V tri[T][3];
	srand(1);
	for (int t = 0; t < T; t++) {
		float cx = rand() % W, cy = rand() % H;
		for (int k = 0; k < 3; k++)
			tri[t][k] = (V){ cx + rand() % 200 - 100, cy + rand() % 200 - 100, (rand() % 1000) / 1000.f,
					 rand() % 256, rand() % 256, rand() % 256 };
	}
	long frag = 0;
	struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int i = 0; i < W * H; i++) { zbuf[i] = 1e9f; rgb[i] = 0; }
	for (int t = 0; t < T; t++) {
		V a = tri[t][0], b = tri[t][1], c = tri[t][2];
		float area = edge(a, b, c.x, c.y);
		if (area == 0) continue;
		int x0 = a.x < b.x ? a.x : b.x; x0 = x0 < c.x ? x0 : c.x; if (x0 < 0) x0 = 0;
		int x1 = a.x > b.x ? a.x : b.x; x1 = x1 > c.x ? x1 : c.x; if (x1 > W - 1) x1 = W - 1;
		int y0 = a.y < b.y ? a.y : b.y; y0 = y0 < c.y ? y0 : c.y; if (y0 < 0) y0 = 0;
		int y1 = a.y > b.y ? a.y : b.y; y1 = y1 > c.y ? y1 : c.y; if (y1 > H - 1) y1 = H - 1;
		for (int y = y0; y <= y1; y++)
			for (int x = x0; x <= x1; x++) {
				float w0 = edge(b, c, x, y) / area, w1 = edge(c, a, x, y) / area, w2 = 1 - w0 - w1;
				if (w0 < 0 || w1 < 0 || w2 < 0) continue;      /* 不在三角形裡（rasterization） */
				frag++;
				float z = w0 * a.z + w1 * b.z + w2 * c.z;       /* 內插深度 */
				if (z >= zbuf[y * W + x]) continue;             /* 被擋住了（z-buffer） */
				zbuf[y * W + x] = z;
				unsigned r = w0 * a.r + w1 * b.r + w2 * c.r, g = w0 * a.g + w1 * b.g + w2 * c.g,
					 bl = w0 * a.b + w1 * b.b + w2 * c.b;  /* Gouraud：內插顏色 */
				rgb[y * W + x] = r << 16 | g << 8 | bl;
			}
	}
	clock_gettime(CLOCK_MONOTONIC, &t1);
	double s = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) * 1e-9;
	long lit = 0; for (int i = 0; i < W * H; i++) lit += rgb[i] != 0;
	printf("一張 %.1f ms（每秒 %.1f 張）；fragment %ld 個，畫面上有顏色的像素 %ld 個（%.0f%%）\n",
	       s * 1e3, 1 / s, frag, lit, 100.0 * lit / (W * H));
	if (argc > 1) {                     /* 給檔名就輸出 PPM 圖檔 */
		FILE *f = fopen(argv[1], "wb"); fprintf(f, "P6 %d %d 255\n", W, H);
		for (int i = 0; i < W * H; i++) { fputc(rgb[i] >> 16, f); fputc(rgb[i] >> 8 & 255, f); fputc(rgb[i] & 255, f); }
		fclose(f);
	}
	return 0;
}
