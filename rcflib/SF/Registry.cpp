
#include <SF/Registry.hpp>

#include <RCF/InitDeinit.hpp>

namespace SF {

    static Registry *pRegistry;
    
#ifndef WIN32 
    // greebo: Added missing forward declaration for Linux systems
    void initRegistrySingleton();
#endif

    Registry::Registry() :
        readWriteMutex(Platform::Threads::writer_priority)
    {}

    Registry &Registry::getSingleton()
    {
        if (!pRegistry)
        {
            initRegistrySingleton();
        }
        return *pRegistry;
    }

    Registry *Registry::getSingletonPtr()
    {
        return &getSingleton();
    }

    bool Registry::isTypeRegistered(const std::string &typeName)
    {
        ReadLock lock(readWriteMutex); RCF_UNUSED_VARIABLE(lock);
        return typeRttis.find(typeName) != typeRttis.end();
    }

    bool Registry::isTypeRegistered(const std::type_info &ti)
    {
        ReadLock lock(readWriteMutex); RCF_UNUSED_VARIABLE(lock);
        Rtti typeRtti = ti.name();
        return typeNames.find(typeRtti) != typeNames.end();
    }

    std::string Registry::getTypeName(const std::type_info &ti)
    {
        ReadLock lock(readWriteMutex); RCF_UNUSED_VARIABLE(lock);
        Rtti typeRtti = ti.name();
        if (typeNames.find(typeRtti) == typeNames.end())
        {
            return "";
        }
        else
        {
            return typeNames[typeRtti];
        }
    }

    void Registry::clear()
    {
        typeRttis.clear();
        typeNames.clear();
        serializerPolymorphicInstances.clear();
    }

    void initRegistrySingleton()
    {
        if (!pRegistry)
        {
            pRegistry = new Registry();
        }
    }

    void deinitRegistrySingleton()
    {
        delete pRegistry;
        pRegistry = NULL;
    }

    RCF_ON_INIT_DEINIT(
        initRegistrySingleton(),
        deinitRegistrySingleton())

}
