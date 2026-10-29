// bsearch.c：影片 [29:00] 的遞迴 binary search。看 gcc 會不會做 tail recursion elimination（把遞迴變成迴圈）
int bsearch_r(const int *arr, int left, int right, int val)
{
	if (left > right) return -1;
	int mid = (left + right) / 2;
	if (val == arr[mid]) return mid;
	if (val < arr[mid]) return bsearch_r(arr, left, mid - 1, val);
	return bsearch_r(arr, mid + 1, right, val);
}
