/* LeetCode 49 · Group Anagrams
 * 思路：anagram 排序後長得一樣 → 用「排序後的字串」當 key，同 key 的放同一組。
 *       只有小寫字母，排序用計數排序，O(L)。雜湊表：字串雜湊 (FNV-1a) + 線性探測，存「組別編號」。
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
#define BITS 15
#define CAP (1 << BITS) /* 最多 10^4 組 */

static unsigned hash_str(const char *s)
{
    unsigned h = 2166136261u; /* FNV-1a */
    for (; *s; s++) {
        h ^= (unsigned char)*s;
        h *= 16777619u;
    }
    return h & (CAP - 1);
}

static char *sorted_key(const char *s)
{
    int cnt[26] = {0}, n = 0;
    for (; s[n]; n++)
        cnt[s[n] - 'a']++;
    char *k = malloc((size_t)n + 1), *p = k;
    for (int c = 0; c < 26; c++)
        while (cnt[c]--)
            *p++ = (char)('a' + c);
    *p = '\0';
    return k;
}

char ***groupAnagrams(char **strs, int strsSize, int *returnSize, int **returnColumnSizes)
{
    int *table = malloc(CAP * sizeof *table); /* 格子裡存組別編號，-1 = 空 */
    memset(table, -1, CAP * sizeof *table);
    char **gkey = malloc((size_t)strsSize * sizeof *gkey);
    char ***groups = malloc((size_t)strsSize * sizeof *groups);
    int *gsize = malloc((size_t)strsSize * sizeof *gsize);
    int *gcap = malloc((size_t)strsSize * sizeof *gcap);
    int ng = 0;

    for (int i = 0; i < strsSize; i++) {
        char *k = sorted_key(strs[i]);
        unsigned s = hash_str(k);
        while (table[s] != -1 && strcmp(gkey[table[s]], k) != 0)
            s = (s + 1) & (CAP - 1);
        int g = table[s];
        if (g == -1) { /* 新的一組 */
            g = table[s] = ng++;
            gkey[g] = k;
            gcap[g] = 4; /* 每組自己長大；一開始就開 n 格的話，n 組 × n 格會用掉 O(n²) 記憶體 */
            groups[g] = malloc((size_t)gcap[g] * sizeof **groups);
            gsize[g] = 0;
        } else {
            free(k);
        }
        if (gsize[g] == gcap[g]) {
            gcap[g] *= 2;
            groups[g] = realloc(groups[g], (size_t)gcap[g] * sizeof **groups);
        }
        groups[g][gsize[g]++] = strs[i]; /* 題目接受直接回傳原字串指標 */
    }
    for (int g = 0; g < ng; g++)
        free(gkey[g]);
    free(gkey);
    free(gcap);
    free(table);
    *returnSize = ng;
    *returnColumnSizes = gsize;
    return groups;
}
/* ===== 提交範圍 結束 ===== */

static int same_letters(const char *a, const char *b)
{
    char *x = sorted_key(a), *y = sorted_key(b);
    int r = strcmp(x, y) == 0;
    free(x);
    free(y);
    return r;
}

static void check(char **strs, int n, int want_groups)
{
    int ng, *cols;
    char ***g = groupAnagrams(strs, n, &ng, &cols);
    assert(ng == want_groups);
    int total = 0;
    for (int i = 0; i < ng; i++) {
        total += cols[i];
        for (int j = 1; j < cols[i]; j++)
            assert(same_letters(g[i][0], g[i][j])); /* 同組都是 anagram */
        for (int k = i + 1; k < ng; k++)
            assert(!same_letters(g[i][0], g[k][0])); /* 不同組不是 */
    }
    assert(total == n);
    for (int i = 0; i < ng; i++)
        free(g[i]);
    free(g);
    free(cols);
}

int main(void)
{
    char *a[] = {"eat", "tea", "tan", "ate", "nat", "bat"};
    check(a, 6, 3);
    char *b[] = {""};
    check(b, 1, 1);
    char *c[] = {"a"};
    check(c, 1, 1);
    char *d[] = {"", "b", "", "ab", "ba", "abc"};
    check(d, 6, 4);
    char *e[] = {"abc", "bca", "cab", "acb", "bac", "cba", "abc", "x", "cab"}; /* 一組 8 個，會觸發 realloc */
    check(e, 9, 2);
    puts("0049: passed");
    return 0;
}
