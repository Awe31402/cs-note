/* LeetCode 1349 · Maximum Students Taking Exam
 * 教室 m×n，'.' 可以坐、'#' 是壞椅子。學生看得到左、右、左前、右前的考卷。最多能坐幾個人、沒人能作弊？
 * 思路：把「會互相看到」的兩個座位連一條邊，要找最大獨立集 (maximum independent set)：選最多點、彼此不相鄰。
 *   一般圖是 NP-hard，但這張圖是「二分圖」：會衝突的兩個座位，行 (column) 一定差 1 → 奇數行、偶數行各一群。
 *   二分圖上：最大獨立集 = 點數 − 最大匹配（König 定理）。最大匹配用增廣路徑。
 *   （另一種常見解法是逐列的 bitmask DP，m, n <= 8 也很快。）
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 提交範圍 開始 ===== */
static int R, C, adj[64][6], deg[64], match_r[64];
static char seen[64];

static int augment(int u)
{
    for (int i = 0; i < deg[u]; i++) {
        int v = adj[u][i];
        if (seen[v])
            continue;
        seen[v] = 1;
        if (match_r[v] < 0 || augment(match_r[v])) {
            match_r[v] = u;
            return 1;
        }
    }
    return 0;
}

int maxStudents(char **seats, int seatsSize, int *seatsColSize)
{
    R = seatsSize;
    C = seatsColSize[0];
    int total = 0;
    memset(deg, 0, sizeof deg);
    /* 衝突的方向：左、右、左前、右前、左後、右後（無向邊，兩邊都要列） */
    const int dr[] = {0, 0, -1, -1, 1, 1}, dc[] = {-1, 1, -1, 1, -1, 1};
    for (int r = 0; r < R; r++)
        for (int c = 0; c < C; c++) {
            if (seats[r][c] != '.')
                continue;
            total++;
            for (int k = 0; k < 6; k++) {
                int nr = r + dr[k], nc = c + dc[k];
                if (nr >= 0 && nr < R && nc >= 0 && nc < C && seats[nr][nc] == '.')
                    adj[r * C + c][deg[r * C + c]++] = nr * C + nc;
            }
        }
    for (int i = 0; i < 64; i++)
        match_r[i] = -1;
    int matching = 0;
    for (int r = 0; r < R; r++)
        for (int c = 0; c < C; c += 2) /* 只從偶數行出發：偶數行 = 左邊，奇數行 = 右邊 */
            if (seats[r][c] == '.') {
                memset(seen, 0, sizeof seen);
                matching += augment(r * C + c);
            }
    return total - matching;
}
/* ===== 提交範圍 結束 ===== */

/* 暴力：枚舉所有座位子集合（座位數很少時） */
static int brute(char **s, int m, int n)
{
    int pos[64], k = 0, best = 0;
    for (int r = 0; r < m; r++)
        for (int c = 0; c < n; c++)
            if (s[r][c] == '.')
                pos[k++] = r * n + c;
    for (int mask = 0; mask < 1 << k; mask++) {
        int ok = 1, cnt = 0;
        for (int i = 0; i < k && ok; i++) {
            if (!(mask >> i & 1))
                continue;
            cnt++;
            for (int j = i + 1; j < k && ok; j++) {
                if (!(mask >> j & 1))
                    continue;
                int r1 = pos[i] / n, c1 = pos[i] % n, r2 = pos[j] / n, c2 = pos[j] % n;
                if (abs(c1 - c2) == 1 && abs(r1 - r2) <= 1)
                    ok = 0;
            }
        }
        if (ok && cnt > best)
            best = cnt;
    }
    return best;
}

static int run(const char *rows[], int m)
{
    static char buf[8][9];
    char *s[8];
    int cols[8];
    for (int i = 0; i < m; i++) {
        strcpy(buf[i], rows[i]);
        s[i] = buf[i];
        cols[i] = (int)strlen(rows[i]);
    }
    return maxStudents(s, m, cols);
}

int main(void)
{
    const char *a[] = {"#.##.#", ".####.", "#.##.#"};
    assert(run(a, 3) == 4);
    const char *b[] = {".#", "##", "#.", "##", ".#"};
    assert(run(b, 5) == 3);
    const char *c[] = {"#...#", ".#.#.", "..#..", ".#.#.", "#...#"};
    assert(run(c, 5) == 10);
    srand(1349);
    for (int t = 0; t < 300; t++) {
        int m = 1 + rand() % 4, n = 1 + rand() % 5;
        static char buf[8][9];
        char *s[8];
        int cols[8];
        for (int r = 0; r < m; r++) {
            for (int col = 0; col < n; col++)
                buf[r][col] = rand() % 3 ? '.' : '#';
            buf[r][n] = '\0';
            s[r] = buf[r];
            cols[r] = n;
        }
        int want = brute(s, m, n);
        assert(maxStudents(s, m, cols) == want);
    }
    puts("1349: passed");
    return 0;
}
