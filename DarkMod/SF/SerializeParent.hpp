
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_SERIALIZEPARENT_HPP
#define INCLUDE_SF_SERIALIZEPARENT_HPP

#include <boost/mpl/bool.hpp>

namespace SF {
    class Archive;
}

#ifdef RCF_USE_SF_SERIALIZATION

#include <SF/Archive.hpp>
#include <SF/SerializePolymorphic.hpp>

template<typename Base, typename Archive, typename Derived>
void serializeParentImpl(Base *, Archive &ar, Derived &derived, boost::mpl::true_ *)
{
    SF::SerializerPolymorphic<Base,Derived>::instantiate();
    ar & SF::Archive::PARENT & static_cast<Base &>(derived);
}

#endif

#ifdef RCF_USE_BOOST_SERIALIZATION

#include <boost/serialization/base_object.hpp>
template<typename Base, typename Archive, typename Derived>
void serializeParentImpl(Base *, Archive &ar, Derived &derived, boost::mpl::false_ *)
{
    ar & boost::serialization::base_object<Base>(derived);
}

#endif

template<typename Base, typename Archive, typename Derived>
void serializeParent(Base *, Archive &ar, Derived &derived)
{
    typedef typename boost::is_same<Archive, SF::Archive>::type type;
    serializeParentImpl( (Base*) 0, ar, derived, (type*) 0);
}

#if !defined(_MSC_VER) || _MSC_VER >= 1310

template<typename Base, typename Archive, typename Derived>
void serializeParent(Archive &ar, Derived &derived)
{
    serializeParent( (Base *) 0, ar, derived);
}

#endif

#endif // ! INCLUDE_SF_SERIALIZEPARENT_HPP
