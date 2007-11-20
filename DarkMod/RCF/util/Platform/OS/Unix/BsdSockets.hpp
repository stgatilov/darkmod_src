// OS-specific details - Unix common

#ifndef INCLUDE_UTIL_PLATFORM_OS_UNIX_BSDSOCKETS_HPP
#define INCLUDE_UTIL_PLATFORM_OS_UNIX_BSDSOCKETS_HPP

#if defined(sun) || defined(__sun) || defined(__sun__)
#define BSD_COMP                // Needs to be defined in order to use FIONBIO in ioctl()
#define MSG_NOSIGNAL 0            // MSG_NOSIGNAL flag for send() not implemented on Solaris
#endif

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <cerrno>

#ifndef __UNIX__
#define __UNIX__
#endif

// O_BINARY is not defined in Unix (all files are "binary" anyway)
#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef SOCKET
#define SOCKET int
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

// Solaris doesn't define INADDR_NONE, for some reason
#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long) -1)
#endif

namespace Platform {

    namespace OS {

        inline std::string GetErrorString(int err) { return std::string( strerror(err) ); }

        inline std::string GetErrorString() { return GetErrorString( errno ); }

        namespace BsdSockets {

#ifndef __CYGWIN__
            typedef ::socklen_t socklen_t;
#endif

            inline int recv(int fd, char *buf, int len, int flags)             
            {
                return ::recv(fd, buf, len, flags);
            }

            inline int send(int fd, const char *buf, int len, int flags)       
            {
                return ::send(fd, buf, len, flags | MSG_NOSIGNAL);
            }

            inline int sendto(int fd, const char *buf, int len, int flags, const sockaddr *to, int tolen)
            {
                return ::sendto(fd, buf, len, flags, to, tolen);
            }

            inline int recvfrom(int fd, char *buf, int len, int flags, sockaddr *from, int *fromlen)
            {
                if (from)
                {
                    socklen_t fromlen_ = *fromlen;
                    int ret = ::recvfrom(fd, buf, len, flags, from, &fromlen_);
                    *fromlen = fromlen_;
                    return ret;
                }
                else
                {
                    return ::recvfrom(fd, buf, len, flags, NULL, NULL);
                }
            }

            inline int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timeval *timeout)
            {
                return ::select( nfds, readfds, writefds, exceptfds, const_cast<struct timeval *>(timeout) );
            }

            inline int accept(int fd, sockaddr *addr, int *addrlen)
            {
                socklen_t addrlen_ = *addrlen;
                int ret = ::accept(fd, addr, &addrlen_);
                *addrlen = addrlen_;
                return ret;
            }

            inline int connect(int fd, const sockaddr *name, int namelen)
            {
                return ::connect(fd, name, namelen);
            }

            inline int closesocket(int fd)                                     
            {
                return ::close(fd);
            }

            inline void setblocking(SOCKET fd, bool bBlocking)                 
            {
                int arg = bBlocking ? 0 : 1;
                ::ioctl(fd, FIONBIO, &arg);
            }

            inline int GetLastError()                                          
            {
                return errno;
            }

            static const int ERR_EWOULDBLOCK = EWOULDBLOCK;
            static const int ERR_EINPROGRESS = EINPROGRESS;
            static const int ERR_ECONNRESET = ECONNRESET;
            static const int ERR_ECONNABORTED = ECONNABORTED;
            static const int ERR_ECONNREFUSED = ECONNREFUSED;
            static const int ERR_EMSGSIZE = EMSGSIZE;

        } //namespace BsdSockets

    } // namespace OS;

} // namespace Platform

#endif //! INCLUDE_UTIL_PLATFORM_OS_UNIX_BSDSOCKETS_HPP
