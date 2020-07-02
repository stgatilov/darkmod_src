#pragma once

#include <string>
#include <stdint.h>

struct MHD_Daemon;
struct MHD_Connection;

//workaround for ssize_t-included errors on MSVC
#ifdef _MSC_VER
    #define _SSIZE_T_DEFINED
    #define ssize_t size_t
#endif

namespace ZipSync {

/**
 * Simple embedded HTTP server.
 * Used only for tests.
 */
class HttpServer {
    std::string _rootDir;
    MHD_Daemon *_daemon = nullptr;
    int _port = -1;
    int _blockSize = -1;

public:
    static const int PORT_DEFAULT = 8090;

    HttpServer();
    ~HttpServer();
    HttpServer(const HttpServer &) = delete;
    HttpServer& operator=(const HttpServer &) = delete;

    //set the root directory so serve files inside
    void SetRootDir(const std::string &root);
    void SetPortNumber(int port = PORT_DEFAULT);
    void SetBlockSize(int blockSize = 128*1024);
    std::string GetRootUrl() const;

    void Start();
    void Stop();

private:
    class FileDownload;
    class MultipartDownload;

    static int MhdFunction(
        void *cls,
        MHD_Connection *connection,
        const char *url,
        const char *method,
        const char *version,
        const char *upload_data,
        size_t *upload_data_size,
        void **ptr
    );
    int AcceptCallback(
        MHD_Connection *connection,
        const char *url,
        const char *method,
        const char *version
    ) const;
};

}
