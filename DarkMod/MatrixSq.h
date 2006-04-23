/********************************************************
* 
* CLASS DESCRIPTION: 
* CMatrixSq is a container for square matrices (n x n)
* The 2d matrices are unwrapped into a 1d private array
*
**********************************************************/

/******************************************************************************
 *
 * PROJECT: DarkMod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 * $Name$
 *
 * $Log$
 * Revision 1.5  2006/04/23 18:39:30  ishtvan
 * saveing/loading fix for empty matrices
 *
 * Revision 1.4  2005/11/19 17:26:48  sparhawk
 * LogString with macro replaced
 *
 * Revision 1.3  2005/08/22 04:43:22  ishtvan
 * fixed math error in CMatRUT::Ind2dTo1d
 *
 * Revision 1.2  2005/03/30 18:16:20  sparhawk
 * CVS Header added
 *
 ******************************************************************************/

#ifndef MATRIXSQ_H
#define MATRIXSQ_H

template <class type>
class CMatrixSq		{

public:

		CMatrixSq(void);
		CMatrixSq( const CMatrixSq<type> &in );
		explicit CMatrixSq(int dim);
		virtual ~CMatrixSq<type>(void);
		CMatrixSq<type> &operator=( const CMatrixSq<type> &in );

		/**
		* Copy is designed for use when the external class is
		* using only a pointer to the matrix (eg, when forward-declaring)
		* and wants to copy one matrix to another.
		**/

		void Copy( const CMatrixSq<type> *input );
		
		/**
		* Initialize the square matrix to hold dim elements.
		* NOTE: A 1x1 matrix has dimension 1, NOT zero!
		**/
		bool Init(int dim);

		void Clear( void );

		/**
		* Set the appropriate entry to the provided type
		*
		* The following applies to derived class CMatRUT only:
		* NOTE: The indices provided here are those of a normal matrix
		* HOWEVER, columns must be greater than rows due to matrix being
		* right upper triangular (entries with row > col are empty)
		* 
		* CsndPropLoader handles this with another function that reverses the
		* indices if row is greater than column.
		**/
		bool Set(int row, int col, type &src);

		/**
		* Set an element explicitly in the 1d unwrapped RUT matrix
		**/
		bool Set1d( int ind, type &src );
		
		/**
		* Returns a pointer to the entry for the given 2d indices
		* Again gives an error if row > col, because this entry is empty.
		**/
		type *Get(int row, int col);

		/**
		* Returns a pointer to the entry for the given 1d index
		**/
		type *Get1d( int ind );
		
		/**
		* Returns the dimension (eg, returns 3 for 3x3 matrix)
		**/
		int Dim( void );
		
		/**
		* Returns number of filled elements in the RUT matrix.
		**/
		int NumFilled( void );

		/**
		* Returns a pointer to the first var in the list
		* Used for internal D3 TypeInfo code
		**/
		type * Ptr( void );
		
		/**
		* Returns true if the private member matrix has been deleted
		**/
		bool IsCleared( void );

		/**
		* The following two functions save/restore the matrix to/from a
		* save file.
		**/

		void SaveMatrixSq (idSaveGame *savefile);

		bool RestoreMatrixSq (idRestoreGame *savefile);

protected:

		/**
		* Calculate number of elements filled for a given dimension RUT matrix
		**/
		virtual int  NumFromDim( int dim );
		
		/**
		* Returns the index in the unwrapped 1d matrix for the given 2d indices
		**/
		virtual int  Ind2dTo1d ( int row, int col );
		
		/**
		* Save an element in the matrix to a savefile
		* Called by SaveMatrix.
		**/
		void SaveElement (idSaveGame *savefile, type &Entry);
		
		/**
		* Restore an element in the matrix from a savefile
		* Called by RestoreMatrix.
		**/
		void ReadElement (idRestoreGame *savefile, int ind);

protected:

		/**
		* Matrix dimension (eg, 3 for a 3x3 matrix)
		**/
		int m_dim;
		/**
		* number of filled elements in the RUT matrix
		**/
		int m_filled;
		/**
		* RUT matrix unwrapped into one dimension
		**/
		type *m_mat;
};

/********************************************************
* CLASS DESCRIPTION: 
*
* CMatRUT : CMatrixSq - Right upper triangular matrix
* (the matrix diagonals and lower 'triangle' are all zero 
*  and not stored or accessed)
*
* Derived class of CMatrixSq.
**********************************************************/

template <class type>
class CMatRUT : public CMatrixSq<type>  {
public:

	CMatRUT(void) : CMatrixSq<type>() {};

	explicit CMatRUT(int dim) : CMatrixSq<type>(dim) {};

	virtual ~CMatRUT<type>(void);

	/**
	* Works like CMatrixSq::Set, except it automatically reverses the indices
	* if row > column
	**/
	bool SetRev(int row, int col, type &src);

	/**
	* Works like CMatrixSq::Get, except it automatically reverses the indices
	* if row > column
	**/
	type *GetRev(int row, int col);


protected:
		/**
		* Calculate number of elements filled for a given dimension RUT matrix
		**/
		int  NumFromDim( int dim );
		/**
		* Returns the index in the unwrapped 1d matrix for the given 2d indices
		**/
		int  Ind2dTo1d ( int row, int col );

};


/****************************************************************
* Begin CMatrixSq Implementation:
*****************************************************************/

template <class type>
inline CMatrixSq<type>::CMatrixSq( void )
{
	m_dim = 0;
	m_filled = 0;
	m_mat = NULL;
	DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("CMatrixSq constructor called, set vars.\r" );
}

template <class type>
inline CMatrixSq<type>::CMatrixSq( int dim )
{
	Init(dim);
}

template <class type>
inline CMatrixSq<type>::CMatrixSq( const CMatrixSq<type> &in )
{
	*this = in;
}

template <class type>
inline CMatrixSq<type>::~CMatrixSq( void )
{
	Clear();
}

template <class type>
inline CMatrixSq<type> &CMatrixSq<type>::operator=(const CMatrixSq<type> &in)
{
	int i;
	if( in.m_filled != m_filled )
	{
		Clear();
		if (!Init( in.m_dim ) )
		{
			DM_LOG(LC_MISC, LT_ERROR)LOGSTRING("Out of memory when creating RUT Loss Matrix.\r");
			Clear();
			goto Quit;
		}
	}
	m_dim = in.m_dim;
	m_filled = in.m_filled;
	for(i=0; i<m_filled; i++)
		m_mat[i] = in.m_mat[i];
Quit:
	return *this;
}

template <class type>
inline bool CMatrixSq<type>::Set(int row, int col, type &src )
{
	bool returnval;
	int ind;

	ind = Ind2dTo1d( row, col );
	if (ind<0)
	{
		returnval = false;
	}
	else
	{
		m_mat[ind] = src;
		returnval = true;
	}
	return returnval;
}

template <class type>
inline bool CMatrixSq<type>::Set1d(int ind, type &src )
{
	bool returnval;
	if (ind >= m_filled || ind < 0)
	{
		returnval = false;
	}
	else
	{
		m_mat[ind] = src;
		returnval = true;
	}
	return returnval;
}

template <class type>
inline type *CMatrixSq<type>::Get(int row, int col)
{
	type *p;
	int ind = Ind2dTo1d( row, col );
	
	if ( ind < 0 )
	{
		// error reporting is handled by Ind2dTo1d, omitted here
		p = NULL;
		goto Quit;
	}

	p = &m_mat[ind];
Quit:
	return p;
}

template <class type>
inline type *CMatrixSq<type>::Get1d( int ind )
{
	type *p;
	
	if ( ind < 0 || ind > m_filled )
	{
		DM_LOG(LC_MISC, LT_ERROR)LOGSTRING("Tried to access matrix with 1d index out of bounds: index %d\r", ind );
		p = NULL;
		goto Quit;
	}

	p = &m_mat[ind];
Quit:
	return p;
}

template <class type>
inline int CMatrixSq<type>::Dim( void )
{
	return m_dim;
}

template <class type>
inline int CMatrixSq<type>::NumFilled( void )
{
	return m_filled;
}

template <class type>
inline bool CMatrixSq<type>::IsCleared( void )
{
	bool returnval(false);
	if( m_mat == NULL )
		returnval = true;
	return returnval;
}
	
template <class type>
inline bool CMatrixSq<type>::Init( int dim )
{
	bool returnval(true);
	int filled;
	
	Clear();
	filled = NumFromDim( dim );
	
	DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("Initializing matrix of dimension %d with %d elements.\r", dim, filled);
	
	if ((m_mat = new type[filled]) == NULL)
	{
		DM_LOG(LC_MISC, LT_ERROR)LOGSTRING("Out of memory allocating for matrix with %d elements\r", filled);
		returnval = false;
	}
	else
	{
		m_dim = dim;
		m_filled = filled;
	}

	return returnval;
}

template <class type>
inline void CMatrixSq<type>::Clear( void )
{
	if( m_mat != NULL )
	{
		delete[] m_mat;
		m_mat = NULL;

	}
	DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("Cleared CMatrixSq with %d elements\r", m_filled);
	m_dim = 0;
	m_filled = 0;
}

template< class type >
inline type *CMatrixSq<type>::Ptr( void ) 
{
	return m_mat;
}

template <class type>
inline void CMatrixSq<type>::SaveElement ( idSaveGame *savefile, type &Entry )
{
	Entry->Save( savefile ); 
}

template <>
inline void CMatrixSq<int>::SaveElement ( idSaveGame *savefile, int &Entry )
{
	savefile->WriteInt( Entry );
	DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("Wrote int: %d to savefile\r", Entry);
}

template <>
inline void CMatrixSq<float>::SaveElement ( idSaveGame *savefile, float &Entry )
{
	savefile->WriteFloat( Entry );
}

template <class type>
inline void CMatrixSq<type>::ReadElement ( idRestoreGame *savefile, int ind )
{
	// general case: This should only be used with
	// specialized classes with the Restore method defined.

	m_mat[ind].Restore( savefile );
}


template <>
inline void CMatrixSq<int>::ReadElement ( idRestoreGame *savefile, int ind )
{
	savefile->ReadInt( m_mat[ind] );
	DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("Read int: %d from savefile\r", m_mat[ind]);
}

template <>
inline void CMatrixSq<float>::ReadElement ( idRestoreGame *savefile, int ind )
{
	savefile->ReadFloat( m_mat[ind] );
}

template <class type>
inline void CMatrixSq<type>::SaveMatrixSq ( idSaveGame *savefile )
{

	int i, num;
	
	if ( IsCleared() )
	{
		DM_LOG(LC_MISC, LT_WARNING)LOGSTRING("Tried to save an empty CMatrixSq/r" );
		
		// write false to the first bool in the savefile which determines validity
		savefile->WriteBool( false );
		goto Quit;
	}

	// matrix is filled
	savefile->WriteBool( true );

	// write the dimension of the matrix
	savefile->WriteInt( m_dim );
	
	num = NumFromDim( m_dim );
	for(i = 0; i < num; i++)
	{
		SaveElement( savefile, m_mat[i] );
	}

Quit:
	return;
}

template <class type>
inline bool CMatrixSq<type>::RestoreMatrixSq( idRestoreGame *savefile )
{
	int i, dim, num;
	bool IsFilled, bReturnVal(false);

	Clear();
	
	// check whether matrix is empty
	savefile->ReadBool( IsFilled );
	if( !IsFilled )
		goto Quit;

	savefile->ReadInt( dim );
	DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("[Relations matrix] Loaded dimension %d from savefile\r", dim);
	
	if (!Init( dim ))
		goto Quit;

	num = NumFromDim( dim );

	for(i=0; i<num; i++)
	{
		DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("Reading element %d from savefile\r", i);
		ReadElement( savefile, i );
	}
	
	bReturnVal = true;
	DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("num_filled = %d, dim = %d\r", m_filled, m_dim);

Quit:
	return bReturnVal;
}

template <class type>
inline void CMatrixSq<type>::Copy( const CMatrixSq<type> *input )
{
	*this = *input;
}

/**
* BEGIN PRIVATE METHODS:
**/

template <class type>
inline int CMatrixSq<type>::NumFromDim( int dim )
{
	int numf;
	if( dim <= 0)
		numf = 0;
	else
	{
		numf = dim*dim;
	}
	
	return numf;
}

/************************************
* Psuedocode for CMatrixSq<type>::Ind2dTo1d
* Given row, col
* rowindex0 = sum(dim - row) (for rows greater than zero)
* coloffset = (col - row - 1 )
* col must be greater than row.
*************************************/

template <class type>
inline int CMatrixSq<type>::Ind2dTo1d ( int row, int col )
{
	int returnval;

	returnval = row*m_dim + col;

	//check if the index is out of bounds
	if ( returnval < 0 || returnval >= m_filled || m_filled == 0 )
	{
		DM_LOG(LC_MISC, LT_ERROR)LOGSTRING("Tried to access matrix with index out of bounds: row %d, col %d.\r", row, col);
		returnval = -1;
	}
	return returnval;
}



/****************************************************************
* Begin CMatRUT Implementation:
*****************************************************************/

template <class type>
inline CMatRUT<type>::~CMatRUT( void )
{
	Clear();
}

template <class type>
inline type *CMatRUT<type>::GetRev(int row, int col)
{
	int temp;
	type *p;

	if (row > col)
	{
		temp = row;
		row = col;
		col = temp;
	}
	int ind = Ind2dTo1d( row, col );
	
	if ( ind < 0 )
	{
		// error reporting is handled by Ind2dTo1d, omitted here
		p = NULL;
		goto Quit;
	}

	p = &m_mat[ind];
Quit:
	return p;
}

template <class type>
inline bool CMatRUT<type>::SetRev(int row, int col, type &src )
{
	bool returnval;
	int ind, temp;

	if( row > col )
	{
		temp = row;
		row = col;
		col = temp;
	}

	ind = Ind2dTo1d( row, col );
	if (ind<0)
	{
		returnval = false;
	}
	else
	{
		m_mat[ind] = src;
		returnval = true;
	}
	return returnval;
}


/**
* BEGIN PRIVATE METHODS:
**/

template <class type>
inline int CMatRUT<type>::NumFromDim( int dim )
{
	int numf;
	if( dim <= 0)
		numf = 0;
	else
	{
		numf = dim*dim - ( (dim >> 1)*(dim+1) );
	}
	return numf;
}

/************************************
* Psuedocode for CMatRUT<type>::Ind2dTo1d
* Given row, col
* rowindex0 = sum(dim - row) (for rows greater than zero)
* coloffset = (col - row - 1 )
* col must be greater than row.
*************************************/

template <class type>
inline int CMatRUT<type>::Ind2dTo1d ( int row, int col )
{
	int rowIndex, colOffset, returnval;

	rowIndex = row*m_dim - ( (row*(row+1)) >> 1);

	colOffset = (col - row - 1);
	returnval = rowIndex + colOffset;

	//check if the index is out of bounds
	if ( row > col || returnval < 0 || returnval >= m_filled )
	{
		DM_LOG(LC_MISC, LT_ERROR)LOGSTRING("Tried to access RUT matrix with bad index (out of bounds or empty): row: %d, col: %d.\r", row, col);
		returnval = -1;
	}

	return returnval;
}

#endif

