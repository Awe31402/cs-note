#include "mystring.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static int sign(int x) { return (x > 0) - (x < 0); }

static void test_basic(void)
{
    assert(s_len("") == 0);
    assert(s_len("hello") == 5);

    const char *words[] = {"", "a", "ab", "abc", "abd", "b", "\xe4\xb8\xad"}; /* 最後一個是「中」的 UTF-8 */
    for (int i = 0; i < 7; i++)
        for (int j = 0; j < 7; j++)
            assert(sign(s_cmp(words[i], words[j])) == sign(strcmp(words[i], words[j])));

    char a[] = "hello", b[] = "", c[] = "ab";
    s_reverse(a);
    s_reverse(b);
    s_reverse(c);
    assert(strcmp(a, "olleh") == 0 && strcmp(b, "") == 0 && strcmp(c, "ba") == 0);

    char u[] = "Hi, c99!";
    s_to_upper(u);
    assert(strcmp(u, "HI, C99!") == 0);
}

static void test_copy(void)
{
    char buf[6];
    assert(s_copy(buf, "hi", sizeof buf) == 2 && strcmp(buf, "hi") == 0);
    assert(s_copy(buf, "overflow", sizeof buf) == 8); /* 8 >= 6：被截斷 */
    assert(strcmp(buf, "overf") == 0);                /* 但一定有 '\0' */
}

static void test_index_substr(void)
{
    assert(s_index("hello world", "world") == 6);
    assert(s_index("hello", "") == 0);
    assert(s_index("aaaab", "aab") == 2);
    assert(s_index("abc", "abcd") == -1);
    assert(s_index("abc", "x") == -1);

    char out[8];
    assert(s_substr("abcdef", 1, 3, out, sizeof out) == 0 && strcmp(out, "bcd") == 0);
    assert(s_substr("abcdef", 4, 3, out, sizeof out) == -1); /* 越界 */
    assert(s_substr("abcdef", 0, 0, out, sizeof out) == 0 && out[0] == '\0');
}

static void test_insert_delete(void)
{
    char s[16] = "HelloWorld";
    assert(s_insert(s, sizeof s, 5, ", ") == 0);
    assert(strcmp(s, "Hello, World") == 0);
    assert(s_insert(s, sizeof s, 0, ">>>>") == -1); /* 12 + 4 + 1 > 16 */
    assert(s_insert(s, sizeof s, (int)strlen(s), "!") == 0);
    assert(strcmp(s, "Hello, World!") == 0);

    s_delete(s, 5, 2);
    assert(strcmp(s, "HelloWorld!") == 0);
    s_delete(s, 5, 100); /* 超過就刪到尾 */
    assert(strcmp(s, "Hello") == 0);
    s_delete(s, 9, 1); /* 越界不動 */
    assert(strcmp(s, "Hello") == 0);
}

static void test_atoi(void)
{
    int v;
    assert(s_atoi("42", &v) == 0 && v == 42);
    assert(s_atoi("   -17", &v) == 0 && v == -17);
    assert(s_atoi("+0", &v) == 0 && v == 0);
    assert(s_atoi("2147483647", &v) == 0 && v == INT_MAX);
    assert(s_atoi("-2147483648", &v) == 0 && v == INT_MIN);
    assert(s_atoi("2147483648", &v) == -1);
    assert(s_atoi("99999999999999999999999", &v) == -1);
    assert(s_atoi("12a", &v) == -1);
    assert(s_atoi("", &v) == -1);
    assert(s_atoi("-", &v) == -1);
}

int main(void)
{
    test_basic();
    test_copy();
    test_index_substr();
    test_insert_delete();
    test_atoi();
    puts("mystring: all tests passed");
    return 0;
}
