// OS-specific details - Windows

#ifndef INCLUDE_UTIL_PLATFORM_OS_WINDOWS_BSDSOCKETS_HPP
#define INCLUDE_UTIL_PLATFORM_OS_WINDOWS_BSDSOCKETS_HPP

// If none of these are defined, then Winsock thinks its WIN16 and redefines error messages etc.
#if !defined(WIN16) && !defined(WIN32) && !defined(_WIN64)
#define WIN32
#endif

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include <winsock2.h>
#include <mswsock.h>

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>

#ifndef __WINDOWS__
#define __WINDOWS__
#endif

#if !defined(NDEBUG) && !defined(_DEBUG)
#define _DEBUG
#endif

// compensate for some things lacking in mingw's platform headers
#ifdef __MINGW32__

#ifndef WSAID_ACCEPTEX

typedef
BOOL
(PASCAL FAR * LPFN_ACCEPTEX)(
    IN SOCKET sListenSocket,
    IN SOCKET sAcceptSocket,
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT LPDWORD lpdwBytesReceived,
    IN LPOVERLAPPED lpOverlapped
);

#define WSAID_ACCEPTEX \
{0xb5367df1,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}

#endif // ! WSAID_ACCEPTEX

#ifndef WSAID_GETACCEPTEXSOCKADDRS

typedef
VOID
(PASCAL FAR * LPFN_GETACCEPTEXSOCKADDRS)(
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT struct sockaddr **LocalSockaddr,
    OUT LPINT LocalSockaddrLength,
    OUT struct sockaddr **RemoteSockaddr,
    OUT LPINT RemoteSockaddrLength
    );

#define WSAID_GETACCEPTEXSOCKADDRS \
{0xb5367df2,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}

#endif // ! WSAID_GETACCEPTEXSOCKADDRS

#endif // __MINGW32__


namespace Platform {

    namespace OS {

        inline std::string GetErrorString(int err)
        {
            std::string errorString = "Error string lookup failed";
            LPVOID lpMsgBuf;
            if (FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                err,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (char *) &lpMsgBuf,
                0,
                NULL ))
            {
                errorString = (const char *) lpMsgBuf;
                LocalFree( lpMsgBuf );
            }

            // strip trailing newline characters
            while (errorString.at(errorString.size()-1) <= 13)
            {
                errorString = errorString.substr(0, errorString.size()-1);
            }

            return errorString;
        }

        inline std::string GetErrorString() { return GetErrorString( ::GetLastError() ); }

        namespace BsdSockets {

            typedef int socklen_t;

            inline int recv(int fd, char *buf, int len, int flags)              
            {
                return ::recv(fd, buf, len, flags);
            }

            inline int send(int fd, const char *buf, int len, int flags)
            {
                return ::send(fd, buf, len, flags);
            }

            inline int sendto(int fd, const char *buf, int len, int flags, const sockaddr *to, int tolen)
            {
                return ::sendto(fd, buf, len, flags, to, tolen);
            }

            inline int recvfrom(int fd, char *buf, int len, int flags, sockaddr *from, int *fromlen)
            {
                return ::recvfrom(fd, buf, len, flags, from, fromlen);
            }

            inline int accept(int fd, sockaddr *addr, int *addrlen)
            {
                return (int) ::accept(fd, addr, addrlen);
            }

            inline int connect(int fd, const sockaddr *name, int namelen)       
            {
                return ::connect(fd, name, namelen);
            }

            inline int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timeval *timeout)
            {
                return ::select(nfds, readfds, writefds, exceptfds, timeout );
            }

            inline int closesocket(int fd)                                      
            {
                return ::closesocket(fd);
            }

            inline void setblocking(int fd, bool bBlocking)                     
            {
                u_long arg = bBlocking ? 0 : 1;
                ioctlsocket(fd, FIONBIO, &arg);
            }

            inline int GetLastError()                                              
            {
                return ::WSAGetLastError();
            }

            static const int ERR_EWOULDBLOCK = WSAEWOULDBLOCK;
            static const int ERR_EINPROGRESS = WSAEINPROGRESS;
            static const int ERR_ECONNRESET = WSAECONNRESET;
            static const int ERR_ECONNABORTED = WSAECONNABORTED;
            static const int ERR_ECONNREFUSED = WSAECONNREFUSED;
            static const int ERR_EMSGSIZE = WSAEMSGSIZE;

        } //namespace BsdSockets

    } // namespace OS;

} // namespace Platform

#endif // ! INCLUDE_UTIL_PLATFORM_OS_WINDOWS_BSDSOCKETS_HPP
