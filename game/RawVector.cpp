#include "../idlib/precompiled.h"
#pragma hdrstop

#include "RawVector.h"

CRawVector::CRawVector()
	: m_Pointer(0)
	, m_Size(0), m_Capacity(0)
{
	reallocate(INITIAL_CAPACITY);
}

CRawVector::~CRawVector()
{
	free(m_Pointer);
}

void CRawVector::reallocate(int newSize) {
	int newCapacity = m_Capacity << 1;
	if (newCapacity < newSize) newCapacity = newSize;

	char *newbuffer = (char*)realloc(m_Pointer, newCapacity);
	if (!newbuffer)
		common->FatalError("CRawBuffer::resize: realloc failed (from %d to %d bytes)", m_Capacity, newCapacity);

	m_Pointer = newbuffer;
	m_Capacity = newCapacity;
}

void CRawVector::clear() {
	resize(0);
}

namespace std {
	void swap(CRawVector &a, CRawVector &b) {
		static const int RVS = sizeof(CRawVector);
		char buffer[RVS];
		memcpy(buffer, &a, RVS);
		memcpy(&a, &b, RVS);
		memcpy(&b, buffer, RVS);
	}
}
