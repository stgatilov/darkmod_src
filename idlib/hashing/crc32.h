/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.1  2004/10/30 15:52:37  sparhawk
 * Initial revision
 *
 ***************************************************************************/


#ifndef __CRC32_H__
#define __CRC32_H__

/*
===============================================================================

	Calculates a checksum for a block of data
	using the CRC-32.

===============================================================================
*/

void CRC32_InitChecksum( unsigned long &crcvalue );
void CRC32_UpdateChecksum( unsigned long &crcvalue, const void *data, int length );
void CRC32_FinishChecksum( unsigned long &crcvalue );
unsigned long CRC32_BlockChecksum( const void *data, int length );

#endif /* !__CRC32_H__ */
