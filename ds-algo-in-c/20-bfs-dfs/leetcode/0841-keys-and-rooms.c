/* LeetCode 841 · Keys and Rooms
 * 思路：房間 = 頂點，鑰匙 = 有向邊。從房間 0 出發做 DFS/BFS，看能不能走遍所有房間。O(V + E)。
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== 提交範圍 開始 ===== */
bool canVisitAllRooms(int **rooms, int roomsSize, int *roomsColSize)
{
    char *seen = calloc((size_t)roomsSize, 1);
    int *st = malloc((size_t)roomsSize * sizeof *st), top = 0, visited = 1;
    seen[0] = 1;
    st[top++] = 0;
    while (top > 0) {
        int u = st[--top];
        for (int i = 0; i < roomsColSize[u]; i++) {
            int v = rooms[u][i];
            if (!seen[v]) {
                seen[v] = 1; /* 每個房間最多進 stack 一次，所以 stack 開 n 格就夠 */
                visited++;
                st[top++] = v;
            }
        }
    }
    free(seen);
    free(st);
    return visited == roomsSize;
}
/* ===== 提交範圍 結束 ===== */

int main(void)
{
    int r0[] = {1}, r1[] = {2}, r2[] = {3};
    int *a[] = {r0, r1, r2, NULL}, ca[] = {1, 1, 1, 0};
    assert(canVisitAllRooms(a, 4, ca));
    int s0[] = {1, 3}, s1[] = {3, 0, 1}, s2[] = {2}, s3[] = {0};
    int *b[] = {s0, s1, s2, s3}, cb[] = {2, 3, 1, 1};
    assert(!canVisitAllRooms(b, 4, cb)); /* 房間 2 的鑰匙只在房間 2 裡 */
    puts("0841: passed");
    return 0;
}
