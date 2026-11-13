#ifndef MYSTRING_H
#define MYSTRING_H

/* 檔名不叫 string.h：Makefile 會用 -I 加入主題資料夾，同名檔案會蓋掉系統的 <string.h>。
 * C 字串 = char 陣列 + 結尾的 '\0'。 */
#include <stddef.h>

size_t s_len(const char *s);                          /* 同 strlen，O(n) */
int    s_cmp(const char *a, const char *b);           /* 同 strcmp：<0 / 0 / >0 */
void   s_reverse(char *s);                            /* 原地反轉 */
void   s_to_upper(char *s);                           /* 'a'..'z' → 'A'..'Z' */
size_t s_copy(char *dst, const char *src, size_t cap); /* 安全複製：最多寫 cap-1 字 + '\0'，回傳 src 長度 */
int    s_index(const char *text, const char *pat);    /* 暴力找子字串，O(nm)；找不到 -1，pat 為空回傳 0 */
int    s_substr(const char *s, int pos, int len, char *out, size_t cap); /* 成功 0；越界或放不下 -1 */
int    s_insert(char *s, size_t cap, int pos, const char *ins);          /* 在 pos 插入；放不下 -1 */
void   s_delete(char *s, int pos, int len);                              /* 從 pos 刪 len 個（超過就刪到尾） */
int    s_atoi(const char *s, int *out); /* 可有前導空白與正負號；成功 0，非數字或溢位 -1 */

#endif
