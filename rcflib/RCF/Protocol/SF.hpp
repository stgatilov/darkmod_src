
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_PROTOCOL_SF_HPP
#define INCLUDE_RCF_PROTOCOL_SF_HPP

#include <RCF/Protocol/Protocol.hpp>

#include <SF/IBinaryStream.hpp>
#include <SF/OBinaryStream.hpp>
#include <SF/string.hpp>

namespace RCF {

    template<>
    class Protocol< boost::mpl::int_<SfBinary> > : public ProtocolImpl1<SF::IBinaryStream, SF::OBinaryStream>
    {
    public:
        static std::string getName()
        {
            return "SF portable binary protocol";
        }
    };

    inline void enableSfPointerTracking_1(SF::OBinaryStream &obinStream, bool enable)
    {
        enable ?
            obinStream.enableContext():
            obinStream.disableContext();
    }

} // namespace RCF

/*
#include <SF/ITextStream.hpp>
#include <SF/OTextStream.hpp>

namespace RCF {

    template<>
    class Protocol< boost::mpl::int_<SfText> > : public ProtocolImpl1<SF::ITextStream, SF::OTextStream>
    {
    public:
        static std::string getName()
        {
            return "SF text protocol";
        }
    };

    inline void enableSfPointerTracking_2(SF::OTextStream &otextStream, bool enable)
    {
        enable ?
            otextStream.enableContext():
            otextStream.disableContext();
    }

} // namespace RCF
*/

#endif //! INCLUDE_RCF_PROTOCOL_SF_HPP
