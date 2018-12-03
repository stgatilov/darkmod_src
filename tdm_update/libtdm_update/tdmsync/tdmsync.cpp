#pragma warning(disable: 4244)	//conversion from 'uint64_t' to 'int', possible loss of data
#pragma warning(disable: 4018)	//'<' : signed/unsigned mismatch
#include "tdmsync.h"
#include <inttypes.h>
#include <vector>
#include <algorithm>

#include "tsassert.h"

//specifies which search algorithm to use to find similar blocks in metainfo
//perfect hash function is used when macro is defined, branchless binary search is used otherwise
//PHF is known to be faster but is a bit more complicated (if it has bugs, then it can even hang)
#define USE_PHF

//specifies which rolling hash (checksum) to use for blocks
//note: this macro affects format of .tdmsync files!
//simple polynomial hash is used when macro is defined, buzhash is used otherwise
//while buzhash is faster on low level, it has no collision guarantees,
//  has low quality, and exhibits quadratic behavior easily
#define USE_POLYHASH

#include "sha1.h"

#ifndef USE_POLYHASH
    #include "buzhash.h"
#else
    #include "polyhash.h"
#endif

#ifndef USE_PHF
    #include "binsearch.h"
#else
    #include "phf.h"
#endif


namespace TdmSync {

//===========================================================================

uint32_t checksumDigest(uint32_t value) {
    return value;
}

uint32_t checksumCompute(const uint8_t *bytes, size_t len) {
    TdmSyncAssert((len & 31) == 0);
#ifndef USE_POLYHASH
    return buzhash_compute(bytes, len);
#else
    return polyhash_compute(bytes, len);
#endif
}

uint32_t checksumUpdate(uint32_t value, uint8_t added, uint8_t removed) {
#ifndef USE_POLYHASH
    return buzhash_fast_update(value, added, removed);
#else
    return polyhash_fast_update(value, added, removed);
#endif
}

void hashCompute(uint8_t hash[20], const uint8_t *bytes, uint32_t len) {
    SHA1_CTX sha;
    SHA1Init(&sha);
    SHA1Update(&sha, bytes, len);
    SHA1Final(hash, &sha);
}

//===========================================================================

static const char MAGIC_STRING[] = "tdmsync.";

void FileInfo::serialize(BaseFile &wrFile) const {
    wrFile.write(MAGIC_STRING, strlen(MAGIC_STRING));

    uint64_t blocksCount = blocks.size();
    wrFile.write(&fileSize, sizeof(fileSize));
    wrFile.write(&blockSize, sizeof(blockSize));
    wrFile.write(&blocksCount, sizeof(blocksCount));
    wrFile.write(blocks.data(), blocksCount * sizeof(BlockInfo));

    wrFile.write(MAGIC_STRING, strlen(MAGIC_STRING));
}

void FileInfo::deserialize(BaseFile &rdFile) {
    char magic[sizeof(MAGIC_STRING)] = {0};
    rdFile.read(magic, strlen(MAGIC_STRING));
    TdmSyncAssert(strcmp(magic, MAGIC_STRING) == 0);

    uint64_t blocksCount;
    rdFile.read(&fileSize, sizeof(fileSize));
    rdFile.read(&blockSize, sizeof(blockSize));
    rdFile.read(&blocksCount, sizeof(blocksCount));
    blocks.resize(blocksCount);
    rdFile.read(blocks.data(), blocksCount * sizeof(BlockInfo));

    rdFile.read(magic, strlen(MAGIC_STRING));
    TdmSyncAssert(strcmp(magic, MAGIC_STRING) == 0);
}


static void readToBuffer(BaseFile &rdFile, std::vector<uint8_t> &buffer, size_t readmore) {
    memmove(buffer.data(), buffer.data() + readmore, buffer.size() - readmore);
    rdFile.read(buffer.data() + buffer.size() - readmore, readmore);
}

void FileInfo::computeFromFile(BaseFile &rdFile, int blockSize) {
    this->blockSize = blockSize;
    fileSize = rdFile.getSize();
    TdmSyncAssert(rdFile.tell() == 0);
    blocks.clear();

    //always download whole file if its size is less than block size
    if (fileSize < blockSize)
        return;

    int blockCount = (fileSize + blockSize-1) / blockSize;
    blocks.reserve(blockCount);

    std::vector<uint8_t> buffer(blockSize);
    int64_t offset = 0;
    for (int i = 0; i < blockCount; i++) {
        //note: the last block always has same size and ends at the end of file
        //so it usually overlaps the pre-last block
        int64_t readmore = std::min(fileSize - offset, int64_t(blockSize));
        readToBuffer(rdFile, buffer, readmore);
        offset += readmore;

        BlockInfo blk;
        blk.offset = offset - blockSize;
        blk.chksum = checksumDigest(checksumCompute(buffer.data(), blockSize));
        hashCompute(blk.hash, buffer.data(), blockSize);

        blocks.push_back(blk);
    }
    TdmSyncAssert(offset == fileSize);
    TdmSyncAssert(rdFile.tell() == fileSize);

    std::sort(blocks.begin(), blocks.end(), [](const BlockInfo &a, const BlockInfo &b) -> bool {
        if (a.chksum != b.chksum)
            return a.chksum < b.chksum;     //main condition: sort by checksum
        return a.offset < b.offset;         //secondary condition: make order deterministic
    });
}

UpdatePlan FileInfo::createUpdatePlan(BaseFile &rdFile) const {
    int64_t srcFileSize = rdFile.getSize();
    TdmSyncAssert(rdFile.tell() == 0);
    UpdatePlan result;

    if (srcFileSize >= blockSize) {
        //copy checksums into simple array, prepare search algorithm on them
        size_t num = blocks.size();
        std::vector<uint32_t> checksums(num);
        for (int i = 0; i < num; i++)
            checksums[i] = blocks[i].chksum;
        TdmSyncAssert(std::is_sorted(checksums.begin(), checksums.end()));
        #ifdef USE_PHF
        TdmPhf::PerfectHashFunc perfecthash;
        perfecthash.create(checksums.data(), num);
        #else
        tdm_bsb_info binsearcher;
        binary_search_branchless_precompute(&binsearcher, num);
        #endif

        //buffer with the latest data from local file
        //when sliding window gets to the end of buffer, we move remaining data to start and read some more
        std::vector<uint8_t> buffer(2 * blockSize);
        rdFile.read(buffer.data(), std::min((int64_t)buffer.size(), srcFileSize));
        uint32_t currChksum = checksumCompute(buffer.data(), blockSize);
        //the current sliding window ends at this position in buffer
        size_t buffPtr = blockSize;

        //for each block from metainfo file: whether it has already been found in local file
        std::vector<char> foundBlocks(blocks.size(), false);
        uint64_t sumCount = 0;

        //the current sliding window starts at "offset" position within local file
        for (int64_t offset = 0; offset + blockSize <= srcFileSize; offset++) {
            uint32_t digest = checksumDigest(currChksum);

            #ifdef USE_PHF
            size_t idx = perfecthash.evaluate(digest);
            #else
            size_t idx = binary_search_branchless_run(&binsearcher, checksums.data(), digest);
            #endif

            if (idx < num && checksums[idx] == digest) {
                //at least one block's checksum equals checksum of the current window
                uint32_t left = idx;
                uint32_t right = left;
                while (right < num && checksums[right] == digest)
                    right++;

                sumCount += (right - left);
                //optimization: do not compute slow hash of current window, if we already found matches for all block candidates 
                int newFound = 0;
                for (int j = left; j < right; j++) if (!foundBlocks[j])
                    newFound++;

                if (newFound > 0) {
                    uint8_t currHash[BlockInfo::HASH_SIZE];
                    hashCompute(currHash, &buffer[buffPtr - blockSize], blockSize);

                    for (int j = left; j < right; j++) if (!foundBlocks[j]) {
                        if (memcmp(blocks[j].hash, currHash, sizeof(currHash)) != 0)
                            continue;   //note: this happens only due to checksum collisions, i.e. very rarely

                        foundBlocks[j] = true;
                        SegmentUse seg;
                        seg.srcOffset = offset;
                        seg.dstOffset = blocks[j].offset;
                        seg.size = blockSize;
                        seg.remote = false;
                        result.segments.push_back(seg);
                    }
                }
            }

            if (offset + blockSize == srcFileSize)
                break;  //end of local file
            if (buffPtr == buffer.size()) {
                //current sliding window hit the end of the buffer
                size_t readmore = std::min((int64_t)buffer.size() - blockSize, srcFileSize - offset - blockSize);
                readToBuffer(rdFile, buffer, readmore);
                buffPtr -= readmore;
            }
            //move current window by one byte and update rolling checksum
            currChksum = checksumUpdate(currChksum, buffer[buffPtr], buffer[buffPtr - blockSize]);
            buffPtr++;
        }
        double avgCandidates = double(sumCount) / double(srcFileSize - blockSize + 1.0);
        //fprintf(stderr, "Average candidates per window: %0.3g\n", avgCandidates);
    }

    int n = 0;
    if (!result.segments.empty()) {
        std::sort(result.segments.begin(), result.segments.end(), [](const SegmentUse &a, const SegmentUse &b) -> bool {
            return a.dstOffset < b.dstOffset;
        });
        n = 1;
        //concatenate found local blocks into larger segments (wherever possible)
        for (int i = 1; i < result.segments.size(); i++) {
            const auto &curr = result.segments[i];
            auto &last = result.segments[n-1];
            if (last.dstOffset + last.size == curr.dstOffset && last.srcOffset + last.size == curr.srcOffset)
                last.size += curr.size;
            else
                result.segments[n++] = curr;
        }
        result.segments.resize(n);
    }

    int64_t lastCovered = 0;
    int64_t downloadSize = 0;
    //detect all uncovered blocks in metainfo and create remote segments for them
    for (int i = 0; i <= n; i++) {
        int64_t offset = i < n ? result.segments[i].dstOffset : fileSize;
        int64_t size = i < n ? result.segments[i].size : 0;
        if (offset > lastCovered) {
            SegmentUse seg;
            seg.srcOffset = downloadSize;
            seg.dstOffset = lastCovered;
            seg.size = offset - lastCovered;
            seg.remote = true;
            result.segments.push_back(seg);
            downloadSize += seg.size;
        }
        lastCovered = offset + size;
    }

    n = result.segments.size();
    for (int i = 0; i < n; i++) {
        const auto &seg = result.segments[i];
        (seg.remote ? result.bytesRemote : result.bytesLocal) += seg.size;
    }
    TdmSyncAssert(result.bytesRemote == downloadSize);

    return result;
}

//===========================================================================

void UpdatePlan::print() const {
    printf("Total bytes:  local=%" PRId64 "  remote=%" PRId64 "\n", bytesLocal, bytesRemote);
    printf("Segments = %d:\n", (int)segments.size());
    for (int i = 0; i < segments.size(); i++) {
        const auto &seg = segments[i];
        printf("  %c %08X: %08" PRIX64 " <- %08" PRIX64 "\n", (seg.remote ? 'R' : 'L'), (int)seg.size, seg.dstOffset, seg.srcOffset);
    }
    printf("\n");
}

static void copyfile(BaseFile &wr, BaseFile &rd, uint64_t size) {
    uint8_t buffer[65536];
    for (uint64_t pos = 0, chunk = 0; pos < size; pos += chunk) {
        chunk = std::min(size_t(size - pos), sizeof(buffer));
        rd.read(buffer, chunk);
        wr.write(buffer, chunk);
    }
}

void UpdatePlan::apply(BaseFile &rdLocalFile, BaseFile &rdDownloadFile, BaseFile &wrResultFile) const {
    uint64_t resSize = 0;
    if (!segments.empty()) {
        const auto &last = segments.back();
        resSize = last.dstOffset + last.size;
    }

    for (int i = 0; i < segments.size(); i++) {
        const auto &seg = segments[i];
        auto &srcFile = seg.remote ? rdDownloadFile : rdLocalFile;
        wrResultFile.seek(seg.dstOffset);
        srcFile.seek(seg.srcOffset);
        copyfile(wrResultFile, srcFile, seg.size);
    }
}

void UpdatePlan::createDownloadFile(BaseFile &rdRemoteFile, BaseFile &wrDownloadFile) const {
    for (int i = 0; i < segments.size(); i++) {
        const auto &seg = segments[i];
        if (seg.remote) {
            TdmSyncAssert(wrDownloadFile.tell() == seg.srcOffset);
            rdRemoteFile.seek(seg.dstOffset);
            copyfile(wrDownloadFile, rdRemoteFile, seg.size);
        }
    }
}

}
