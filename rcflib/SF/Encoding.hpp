
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_ENCODING_HPP
#define INCLUDE_SF_ENCODING_HPP

#include <RCF/ByteOrdering.hpp>

#include <SF/DataPtr.hpp>
#include <SF/Exception.hpp>
#include <SF/Tools.hpp>

namespace SF {

    class Text;
    class BinaryNative;
    class BinaryPortable;

    static char chSeparator = ':';

    //**************************************************************************
    // countElements()

    template<typename T>
    inline UInt32 countTypedElements( BinaryNative *, T *, DataPtr &data)
    {
        RCF_ASSERT(data.length() % sizeof(T) == 0);
        return data.length() / sizeof(T);
    }

    template<typename T>
    inline UInt32 countTypedElements( BinaryPortable *, T *, DataPtr &data)
    {
        RCF_ASSERT(data.length() % sizeof(T) == 0);
        return data.length() / sizeof(T);
    }

    template<typename T>
    inline UInt32 countTypedElements( Text *, T *, DataPtr &data)
    {
        // Count number of internally occurring separators in the data, and then add 1
        UInt32 count = 0;
        for (UInt32 i=1; i<data.length()-1; i++)
        {
            if (data.get()[i] == Byte8(chSeparator))
            {
                count++;
            }
        }
        return count+1;
    }

    inline UInt32 countTypedElements( Text *, char *, DataPtr &data)
    {
        return data.length();
    }

    inline UInt32 countTypedElements( Text *, unsigned char *, DataPtr &data)
    {
        return data.length();
    }

#define SF_TRY_COUNT_ELEMENTS(type)                             \
    if (objType == typeid(type))                                \
    {                                                           \
        return countTypedElements( (E *) 0, (type *) 0, data);  \
    }

    template<typename E>
    inline UInt32 countElements( E *, DataPtr &data, const std::type_info &objType)
    {
        SF_FOR_EACH_FUNDAMENTAL_TYPE( SF_TRY_COUNT_ELEMENTS );
        RCF_ASSERT(0)(objType);
        return 0;
    }

#undef SF_TRY_COUNT_ELEMENTS


    //**************************************************************************
    // encodeTypedElements()/decodeTypedElements()

    // TODO: precision issues when encoding/decoding floating point values?

    template<typename T>
    inline void encodeTypedElements( Text *, T *, DataPtr &data, T *t, int nCount)
    {
        std::ostringstream ostr;
        ostr << t[0];
        for (int i=1; i<nCount; i++)
        {
            ostr.put(chSeparator);
            ostr << t[i];
        }
        std::string s = ostr.str();
        data.assign(reinterpret_cast<const Byte8 *>(s.c_str()), static_cast<UInt32>(s.length()));
    }

    template<typename T>
    inline void decodeTypedElements( Text *, T *, DataPtr &data, T *t, int nCount)
    {
        if (data.length() == 0)
        {
            RCF_THROW(RCF::Exception(RCF::SfError_DataFormat));
        }
        std::string strData(reinterpret_cast<char *>(data.get()), data.length());
        std::istringstream istr(strData);
        istr >> t[0];
        for (int i=1; i<nCount; i++)
        {
            char ch;
            istr.get(ch);
            RCF_ASSERT( ch == chSeparator );
            istr >> t[i];
        }
    }

    inline void encodeTypedElements( Text *, char *, DataPtr &data, char *t, int nCount)
    {
        data.assign(reinterpret_cast<Byte8 *>(t), nCount);
    }

    inline void decodeTypedElements( Text *, char *, DataPtr &data, char *t, int nCount)
    {
        memcpy(t, data.get(), nCount);
    }

    inline void encodeTypedElements( Text *, unsigned char *, DataPtr &data, unsigned char *t, int nCount)
    {
        data.assign(reinterpret_cast<Byte8 *>(t), nCount);
    }

    inline void decodeTypedElements( Text *, unsigned char *, DataPtr &data, unsigned char *t, int nCount)
    {
        memcpy(t, data.get(), nCount);
    }

    inline void encodeTypedElements( Text *, wchar_t *, DataPtr &data, wchar_t *t, int nCount)
    {
        data.assign(reinterpret_cast<Byte8 *>(t), nCount*sizeof(wchar_t));
    }

    inline void decodeTypedElements( Text *, wchar_t *, DataPtr &data, wchar_t *t, int nCount)
    {
        memcpy(t, data.get(), nCount*sizeof(wchar_t));
    }

    template<typename T>
    inline void encodeTypedElements( BinaryNative *, T *, DataPtr &data, T *t, int nCount)
    {
        data.assign(reinterpret_cast<Byte8 *>(t), sizeof(T)*nCount );
    }

    template<typename T>
    inline void decodeTypedElements( BinaryNative *, T *, DataPtr &data, T *t, int nCount)
    {
        RCF_ASSERT( data.length() == sizeof(T)*nCount);
        memcpy(t, data.get(), sizeof(T)*nCount);
    }

    template<typename T>
    inline void encodeTypedElements( BinaryPortable *, T *, DataPtr &data, T *t, int nCount)
    {
        UInt32 nBufferSize = sizeof(T) * nCount;
        UInt32 nAlloc = data.allocate(nBufferSize);
        RCF_ASSERT(nAlloc == nBufferSize);
        T *buffer = reinterpret_cast<T *>(data.get());
        memcpy(buffer, t, nBufferSize);
        RCF::machineToNetworkOrder(buffer, sizeof(T), nCount);
    }

    template<typename T>
    inline void decodeTypedElements( BinaryPortable *, T *, DataPtr &data, T *t, int nCount)
    {
        if (data.length() != sizeof(T)*nCount)
        {
            RCF_THROW(RCF::Exception(RCF::SfError_DataFormat))(data.length())(nCount)(typeid(T).name());
        }
        T *buffer = reinterpret_cast<T *>(data.get());
        RCF::networkToMachineOrder(buffer, sizeof(T), nCount);
        memcpy(t, buffer, nCount*sizeof(T));
    }

    //**************************************************************************
    // encodeElements()/decodeElements()

#define SF_TRY_ENCODE_ELEMENTS(type)                \
    if (objType == typeid(type))                    \
    {                                               \
        encodeTypedElements(                        \
            (Encoding *) 0,                         \
            (type *) 0,                             \
            data,                                   \
            static_cast<type *>(pvObject),          \
            nCount);                                \
        return;                                     \
    }

    template<typename Encoding>
    inline void encodeElements(
        Encoding *,
        DataPtr &data,
        void *pvObject,
        const std::type_info &objType,
        int nCount)
    {
        SF_FOR_EACH_FUNDAMENTAL_TYPE( SF_TRY_ENCODE_ELEMENTS );
        RCF_ASSERT(0)(objType);
    }

#undef SF_TRY_ENCODE_ELEMENTS

#define SF_TRY_DECODE_ELEMENTS(type)                \
    if (objType == typeid(type))                    \
    {                                               \
        decodeTypedElements(                        \
            (Encoding *) 0,                         \
            (type *) 0,                             \
            data,                                   \
            static_cast<type *>(pvObject),          \
            nCount);                                \
        return;                                     \
    }

    template<typename Encoding>
    inline void decodeElements(
        Encoding *,
        DataPtr &data,
        void *pvObject,
        const std::type_info &objType,
        int nCount)
    {
        SF_FOR_EACH_FUNDAMENTAL_TYPE( SF_TRY_DECODE_ELEMENTS );
        RCF_ASSERT(0)(objType);
    }

#undef SF_TRY_DECODE_ELEMENTS

} // namespace SF


#endif // !INCLUDE_SF_ENCODING_HPP
