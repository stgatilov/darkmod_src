
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_BYTEORDERING_HPP
#define INCLUDE_RCF_BYTEORDERING_HPP

#include <boost/type_traits/is_same.hpp>

#include <RCF/Tools.hpp>
#include <RCF/util/Platform/Machine/ByteOrder.hpp>

namespace RCF {

    typedef Platform::Machine::LittleEndian         LittleEndian;
    typedef Platform::Machine::BigEndian            BigEndian;

    typedef BigEndian                               NetworkByteOrder;
    typedef Platform::Machine::ByteOrder            MachineByteOrder;

    inline void swapBytes(char *b1, char *b2)
    {
        //*b2 ^= *b1;
        //*b1 ^= *b2;
        //*b2 ^= *b1;

        char temp = *b1;
        *b1 = *b2;
        *b2 = temp;
    }

    inline void reverseByteOrder(void *buffer, int width, int count)
    {
        RCF_ASSERT(width > 0)(width);
        RCF_ASSERT(count > 0)(count);
        if (width == 1) return;

        BOOST_STATIC_ASSERT( sizeof(char) == 1 );   
        char *chBuffer = static_cast<char *>(buffer);
        for (int i=0; i<count; i++)
        {
            for (int j=0;j<width/2;j++)
            {
                swapBytes(
                    chBuffer + i*width + j,
                    chBuffer + i*width + width - j - 1 );
            }
        }

    }

    inline void machineToNetworkOrder(void *buffer, int width, int count)
    {
        if ( boost::is_same<MachineByteOrder, NetworkByteOrder>::value )
        {
            reverseByteOrder(buffer, width, count);
        }
    }

    inline void networkToMachineOrder(void *buffer, int width, int count)
    {
        if ( boost::is_same<MachineByteOrder, NetworkByteOrder>::value )
        {
            reverseByteOrder(buffer, width, count);
        }

    }

} // namespace RCF

#endif // ! INCLUDE_RCF_BYTEORDERING_HPP
