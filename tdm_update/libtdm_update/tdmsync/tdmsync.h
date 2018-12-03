#ifndef _TDM_SYNC_H_028848_
#define _TDM_SYNC_H_028848_

#include <stdint.h>
#include <vector>
#include <stdexcept>
#include "fileio.h"


namespace TdmSync {

//base exception thrown by tdmsync when something fails
struct BaseError : public std::runtime_error {
    BaseError(const std::string &message) : std::runtime_error(message) {}
};

//an element of update plan: says that some segment should be taken from some place
struct SegmentUse {
    //start of the segment in the resulting file (i.e. in remote file = local file after update)
    int64_t dstOffset = 0;
    //start of the segment in: local file (remote = false) / file with downloaded parts (remote = true)
    int64_t srcOffset = 0;
    //length of the segment (in bytes)
    int64_t size = 0;
    //the data for this segment is taken from: local file (remote = false) / remote file (remote = true)
    bool remote = false;
};

//full instructions for turning the existing local file into the specified remote file
struct UpdatePlan {
    //array of segments covering the resulting file
    //local segments go first, remote segments go then (both sorted by offset in the resulting file)
    std::vector<SegmentUse> segments;
    //stats: how many bytes are taken from local file / must be downloaded from remote file
    int64_t bytesLocal = 0;
    int64_t bytesRemote = 0;

    //creates the file with all remote segments from "remote" file (when it is actually located on same machine)
    //note: if remote file is on web server, then use CurlDownloader::downloadMissingParts instead
    void createDownloadFile(BaseFile &rdRemoteFile, BaseFile &wrDownloadFile) const;

    //patch the local file according to this plan
    //rdLocalFile --- initial version of local file (against which the plan was devised)
    //rdDownloadFile --- file with all remote segments downloaded and concatenated in their order
    //wrResultFile --- the resulting file where the patched version will be constructed
    void apply(BaseFile &rdLocalFile, BaseFile &rdDownloadFile, BaseFile &wrResultFile) const;

    //(debug) print the plan to stdout
    void print() const;
};

#pragma pack(push, 1)
//information about one block of remote file (stored in the metainfo file)
struct BlockInfo {
    static const int HASH_SIZE = 20;    //SHA-1 is 160-bit
    //position of block start (size is always FileInfo::blockSize)
    int64_t offset = 0;
    //rolling checksum of this block
    uint32_t chksum = 0;
    //slow and good hash of the block (SHA-1)
    uint8_t hash[HASH_SIZE];
};
#pragma pack(pop)

//full metainfo about the remote file
struct FileInfo {
    //length of the whole file
    int64_t fileSize = 0;
    //size of every block of file
    int blockSize = 0;
    //information about all the blocks of file
    //blocks are sorted by their checksum
    //physically last block usually slightly overlaps with the prelast one
    std::vector<BlockInfo> blocks;

    //save this metainfo into file
    void serialize(BaseFile &wrFile) const;
    //load this metainfo from file
    void deserialize(BaseFile &rdFile);

    //compute metainfo for the specified file
    //completely overwrites this object with new info
    void computeFromFile(BaseFile &rdFile, int blockSize);

    //devise update plan, which could turn specified local file into the remote file with this metainfo
    UpdatePlan createUpdatePlan(BaseFile &rdFile) const;
};

}

#endif
