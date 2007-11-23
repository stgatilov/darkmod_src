
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_SERIALIZEPOLYMORPHIC_HPP
#define INCLUDE_SF_SERIALIZEPOLYMORPHIC_HPP

namespace SF {

    class Archive;

    class I_SerializerPolymorphic
    {
    public:
        virtual ~I_SerializerPolymorphic() {}
        virtual bool invoke(void **ppvb, Archive &ar) = 0;
    };

    //template<typename Base, typename Derived>
    //void registerBaseAndDerived(Base * = 0, Derived * = 0);

#if defined(_MSC_VER) && _MSC_VER <= 1200

    template<typename Base, typename Derived>
    class SerializerPolymorphic : public I_SerializerPolymorphic
    {
    public:
        static void instantiate() {}
        bool invoke(void **ppvb, Archive &ar);
    };

#else

    template<typename Base, typename Derived>
    void registerBaseAndDerived();

    template<typename Base, typename Derived>
    class SerializerPolymorphic : public I_SerializerPolymorphic
    {
    public:
        static SerializerPolymorphic &instantiate() { return instance; }
        SerializerPolymorphic() {}
        bool invoke(void **ppvb, Archive &ar);

        static SerializerPolymorphic instance;
        SerializerPolymorphic(int)
        {
            registerBaseAndDerived<Base, Derived>();
        }
    };

    // on gcc 3.2, this requires the SerializerPolymorphic ctor to be public
    template<class Base, class Derived>
    SerializerPolymorphic<Base, Derived> SerializerPolymorphic<Base, Derived>::instance(0);

#endif

}

#include <SF/Archive.hpp>

namespace SF {

    template<typename Base, typename Derived>
    bool SerializerPolymorphic<Base,Derived>::invoke(void **ppvb, Archive &ar)
    {
        if (ar.isWrite())
        {
            Base *pb = reinterpret_cast<Base *>(*ppvb);
            Derived *pd = dynamic_cast<Derived *>(pb);
            ar & pd;
        }
        else if (ar.isRead())
        {
            if (ar.isFlagSet(Archive::POINTER))
            {
                Derived *pd = NULL;
                ar & pd;
                Base *pb = static_cast<Base *>(pd);
                *ppvb = pb;
            }
            else
            {
                Base *pb = reinterpret_cast<Base *>(*ppvb);
                Derived *pd = dynamic_cast<Derived *>(pb);
                ar & *pd;
            }
        }
        return ar.isOk();
    }

}

#endif // ! INCLUDE_SF_SERIALIZEPOLYMORPHIC_HPP
