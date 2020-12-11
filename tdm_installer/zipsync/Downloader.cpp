#include "Downloader.h"
#include <curl/curl.h>
#include <algorithm>
#include "Logging.h"
#include "StdString.h"
#include <string.h>
#undef min
#undef max


//try to avoid CURL requests of total size less than this
static const int DESIRED_REQUEST_SIZE = 10<<20;
//forbid multipart requests with more that this number of chunks
static const int MAX_PARTS_PER_REQUEST = 20;
//overhead per download in bytes --- for progress callback only
static const int ESTIMATED_DOWNLOAD_OVERHEAD = 100;

namespace ZipSync {

//note: HTTP header field names are case-insensitive
//however, we cannot just lowercase all headers, since multipary boundary is case-sensitive
const char *CheckHttpPrefix(const std::string &line, const std::string &prefix) {
    if (!stdext::istarts_with(line, prefix))
        return nullptr;
    const char *rest = line.c_str() + prefix.size();
    return rest;
}

DownloadSource::DownloadSource() { byterange[0] = byterange[1] = 0; }
DownloadSource::DownloadSource(const std::string &url) : url(url) { byterange[0] = 0; byterange[1] = UINT32_MAX; }
DownloadSource::DownloadSource(const std::string &url, uint32_t from, uint32_t to) : url(url) { byterange[0] = from; byterange[1] = to; }


Downloader::~Downloader() {}
Downloader::Downloader() : _curlHandle(nullptr, curl_easy_cleanup) {}

void Downloader::EnqueueDownload(const DownloadSource &source, const DownloadFinishedCallback &finishedCallback) {
    _downloads.push_back(Download{source, finishedCallback});
}
void Downloader::SetProgressCallback(const GlobalProgressCallback &progressCallback) {
    _progressCallback = progressCallback;
}
void Downloader::SetErrorMode(bool silent) {
    _silentErrors = silent;
}
void Downloader::SetUserAgent(const char *useragent) {
    if (useragent)
        _useragent.reset(new std::string(useragent));
    else
        _useragent.reset();
}

void Downloader::DownloadAll() {
    if (_progressCallback)
        _progressCallback(0.0, "Downloading started");

    _curlHandle.reset(curl_easy_init());

    for (int i = 0; i <  _downloads.size(); i++)
        _urlStates[_downloads[i].src.url].downloadsIds.push_back(i);
    for (auto &pKV : _urlStates) {
        std::string url = pKV.first;
        std::vector<int> &ids = pKV.second.downloadsIds;
        std::sort(ids.begin(), ids.end(), [this](int a, int b) {
           return _downloads[a].src.byterange[0] < _downloads[b].src.byterange[0];
        });
    }

    for (const auto &pKV : _urlStates) {
        std::string url = pKV.first;
        try {
            DownloadAllForUrl(url);
        }
        catch(const ErrorException &e) {
            if (!_silentErrors)
                throw e;    //rethrow further to caller
            else
                {}          //supress exception, continue with other urls
        }
    }

    ZipSyncAssert(_curlHandle.get_deleter() == curl_easy_cleanup);
    _curlHandle.reset();

    if (_progressCallback)
        _progressCallback(1.0, "Downloading finished");
}

void Downloader::DownloadAllForUrl(const std::string &url) {
    UrlState &state = _urlStates.find(url)->second;
    int n = state.downloadsIds.size();

    while (state.doneCnt < n) {
        uint64_t totalSize = 0;
        int rangesCnt = 0;
        int end = state.doneCnt;
        uint32_t last = UINT32_MAX;
        std::vector<int> ids;
        do {
            int idx = state.downloadsIds[end++];
            const Download &down = _downloads[idx];
            ids.push_back(idx);
            totalSize += down.src.byterange[1] - down.src.byterange[0];
            rangesCnt += (last != down.src.byterange[0]);
            last = down.src.byterange[1];
        } while (end < n && rangesCnt < MAX_PARTS_PER_REQUEST && totalSize < DESIRED_REQUEST_SIZE);

        DownloadOneRequest(url, ids);

        state.doneCnt = end;
    }
}

void Downloader::DownloadOneRequest(const std::string &url, const std::vector<int> &downloadIds) {
    if (downloadIds.empty())
        return;

    std::vector<std::pair<uint32_t, uint32_t>> coaslescedRanges;
    for (int idx : downloadIds) {
        const auto &down = _downloads[idx];
        if (!coaslescedRanges.empty() && coaslescedRanges.back().second >= down.src.byterange[0])
            coaslescedRanges.back().second = std::max(coaslescedRanges.back().second, down.src.byterange[1]);
        else
            coaslescedRanges.push_back(std::make_pair(down.src.byterange[0], down.src.byterange[1]));
    }
    std::string byterangeStr;
    for (auto rng : coaslescedRanges) {
        if (!byterangeStr.empty())
            byterangeStr += ",";
        byterangeStr += std::to_string(rng.first) + "-";
        if (rng.second != UINT32_MAX)   //-1 means "up to the end"
            byterangeStr += std::to_string(rng.second - 1);
    }

    int64_t totalEstimate = 0;
    int64_t thisEstimate = 0;
    for (int idx : downloadIds)
        thisEstimate += BytesToTransfer(_downloads[idx]);
    for (const auto &down : _downloads)
        totalEstimate += BytesToTransfer(down);

    auto header_callback = [](char *buffer, size_t size, size_t nitems, void *userdata) {
        size *= nitems;
        auto &resp = *((Downloader*)userdata)->_currResponse;
        std::string str(buffer, buffer + size);
        size_t from, to, all;
        if (const char *tail = CheckHttpPrefix(str, "Content-Range: bytes ")) {
            if (sscanf(tail, "%zu-%zu/%zu", &from, &to, &all) == 3) {
                resp.onerange[0] = from;
                resp.onerange[1] = to + 1;
            }
        }
        char boundary[128] = {0};
        if (const char *tail = CheckHttpPrefix(str, "Content-Type: multipart/byteranges; boundary=")) {
            if (sscanf(tail, "%s", boundary) == 1) {
                resp.boundary = std::string("\r\n--") + boundary;// + "\r\n";
            }
        }
        return size;
    };
    auto write_callback = [](char *buffer, size_t size, size_t nitems, void *userdata) -> size_t {
        size *= nitems;
        auto &resp = *((Downloader*)userdata)->_currResponse;
        if (resp.onerange[0] == resp.onerange[1] && resp.boundary.empty())
            return 0;  //neither range nor multipart response -> halt
        resp.data.insert(resp.data.end(), buffer, buffer + size);
        return size;
    };
    auto xferinfo_callback = [](void *userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
        auto &resp = *((Downloader*)userdata)->_currResponse;
        if (dltotal > 0 && dlnow > 0) {
            resp.progressRatio = double(dlnow) / std::max(dltotal, dlnow);
            resp.bytesDownloaded = dlnow;
            if (int code = ((Downloader*)userdata)->UpdateProgress())
                return code;   //interrupt!
        }
        return 0;
    };
    _currResponse.reset(new CurlResponse());
    _currResponse->url = url;
    _currResponse->progressWeight = double(thisEstimate) / totalEstimate;
    CURL *curl = _curlHandle.get();
    if (_useragent)
        curl_easy_setopt(curl, CURLOPT_USERAGENT, _useragent->c_str());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_RANGE, byterangeStr.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (curl_write_callback)write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, (curl_write_callback)header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, (curl_xferinfo_callback)xferinfo_callback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    UpdateProgress();
    CURLcode ret = curl_easy_perform(curl);
    long httpRes = 0;
    if (ret == CURLE_ABORTED_BY_CALLBACK)
        g_logger->errorf(lcUserInterrupt, "Interrupted by user");
    curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &httpRes);
    ZipSyncAssertF(httpRes != 404, "not found result for URL %s", url.c_str());
    ZipSyncAssertF(ret != CURLE_WRITE_ERROR, "Response without byteranges for URL %s", url.c_str());
    ZipSyncAssertF(ret == CURLE_OK, "Unexpected CURL error %d on URL %s", ret, url.c_str());
    ZipSyncAssertF(httpRes == 200 || httpRes == 206, "Unexpected HTTP return code %d for URL %s", httpRes, url.c_str());
    _currResponse->progressRatio = 1.0;
    UpdateProgress();

    std::vector<CurlResponse> results;
    if (_currResponse->boundary.empty())
        results.push_back(std::move(*_currResponse));
    else
        BreakMultipartResponse(*_currResponse, results);

    _totalBytesDownloaded += _currResponse->bytesDownloaded;
    _totalProgress += _currResponse->progressWeight;
    _currResponse.reset();

    std::sort(results.begin(), results.end(), [](const CurlResponse &a, const CurlResponse &b) {
        return a.onerange[0] < b.onerange[0];
    });
    for (int idx : downloadIds) {
        const auto &downSrc = _downloads[idx].src;
        uint32_t totalSize = UINT32_MAX;
        std::vector<uint8_t> answer;
        if (downSrc.byterange[1] != UINT32_MAX) {
            totalSize = downSrc.byterange[1] - downSrc.byterange[0];
            answer.reserve(totalSize);
        }
        for (const auto &resp : results) {
            uint32_t currPos = downSrc.byterange[0] + (uint32_t)answer.size();
            uint32_t left = std::max(currPos, resp.onerange[0]);
            uint32_t right = std::min(downSrc.byterange[1], resp.onerange[1]);
            if (right <= left)
                continue;
            ZipSyncAssertF(left == currPos, "Missing chunk %u..%u (%u bytes) after downloading URL %s", left, currPos, currPos - left, url.c_str());
            answer.insert(answer.end(),
                resp.data.data() + (left - resp.onerange[0]),
                resp.data.data() + (right - resp.onerange[0])
            );
        }
        if (downSrc.byterange[1] != UINT32_MAX) {
            ZipSyncAssertF(answer.size() == totalSize, "Missing end chunk %zu..%u (%u bytes) after downloading URL %s", answer.size(), totalSize, totalSize - (uint32_t)answer.size(), url.c_str());
        }
        _downloads[idx].finishedCallback(answer.data(), answer.size());
    }
}

void Downloader::BreakMultipartResponse(const CurlResponse &response, std::vector<CurlResponse> &parts) {
    const auto &data = response.data;
    const std::string &bound = response.boundary;

    //find all occurences of boundary
    std::vector<size_t> boundaryPos;
    for (size_t pos = 0; pos + bound.size() <= data.size(); pos++)
        if (memcmp(&data[pos], &bound[0], bound.size()) == 0)
            boundaryPos.push_back(pos);

    for (size_t i = 0; i+1 < boundaryPos.size(); i++) {
        size_t left = boundaryPos[i] + bound.size() + 2;        //+2 for "\r\n" or "--"
        size_t right = boundaryPos[i+1];

        //parse header into sequence of lines
        std::vector<std::string> header;
        size_t lineStart = left;
        while (1) {
            size_t lineEnd = lineStart;
            while (strncmp((char*)&data[lineEnd], "\r\n", 2))
                lineEnd++;
            header.emplace_back((char*)&data[lineStart], (char*)&data[lineEnd]);
            lineStart = lineEnd + 2;
            if (header.back().empty())
                break;  //empty line: header has ended
        }

        //find range in headers
        CurlResponse part;
        for (const auto &h : header) {
            size_t from, to, all;
            if (const char *tail = CheckHttpPrefix(h, "Content-Range: bytes ")) {
                if (sscanf(tail, "%zu-%zu/%zu", &from, &to, &all) == 3) {
                    part.onerange[0] = from;
                    part.onerange[1] = to + 1;
                }
            }
        }
        ZipSyncAssertF(part.onerange[0] != part.onerange[1], "Failed to find range in part headers");

        part.data.assign(&data[lineStart], &data[right]);
        parts.push_back(std::move(part));
    }
}

int Downloader::UpdateProgress() {
    char buffer[256] = "Downloading...";
    double progress = _totalProgress;
    if (_currResponse) {
        sprintf(buffer, "Downloading \"%s\"...", _currResponse->url.c_str());
        progress += _currResponse->progressWeight * _currResponse->progressRatio;
    }
    if (_progressCallback) {
        int code = _progressCallback(progress, buffer);
        return code;
    }
    return 0;
}

size_t Downloader::BytesToTransfer(const Download &download) {
    size_t dataSize = download.src.byterange[1] - download.src.byterange[0];
    if (download.src.byterange[1] == UINT32_MAX)
        dataSize = (1<<20); //a wild guess =)
    return dataSize + ESTIMATED_DOWNLOAD_OVERHEAD;
}

}
