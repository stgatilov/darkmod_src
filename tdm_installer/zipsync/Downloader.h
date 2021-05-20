#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <limits.h>


typedef void CURL;

namespace ZipSync {

/**
 * A chunk of data which we want to download
 */
struct DownloadSource {
    //URL to download file from
    std::string url;
    //the range of bytes to be downloaded
    uint32_t byterange[2];

    DownloadSource();
    DownloadSource(const std::string &url); //download whole file
    DownloadSource(const std::string &url, uint32_t from, uint32_t to); //download range of file
};

//called when download is complete
typedef std::function<void(const void*, uint32_t)> DownloadFinishedCallback;
//called during download to report progress: returning nonzero value interrupts download
typedef std::function<int(double, const char*)> GlobalProgressCallback;

/**
 * Smart downloader over HTTP protocol.
 * Utilizes multipart byteranges requests to download many chunks quickly.
 */
class Downloader {
    bool _silentErrors = false;
    std::unique_ptr<std::string> _useragent;
    struct Download {
        DownloadSource src;
        DownloadFinishedCallback finishedCallback;
    };
    std::vector<Download> _downloads;
    GlobalProgressCallback _progressCallback;
    struct UrlState {
        int doneCnt = 0;
        std::vector<int> downloadsIds;      //sorted by starting offset
        int speedProfile = 0;               //index in SPEED_PROFILES
    };
    std::map<std::string, UrlState> _urlStates;

    struct CurlResponse {
        std::string url;

        std::vector<uint8_t> data;
        uint32_t onerange[2] = {UINT_MAX, UINT_MAX};
        std::string boundary;

        double progressRatio = 0.0;         //which portion of this CURL request is done
        int64_t bytesDownloaded = 0;
        double progressWeight = 0.0;        //this request size / total size of all downloads
    };
    std::unique_ptr<CurlResponse> _currResponse;
    double _totalProgress = 0.0;            //which portion of DownloadAll is complete (without current request)
    int64_t _totalBytesDownloaded = 0;      //how many bytes downloaded in total (without current request)

    //reuse CURL handle for requests in order to exploit connection pool
    std::unique_ptr<CURL, void (*)(CURL*)> _curlHandle;

public:
    ~Downloader();
    Downloader();

    void EnqueueDownload(const DownloadSource &source, const DownloadFinishedCallback &finishedCallback);
    void SetProgressCallback(const GlobalProgressCallback &progressCallback);
    //silent == false: throw exception on any error up to the caller, stopping the whole run
    //silent == true: just don't call callback function for failed requests (grouped by url), no stopping, no exception
    void SetErrorMode(bool silent);
    void SetUserAgent(const char *useragent = nullptr);
    void DownloadAll();

    int64_t TotalBytesDownloaded() const { return _totalBytesDownloaded; }

private:
    void DownloadAllForUrl(const std::string &url);
    bool DownloadOneRequest(const std::string &url, const std::vector<int> &downloadIds, int lowSpeedTime = 0);
    void BreakMultipartResponse(const CurlResponse &response, std::vector<CurlResponse> &parts);
    int UpdateProgress();
    size_t BytesToTransfer(const Download &download);
};

}
