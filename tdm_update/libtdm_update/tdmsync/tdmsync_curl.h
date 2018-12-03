#ifndef _TDM_SYNC_CURL_H_328817_
#define _TDM_SYNC_CURL_H_328817_

#include "tdmsync.h"
#include <string>

#include <curl/curl.h>


namespace TdmSync {

struct HttpError : public BaseError {
    int code;
    HttpError(const char *message, int code) : BaseError(message + std::to_string(code)), code(code) {}
};

//implements tdmsync differential update over HTTP 1.1 protocol (using curl)
class CurlDownloader {
public:
    //download the metainfo file from specified url into specified file
    //you can then deserialize it and create an update plan for local file using it
    void downloadMeta(BaseFile &wrDownloadFile, const char *url);

    //download into specified file all the remote segments of the specified update plan from the specified url
    //this invokes multi-byte-range HTTP requests which needs proper web server support
    void downloadMissingParts(BaseFile &wrDownloadFile, const UpdatePlan &plan, const char *url);

    enum DownloadMode {
        dmUnknown,              //not yet done anything =)
        dmNone,                 //nothing to download: file already correct
        dmSingleByterange,      //only one chunk was downloaded (using byterange request)
        dmMultipartByterange,   //used multipart byteranges request to download all chunks
        dmManyByteranges,       //had to fallback to many requests with single byterange in each
    };
    //call after the request to learn which download mode was used
    //usually used for status/logging
    DownloadMode getModeUsed() const { return usedMode; }

private:
    struct WorkRange;

    void clear();

    size_t headerWriteCallback(char *ptr, size_t size, size_t nmemb);
    size_t plainWriteCallback(char *ptr, size_t size, size_t nmemb);

    int performSingle();
    size_t singleWriteCallback(char *ptr, size_t size, size_t nmemb, WorkRange *work);

    int performMulti();
    size_t multiWriteCallback(char *ptr, size_t size, size_t nmemb);
    bool processBuffer(bool flush);
    int findBoundary(const char *ptr, int from, int to) const;

    int performMany();

private:
    //input data from user
    BaseFile *downloadFile = nullptr;
    const UpdatePlan *plan = nullptr;
    std::string url;

    //byte ranges we have to download
    int64_t totalCount = 0, totalSize = 0;
    std::vector<std::string> ranges;
    std::string rangesString;

    //intermediate data: header / boundary of HTTP response
    std::string header, boundary;
    bool isHttp = false, acceptRanges = false;
    DownloadMode usedMode = dmUnknown;
    long httpCode = 0;

    //how much bytes we have written to file
    struct WorkRange {
        int64_t start = 0, end = 0;
        int64_t written = 0;
    };
    WorkRange mainWorkRange;

    //intermediate data: only for "performMulti"
    static const int BufferSize = 16 << 10;
    std::vector<char> bufferData;
    int bufferAvail = 0;
};

}

#endif
