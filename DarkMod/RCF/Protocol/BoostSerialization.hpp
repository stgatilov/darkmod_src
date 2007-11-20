
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_PROTOCOL_BOOSTSERIALIZATION_HPP
#define INCLUDE_RCF_PROTOCOL_BOOSTSERIALIZATION_HPP

#include <RCF/Protocol/Protocol.hpp>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/level.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/utility.hpp>

namespace RCF {   

    template<> class Protocol< boost::mpl::int_<BsBinary> > :
        public ProtocolImpl2<boost::archive::binary_iarchive, boost::archive::binary_oarchive>
    {
    public:
        static std::string getName()
        {
            return "boost serialization binary protocol";
        }
    };

    template<> class Protocol< boost::mpl::int_<BsText> > :
        public ProtocolImpl2<boost::archive::text_iarchive, boost::archive::text_oarchive>
    {
    public:
        static std::string getName()
        {
            return "boost serialization text protocol";
        }
    };

} // namespace RCF

#endif //! INCLUDE_RCF_PROTOCOL_BOOSTSERIALIZATION_HPP
