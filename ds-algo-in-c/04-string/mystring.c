#include "mystring.h"

#include <limits.h>
#include <string.h>

size_t s_len(const char *s)
{
    const char *p = s;
    while (*p)
        p++;
    return (size_t)(p - s);
}

int s_cmp(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    /* 轉 unsigned char 再相減：char 可能是 signed，中文等 > 127 的位元組會變負數 */
    return (unsigned char)*a - (unsigned char)*b;
}

void s_reverse(char *s)
{
    size_t n = s_len(s);
    if (n < 2)
        return;
    for (size_t l = 0, r = n - 1; l < r; l++, r--) {
        char t = s[l];
        s[l] = s[r];
        s[r] = t;
    }
}

void s_to_upper(char *s)
{
    for (; *s; s++)
        if (*s >= 'a' && *s <= 'z')
            *s = (char)(*s - 'a' + 'A');
}

size_t s_copy(char *dst, const char *src, size_t cap)
{
    size_t n = s_len(src);
    if (cap > 0) {
        size_t k = n < cap - 1 ? n : cap - 1;
        memcpy(dst, src, k);
        dst[k] = '\0';
    }
    return n; /* 回傳值 >= cap 代表被截斷 */
}

int s_index(const char *text, const char *pat)
{
    size_t n = s_len(text), m = s_len(pat);
    if (m > n)
        return -1;
    for (size_t i = 0; i + m <= n; i++) {
        size_t j = 0;
        while (j < m && text[i + j] == pat[j])
            j++;
        if (j == m)
            return (int)i;
    }
    return -1;
}

int s_substr(const char *s, int pos, int len, char *out, size_t cap)
{
    int n = (int)s_len(s);
    if (pos < 0 || len < 0 || pos + len > n || (size_t)len + 1 > cap)
        return -1;
    memcpy(out, s + pos, (size_t)len);
    out[len] = '\0';
    return 0;
}

int s_insert(char *s, size_t cap, int pos, const char *ins)
{
    size_t n = s_len(s), m = s_len(ins);
    if (pos < 0 || (size_t)pos > n || n + m + 1 > cap)
        return -1;
    memmove(s + pos + m, s + pos, n - (size_t)pos + 1); /* 連 '\0' 一起往後搬 */
    memcpy(s + pos, ins, m);
    return 0;
}

void s_delete(char *s, int pos, int len)
{
    int n = (int)s_len(s);
    if (pos < 0 || pos >= n || len <= 0)
        return;
    if (pos + len > n)
        len = n - pos;
    memmove(s + pos, s + pos + len, (size_t)(n - pos - len) + 1);
}

int s_atoi(const char *s, int *out)
{
    while (*s == ' ' || *s == '\t' || *s == '\n')
        s++;
    int neg = 0;
    if (*s == '+' || *s == '-')
        neg = *s++ == '-';
    if (*s < '0' || *s > '9')
        return -1;
    long long v = 0;
    for (; *s >= '0' && *s <= '9'; s++) {
        v = v * 10 + (*s - '0');
        if (v > (long long)INT_MAX + 1) /* 先擋住，避免 long long 也溢位 */
            return -1;
    }
    if (*s != '\0')
        return -1;
    if (neg)
        v = -v;
    if (v > INT_MAX || v < INT_MIN)
        return -1;
    *out = (int)v;
    return 0;
}
