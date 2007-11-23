
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_UTIL_PORTS_HPP
#define INCLUDE_UTIL_PORTS_HPP

#include <fstream>
#include <string>

#include "AutoBuild.hpp"
#include "InitDeinit.hpp"

namespace util {

    class PortNumbers {

    private:
        PortNumbers() : mPort(RCF_DEFAULT_INIT), mIp("127.0.0.1")
        {
            {
                std::ifstream fin(getFileName().c_str());
                fin >> mPort;
            }
            if (mPort <= 50000 || mPort >= 70000)
            {
                mPort = 50001;
                std::ofstream fout( getFileName().c_str() );
                fout << mPort;
            }
        }
       
        ~PortNumbers()
        {
            std::ofstream fout( getFileName().c_str() );
            fout << mPort;
        }

        int mPort;
        std::string mIp;

        std::string getFileName()
        {
            return TEMP_DIR "port.txt";
        }

        static PortNumbers *&getSingletonPtrRef()
        {
            static PortNumbers *pSingleton = NULL;
            return pSingleton;
        }

    public:
        // not synchronized
        static PortNumbers &getSingleton()
        {
            PortNumbers *&pSingleton = getSingletonPtrRef();
            if (pSingleton == NULL)
            {
                pSingleton = new PortNumbers();
            }
            return *pSingleton;
        }

        // not synchronized
        static void deleteSingleton()
        {
            PortNumbers *&pSingleton = getSingletonPtrRef();
            delete pSingleton;
            pSingleton = NULL;
        }

        int getNext()
        {
            return mPort++;
        }

        int getCurrent()
        {
            return mPort;
        }

        void setCurrent(int port)
        {
            mPort = port;
        }

        std::string getIp()
        {
            return mIp;
        }

        void setIp(const std::string ip)
        {
            mIp = ip;
        }

    };

    UTIL_ON_INIT_DEINIT( PortNumbers::getSingleton(), PortNumbers::deleteSingleton() )

} // namespace util

#endif //! INCLUDE_UTIL_PORTS_HPP
