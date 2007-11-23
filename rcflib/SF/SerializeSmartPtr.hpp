
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_SERIALIZESMARTPTR_HPP
#define INCLUDE_SF_SERIALIZESMARTPTR_HPP

#include <SF/Archive.hpp>
#include <SF/Stream.hpp>

namespace SF {

    //********************************************************
    // smart pointer utilities

    //template<typename T>
    //class Serializer;

    // 1. Non-ref counted smart pointer. SmartPtr<T> must support reset() and operator->().
/*
#ifndef __BORLANDC__

    template< template<typename> class SmartPtr, typename T >
    inline bool serializeSimpleSmartPtr(SmartPtr<T> **pps, SF::Archive &ar)
    {
        if (ar.isRead()) {
            if (ar.isFlagSet(Archive::POINTER)) *pps = new SmartPtr<T>;
            T *pt = NULL;
            ar & pt;
            (**pps).reset(pt);
        }
        else if (ar.isWrite()) {
            T *pt = NULL;
            if (*pps && (**pps).get()) pt = (**pps).operator->();
            ar & pt;
        }
        return true;
    }

#define SF_SERIALIZE_SIMPLE_SMARTPTR( SmartPtr )                                        \
    template<typename T>                                                                \
    class Serializer< SmartPtr<T> > {                                                   \
    public:                                                                             \
        SmartPtr<T> **ppt_;                                                             \
        Serializer(SmartPtr<T> **ppt) : ppt_(ppt) {}                                    \
        bool invoke(Archive &ar) { return serializeSimpleSmartPtr(ppt_, ar); }          \
    };

#else // __BORLANDC__
*/

#define SF_SERIALIZE_SIMPLE_SMARTPTR( SmartPtr )                                        \
    template<typename T>                                                                \
    inline bool serializeSimpleSmartPtr(SmartPtr<T> **pps, SF::Archive &ar)             \
    {                                                                                   \
        if (ar.isRead()) {                                                              \
            if (ar.isFlagSet(Archive::POINTER)) *pps = new SmartPtr<T>;                 \
            T *pt = NULL;                                                               \
            ar & pt;                                                                    \
            (**pps).reset(pt);                                                          \
        }                                                                               \
        else if (ar.isWrite()) {                                                        \
            T *pt = NULL;                                                               \
            if (*pps && (**pps).get()) pt = (**pps).operator->();                       \
            ar & pt;                                                                    \
        }                                                                               \
        return true;                                                                    \
    }                                                                                   \
                                                                                        \
    template<typename T>                                                                \
    inline bool invokeCustomSerializer(SmartPtr<T> **ppt, Archive &ar, int)             \
    {                                                                                   \
        return serializeSimpleSmartPtr(ppt, ar);                                        \
    }

//#endif

    // 2. Ref counted smart pointer. Must support operator=(), operator->(), and get().
/*
#ifndef __BORLANDC__

    template< template<typename> class SmartPtr, typename T >
    inline bool serializeRefCountedSmartPtr(SmartPtr<T> **pps, SF::Archive &ar)
    {
        RCF_ASSERT( ar.isRead() || ar.isWrite() );
        if (ar.isRead()) {
            if (ar.isFlagSet(Archive::POINTER)) *pps = new SmartPtr<T>;
            T *pt = NULL;
            if ((ar & pt).isOk()) {
                typedef ObjectId IdT;
                I_ContextRead &ctx = dynamic_cast<WithContextRead *>(ar.getStream())->getContext();
                void *pv = NULL;
                if (pt && ctx.query( pt, typeid(SmartPtr<T>), pv )) {
                    SmartPtr<T> *ps_prev = reinterpret_cast< SmartPtr<T> * >(pv);
                    **pps = *ps_prev;
                }
                else if (pt) {
                    ctx.add( pt, typeid(SmartPtr<T>), *pps );
                    // **pps = SmartPtr<T>(pt); // causes ICE with codewarrior 9.0
                    SmartPtr<T> spt(pt);
                    **pps = spt;
                }
                else {
                    // **pps = SmartPtr<T>(pt); // causes ICE with codewarrior 9.0
                    SmartPtr<T> spt(pt);
                    **pps = spt;
                }
                return true;
            }
            else
                return false;
        }
        else {
            T *pt = NULL;
            if (*pps) pt = (**pps).get();
            return (ar & pt).isOk();
        }
    }

#define SF_SERIALIZE_REFCOUNTED_SMARTPTR( SmartPtr )                                    \
    template<typename T>                                                                \
    class Serializer< SmartPtr<T> > {                                                   \
    public:                                                                             \
        SmartPtr<T> **ppt_;                                                             \
        Serializer(SmartPtr<T> **ppt) : ppt_(ppt) {}                                    \
        bool invoke( Archive &ar ) { return serializeRefCountedSmartPtr(ppt_, ar); }    \
    };

#else // __BORLANDC__
*/
#define SF_SERIALIZE_REFCOUNTED_SMARTPTR( SmartPtr )                                    \
    template<typename T >                                                               \
    inline bool serializeRefCountedSmartPtr(SmartPtr<T> **pps, SF::Archive &ar)         \
    {                                                                                   \
        if (ar.isRead()) {                                                              \
            if (ar.isFlagSet(Archive::POINTER)) *pps = new SmartPtr<T>;                 \
            T *pt = NULL;                                                               \
            if ((ar & pt).isOk()) {                                                     \
                typedef ObjectId IdT;                                                   \
                I_ContextRead &ctx = dynamic_cast<WithContextRead *>(ar.getStream())->getContext();    \
                void *pv = NULL;                                                        \
                if (pt && ctx.query( pt, typeid(SmartPtr<T>), pv )) {                   \
                    SmartPtr<T> *ps_prev = reinterpret_cast< SmartPtr<T> * >(pv);       \
                    **pps = *ps_prev;                                                   \
                }                                                                       \
                    else if (pt) {                                                      \
                    ctx.add( pt, typeid(SmartPtr<T>), *pps );                           \
                    **pps = SmartPtr<T>(pt);                                            \
                }                                                                       \
                else {                                                                  \
                    **pps = SmartPtr<T>(pt);                                            \
                }                                                                       \
                return true;                                                            \
            }                                                                           \
            else                                                                        \
                return false;                                                           \
        }                                                                               \
        else /*if (ar.isWrite())*/ {                                                        \
            T *pt = NULL;                                                               \
            if (*pps) pt = (**pps).get();                                               \
            return (ar & pt).isOk();                                                    \
        }                                                                               \
    }                                                                                   \
                                                                                        \
    template<typename T>                                                                \
    inline bool invokeCustomSerializer(SmartPtr<T> **ppt, Archive &ar, int)             \
    {                                                                                   \
        return serializeRefCountedSmartPtr(ppt, ar);                                    \
    }

//#endif

} // namespace SF

#endif // ! INCLUDE_SF_SERIALIZERSMARTPTR_HPP
