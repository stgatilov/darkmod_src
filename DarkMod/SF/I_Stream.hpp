
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_I_STREAM_HPP
#define INCLUDE_SF_I_STREAM_HPP

#include <typeinfo>

#include <SF/PortableTypes.hpp>

namespace SF {

    //*************************************************************************
    // Stream interfaces

    class DataPtr;
    class Node;

    typedef std::pair<void *, const std::type_info *> ObjectId;

    class I_ContextRead
    {
    public:
        virtual ~I_ContextRead() {}
        virtual void add(SF::UInt32 nid, const ObjectId &id) = 0;
        virtual void add(void *ptr, const std::type_info &objType, void *pObj ) = 0;
        virtual bool query(SF::UInt32 nid, ObjectId &id) = 0;
        virtual bool query(void *ptr, const std::type_info &objType, void *&pObj) = 0;
        virtual void clear() = 0;
    };

    class I_ContextWrite
    {
    public:
        virtual ~I_ContextWrite() {}
        virtual void setEnabled(bool enable) = 0;
        virtual bool getEnabled() = 0;
        virtual void add(const ObjectId &id, SF::UInt32 &nid) = 0;
        virtual bool query(const ObjectId &id, SF::UInt32 &nid) = 0;
        virtual void clear() = 0;
    };

    class I_WithContextRead
    {
    public:
        virtual ~I_WithContextRead() {}
        virtual I_ContextRead &getContext() = 0;
    };

    class I_WithContextWrite
    {
    public:
        virtual ~I_WithContextWrite() {}
        virtual I_ContextWrite &getContext() = 0;
    };

    class I_LocalStorage
    {
    public:
        virtual ~I_LocalStorage() {}
        virtual void setNode(Node *) = 0;
        virtual Node *getNode() = 0;
    };

    class I_WithLocalStorage
    {
    public:
        virtual ~I_WithLocalStorage() {}
        virtual I_LocalStorage &getLocalStorage() = 0;
    };

    class I_Encoding
    {
    public:
        virtual ~I_Encoding() {}
        virtual UInt32 getCount(DataPtr &data, const std::type_info &type) = 0;
        virtual void toData(DataPtr &data, void *pvObject, const std::type_info &type, int nCount) = 0;
        virtual void toObject(DataPtr &data, void *pvObject, const std::type_info &type, int nCount) = 0;
    };

    class I_WithEncoding
    {
    public:
        virtual ~I_WithEncoding() {}
        virtual I_Encoding &getEncoding() = 0;
    };

    class I_Stream
    {
    public:
        virtual ~I_Stream() {}
    };

    class WithFormatWrite
    {
    public:
        virtual ~WithFormatWrite() {}
        virtual void begin(const Node &) = 0;
        virtual void put(const DataPtr &) = 0;
        virtual void end() = 0;
    };

    class WithFormatRead
    {
    public:
        virtual ~WithFormatRead() {}
        virtual bool begin(Node &) = 0;
        virtual bool get(DataPtr &) = 0;
        virtual void end() = 0;
    };

}

#endif // ! INCLUDE_SF_I_STREAM_HPP
