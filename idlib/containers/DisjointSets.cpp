#include "precompiled.h"
#include "DisjointSets.h"


void idDisjointSets::Init(idList<int> &arr, int num) {
	arr.SetNum(num);
	for (int i = 0; i < num; i++)
		arr[i] = i;
}

int idDisjointSets::GetHead(idList<int> &arr, int v) {
	//pass 1: find head
	int x = v;
	while (x != arr[x])
		x = arr[x];
	int head = x;
	//pass 2: path compression
	int y = v;
	while (y != head) {
		x = arr[y];
		arr[y] = head;
		y = x;
	}
	return head;
}

bool idDisjointSets::Merge(idList<int> &arr, int u, int v) {
	u = GetHead(arr, u);
	v = GetHead(arr, v);
	if (u == v)
		return false;
	arr[u] = v;
	return true;
}

void idDisjointSets::CompressAll(idList<int> &arr) {
	//just let path compression do its job
	for (int i = 0; i < arr.Num(); i++)
		GetHead(arr, i);
}

int idDisjointSets::ConvertToColors(idList<int> &arr) {
	CompressAll(arr);
	int *remap = (int*)alloca(arr.Num() * sizeof(int));
	int k = 0;
	for (int i = 0; i < arr.Num(); i++) if (arr[i] == i)
		remap[i] = k++;
	for (int i = 0; i < arr.Num(); i++)
		arr[i] = remap[arr[i]];
	return k;
}
