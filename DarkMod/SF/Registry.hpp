
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_REGISTRY_HPP
#define INCLUDE_SF_REGISTRY_HPP

#include <map>
#include <string>
#include <typeinfo>

#include <boost/shared_ptr.hpp>

#include <RCF/ThreadLibrary.hpp>
#include <RCF/util/InitDeinit.hpp>

#include <SF/Exception.hpp>
#include <SF/SerializePolymorphic.hpp>
#include <SF/Serializer.hpp>
#include <SF/Tools.hpp>

namespace SF {

    typedef util::ReadWriteMutex ReadWriteMutex;
    typedef util::ReadLock ReadLock;
    typedef util::WriteLock WriteLock;
    static const Platform::Threads::read_write_scheduling_policy ReaderPriority = Platform::Threads::reader_priority;
    static const Platform::Threads::read_write_scheduling_policy WriterPriority = Platform::Threads::writer_priority;

    class Registry : boost::noncopyable
    {
    private:
        Registry();
        //typedef const std::type_info * Rtti; // not safe, several pointers might correspond to single type
        typedef std::string Rtti;
        std::map<std::string, Rtti> typeRttis;
        std::map<Rtti, std::string> typeNames;
        std::map<std::pair<Rtti, Rtti>, boost::shared_ptr<I_SerializerPolymorphic> > serializerPolymorphicInstances;

        ReadWriteMutex readWriteMutex;

    public:

        friend void initRegistrySingleton();

        static Registry &getSingleton();

        static Registry *getSingletonPtr();

        template<typename Type>
        void registerType(Type *, const std::string &typeName)
        {
            WriteLock lock(readWriteMutex); RCF_UNUSED_VARIABLE(lock);
            Rtti typeRtti = typeid(Type).name();
            typeNames[typeRtti] = typeName;
            typeRttis[typeName] = typeRtti;

            // instantiate Type's serializer function so we can register the base/derived info
            // NB: release build optimizers had better not eliminate this.
            if (0)
            {
                serialize( *((Archive *) NULL), *((Type *) NULL), 0);
            }
        }

        template<typename Base, typename Derived>
        void registerBaseAndDerived(Base *, Derived *)
        {
            WriteLock lock(readWriteMutex); RCF_UNUSED_VARIABLE(lock);
            Rtti baseRtti = typeid(Base).name();
            Rtti derivedRtti = typeid(Derived).name();
            std::pair<Rtti, Rtti> baseDerivedRtti(baseRtti, derivedRtti);
            serializerPolymorphicInstances[baseDerivedRtti].reset(
                new SerializerPolymorphic<Base,Derived>);
        }

#if !defined(_MSC_VER) || _MSC_VER > 1200

        template<typename Type>
        void registerType(const std::string &typeName)
        {
            registerType( (Type *) 0);
        }

        template<typename Base, typename Derived>
        void registerBaseAndDerived()
        {
            registerBaseAndDerived( (Base *) 0, (Derived *) 0);
        }

#endif

#if !defined(_MSC_VER) || _MSC_VER > 1200

        template<typename Base>
        I_SerializerPolymorphic &getSerializerPolymorphic(const std::string &derivedTypeName)
        {
            return getSerializerPolymorphic( (Base *) 0, derivedTypeName);
        }

#endif

        template<typename Base>
        I_SerializerPolymorphic &getSerializerPolymorphic(Base *, const std::string &derivedTypeName)
        {
            ReadLock lock(readWriteMutex); RCF_UNUSED_VARIABLE(lock);
            Rtti baseRtti = typeid(Base).name();
            Rtti derivedRtti = typeRttis[derivedTypeName];
            std::pair<Rtti, Rtti> baseDerivedRtti(baseRtti, derivedRtti);
            if (serializerPolymorphicInstances.find(baseDerivedRtti) == serializerPolymorphicInstances.end())
            {
                RCF_THROW(RCF::Exception(RCF::SfError_BaseDerivedRegistration))(derivedTypeName)(baseRtti)(derivedRtti);
            }
            return *serializerPolymorphicInstances[ std::make_pair(baseRtti, derivedRtti) ];
        }

        bool isTypeRegistered(const std::string &typeName);

        bool isTypeRegistered(const std::type_info &ti);

#if !defined(_MSC_VER) || _MSC_VER > 1200

        template<typename T>
        std::string getTypeName()
        {
            //return getTypeName(typeid(T));
            return getTypeName( (T *) 0);
        }

#endif

        template<typename T>
        std::string getTypeName(T *)
        {
            return getTypeName(typeid(T));
        }

        std::string getTypeName(const std::type_info &ti);

        void clear();
    };

#if !defined(_MSC_VER) || _MSC_VER > 1200

    template<typename Type>
    inline void registerType(const std::string &typeName)
    {
        Registry::getSingleton().registerType( (Type *) 0, typeName);
    }

    template<typename Base, typename Derived>
    inline void registerBaseAndDerived()
    {
        Registry::getSingleton().registerBaseAndDerived( (Base *) 0, (Derived *) 0);
    }

#endif

    template<typename Type>
    inline void registerType( Type *, const std::string &typeName)
    {
        Registry::getSingleton().registerType( (Type *) 0, typeName);
    }

    template<typename Base, typename Derived>
    inline void registerBaseAndDerived( Base *, Derived *)
    {
        Registry::getSingleton().registerBaseAndDerived( (Base *) 0, (Derived *) 0);
    }

} // namespace SF

#endif // ! INCLUDE_SF_REGISTRY_HPP
