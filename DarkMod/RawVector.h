#ifndef RAW_VECTOR_H
#define RAW_VECTOR_H

/**
 * std::vector-like raw buffer class.
 * Contains raw bytes, uses C allocation routines.
 * Grows exponentially, never shrinks.
 * No initialization or checks are done.
 * If malloc error happens then Error() is called
 * Only the most necessary methods are implemented.
 */
class CRawVector {
	static const int INITIAL_CAPACITY = 16;
public:
	CRawVector();
	~CRawVector();

	ID_INLINE int size() const { return m_Size; }
	ID_INLINE char &operator[] (int i) { return m_Pointer[i]; }
	ID_INLINE const char &operator[] (int i) const { return m_Pointer[i]; }

	inline void resize(int newSize)
	{
		if (newSize > m_Capacity) reallocate(newSize);
		m_Size = newSize;
	}

	//note: does not free memory!
	void clear();

private:
	///Pointer to data (allocated with malloc).
	char *m_Pointer;
	///Size of vector (number of bytes).
	int m_Size;
	///Size of allocated buffer.
	int m_Capacity;

	///Reallocate the memory buffer
	void reallocate(int newSize);

	//Non-copyable
	CRawVector(const CRawVector &other);
	CRawVector &operator= (const CRawVector &other);
};

namespace std {
	void swap(CRawVector &a, CRawVector &b);
}

#endif
