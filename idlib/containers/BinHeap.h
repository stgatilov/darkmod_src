#pragma once

#include "containers/List.h"

struct idLess {
	template<class A, class B> ID_FORCE_INLINE bool operator() (const A &a, const B &b) const {
		return a < b;
	}
};

/**
 * Binary heap: allows to quickly find minimum element in managed set.
 */
template<class T, class Cmp = idLess> class idBinHeap {
	struct Node {
		T value;
		int id;
	};
	Cmp comp;
	idList<Node> heap;
	idList<int> idToHeap;

	bool IsBetter(int a, int b) const {
		return comp(heap[a].value, heap[b].value);
	}
	void Swap(int a, int b) {
		idSwap(heap[a], heap[b]);
		idSwap(idToHeap[heap[a].id], idToHeap[heap[b].id]);
	}

	void SiftUp(int v) {
		while (v) {
			int f = (v - 1) >> 1;
			if (!IsBetter(v, f))
				break;
			Swap(v, f);
			v = f;
		}
	}

	void SiftDown(int v) {
		while (1) {
			int a = 2*v+1;
			int b = 2*v+2;
			if (a >= heap.Num())
				break;
			int s = a;
			if (b < heap.Num() && IsBetter(b, a))
				s = b;
			if (!IsBetter(s, v))
				break;
			Swap(s, v);
			v = s;
		}
	}

public:
	ID_FORCE_INLINE idBinHeap(const Cmp &cmp = Cmp()) : comp(cmp) {}

	//clear heap without losing memory buffers
	void Reset(const Cmp &cmp = Cmp()) {
		heap.SetNum(0, false);
		idToHeap.SetNum(0, false);
		comp = cmp;
	}
	
	ID_FORCE_INLINE int Num() const { return heap.Num(); }

	//if IDs are set explicitly, then they must be unique at all times
	//if not, then they are simply assigned sequentally
	int Add(const T &val, int *pId = nullptr) {
		//get some ID
		int id = (pId ? *pId : idToHeap.Num());
		while (id >= idToHeap.Num())
			idToHeap.AddGrow(-1);
		assert(idToHeap[id] < 0);
		//register new element
		idToHeap[id] = heap.Num();
		heap.AddGrow(Node{val, id});
		//restore heap property
		SiftUp(heap.Num() - 1);
		return id;
	}

	int GetMin(T *value = nullptr) const {
		assert(heap.Num() > 0);
		//return value
		int id = heap[0].id;
		if (value)
			*value = heap[0].value;
		return id;
	}

	int ExtractMin(T *value = nullptr) {
		int id = GetMin();
		//swap with last element and remove
		Swap(0, heap.Num() - 1);
		heap.Pop();
		idToHeap[id] = -1;
		//restore heap property
		SiftDown(0);
		return id;
	}

	void Update(int id, const T &val) {
		//find the element
		assert(id < idToHeap.Num() && idToHeap[id] >= 0);
		int h = idToHeap[id];
		//assign new value
		heap[h].value = val;
		//restore heap property
		SiftUp(h);
		SiftDown(h);
	}

	void Remove(int id) {
		//find the element
		assert(id < idToHeap.Num() && idToHeap[id] >= 0);
		int h = idToHeap[id];
		//lift it up to root, like as it has -infty key
		while (h) {
			int f = (h - 1) >> 1;
			Swap(h, f);
			h = f;
		}
		//remove root element
		ExtractMin();
	}
};
