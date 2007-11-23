
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_SERIALIZESTL_HPP
#define INCLUDE_SF_SERIALIZESTL_HPP

#include <SF/Archive.hpp>

namespace SF {

    class PushBackSemantics
    {
    public:
        template<typename Container, typename Value>
        void operator()(Container &container, const Value &value)
        {
            container.push_back(value);
        }
    };

    class InsertSemantics
    {
    public:
        template<typename Container, typename Value>
        void operator()(Container &container, const Value &value)
        {
            container.insert(value);
        }
    };

    template<typename AddSemantics, typename StlContainer>
    void serializeStlContainer(Archive &ar, StlContainer &t, const unsigned int)
    {

        typedef typename StlContainer::iterator Iterator;
        typedef typename StlContainer::value_type Value;

        if (ar.isRead())
        {
            t.clear();
            unsigned int count = 0;
            ar & count;
            for (unsigned int i=0; i<count; i++)
            {
                Value value;
                ar & value;
                AddSemantics()(t, value);
            }
        }
        else if (ar.isWrite())
        {
            unsigned int count = static_cast<unsigned int>(t.size());
            ar & count;
            Iterator it = t.begin();
            for (unsigned int i=0; i<count; i++)
            {
                ar & *it;
                it++;
            }
        }
    }

}

#endif // ! INCLUDE_SF_SERIALIZESTL_HPP
