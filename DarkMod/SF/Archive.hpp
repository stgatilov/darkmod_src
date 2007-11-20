
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_SF_ARCHIVE_HPP
#define INCLUDE_SF_ARCHIVE_HPP

#include <boost/noncopyable.hpp>

#include <RCF/util/DefaultInit.hpp>

#include <SF/DataPtr.hpp>
#include <SF/I_Stream.hpp>

namespace SF {

    class Archive;

    template<typename U>
    inline bool invokePtrSerializer(U u, Archive &ar);

    class Archive : boost::noncopyable
    {
    public:

        enum Direction
        {
            READ,
            WRITE
        };

        enum Flag
        {
            PARENT              = 1 << 0,
            POINTER             = 1 << 1,
            NODE_ALREADY_READ   = 1 << 2,
            NO_BEGIN_END        = 1 << 3,
            POLYMORPHIC         = 1 << 4
        };

        Archive(Direction dir, I_Stream *stream) :
            ok(true),
            dir(dir),
            stream(stream),
            label(),
            flags(RCF_DEFAULT_INIT)
        {}

        template<typename T> Archive &operator&(const T &t)
        {
            ok = invokePtrSerializer(&t, *this);
            return *this;
        }

        /*template<typename T> Archive &operator&(T &t)
        {
            ok = invokePtrSerializer(&t, *this);
            return *this;
        }*/

        Archive &operator&(Flag flag)
        {
            setFlag(flag);
            return *this;
        }

        bool isRead() const
        {
            return dir == READ;
        }

        bool isWrite() const
        {
            return dir == WRITE;
        }

        I_Stream *getStream() const
        {
            return stream;
        }

        bool isFlagSet(Flag flag) const
        {
            return flags & flag ? true : false;
        }

        void setFlag(Flag flag, bool bEnable = true)
        {
            if (bEnable)
                flags |= flag;
            else
                flags &= ~flag;
        }

        void clearFlag(Flag flag)
        {
            setFlag(flag, false);
        }

        bool isOk() const
        {
            return ok;
        }

        void clearState()
        {
            label = "";
            flags = 0;
        }

        DataPtr &getLabel()
        {
            return label;
        }

    private:

        bool ok;
        Direction dir;
        I_Stream *stream;
        DataPtr label;
        unsigned int flags;
    };

}

#endif // ! INCLUDE_SF_ARCHIVE_HPP
