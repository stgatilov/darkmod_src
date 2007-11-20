
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_PROTOCOL_PROTOCOL_HPP
#define INCLUDE_RCF_PROTOCOL_PROTOCOL_HPP

#include <iosfwd>

#include <boost/function.hpp>

#include <RCF/Tools.hpp>

namespace RCF {

    static const int SfBinary       = 1;
    static const int SfText         = 2;
    static const int BsBinary       = 3;
    static const int BsText         = 4;

    extern const int DefaultSerializationProtocol;

    template<typename N>
    class Protocol
    {
    public:

        static std::string getName()
        {
            return "";
        }

        class In
        {
        public:
            void bind(std::istream &is)
            {
                RCF_UNUSED_VARIABLE(is);
            }

            void unbind()
            {}
           
            template<typename T>
            In &operator>>(T &t)
            {
                RCF_UNUSED_VARIABLE(t);
                RCF_THROW(RCF::SerializationException(RcfError_UnknownSerializationProtocol))(N::value);
                return *this;
            }
        };

        class Out
        {
        public:
            void bind(std::ostream &os)
            {
                RCF_UNUSED_VARIABLE(os);
            }

            void unbind()
            {}

            template<typename T>
            Out &operator<<(const T &t)
            {
                RCF_UNUSED_VARIABLE(t);
                RCF_THROW(RCF::SerializationException(RcfError_UnknownSerializationProtocol))(N::value);
                return *this;
            }
        };
    };


    template<typename IS, typename OS>
    class ProtocolImpl1
    {
    public:
        class In
        {
        public:
            void bind(std::istream &is)                           
            {
                mAr.clearState();
                mAr.setIs(is);
                invokeCustomizationCallback();
            }

            void unbind()
            {}
           
            template<typename T> In &operator>>(T &t)
            {
                mAr >> t;
                return *this;
            }

            typedef boost::function1<void, IS &> CustomizationCallback;

            void setCustomizationCallback(CustomizationCallback customizationCallback)
            {
                mCustomizationCallback = customizationCallback;
                invokeCustomizationCallback();
            }           
       
        private:
            IS mAr;

            CustomizationCallback mCustomizationCallback;

            void invokeCustomizationCallback()
            {
                if (mCustomizationCallback)
                {
                    mCustomizationCallback(mAr);
                }
            }

        };

        class Out
        {
        public:
            void bind(std::ostream &os)                           
            {
                mAr.clearState();
                mAr.setOs(os);

                invokeCustomizationCallback();
            }

            void unbind()
            {}
           
            template<typename T> Out &operator<<(const T &t)
            {
                mAr << t;
                return *this;
            }

            typedef boost::function1<void, OS &> CustomizationCallback;

            void setCustomizationCallback(CustomizationCallback customizationCallback)
            {
                mCustomizationCallback = customizationCallback;
                invokeCustomizationCallback();
            }

        private:
            OS mAr;

            CustomizationCallback mCustomizationCallback;

            void invokeCustomizationCallback()
            {
                if (mCustomizationCallback)
                {
                    mCustomizationCallback(mAr);
                }
            }

        };
    };
   
    template<typename IS, typename OS>
    class ProtocolImpl2
    {
    public:
        class In
        {
        public:
            void bind(std::istream &is)                           
            {
                mAr.reset( new IS(is) );
                invokeCustomizationCallback();
            }

            void unbind()
            {
                mAr.reset();
            }

            template<typename T> In &operator>>(T &t)
            {
                *mAr >> t;
                return *this;
            }

            typedef boost::function1<void, IS &> CustomizationCallback;

            void setCustomizationCallback(CustomizationCallback customizationCallback)
            {
                mCustomizationCallback = customizationCallback;
                invokeCustomizationCallback();
            }           

        private:
            std::auto_ptr<IS> mAr;

            CustomizationCallback mCustomizationCallback;

            void invokeCustomizationCallback()
            {
                if (mCustomizationCallback)
                {
                    mCustomizationCallback(*mAr);
                }
            }

        };

        class Out
        {
        public:
            void bind(std::ostream &os)                           
            {
                mAr.reset( new OS(os) );
                invokeCustomizationCallback();
            }

            void unbind()
            {
                mAr.reset();
            }

            template<typename T> Out &operator<<(const T &t)
            {
                *mAr << t;
                return *this;
            }

            typedef boost::function1<void, OS &> CustomizationCallback;

            void setCustomizationCallback(CustomizationCallback customizationCallback)
            {
                mCustomizationCallback = customizationCallback;
                invokeCustomizationCallback();
            }

        private:
            std::auto_ptr<OS> mAr;

            CustomizationCallback mCustomizationCallback;

            void invokeCustomizationCallback()
            {
                if (mCustomizationCallback)
                {
                    mCustomizationCallback(*mAr);
                }
            }

        };
    };

} // namespace RCF

#endif //! INCLUDE_RCF_PROTOCOL_PROTOCOL_HPP
