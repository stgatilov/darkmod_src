#pragma warning(disable: 4244)	//conversion from 'uint64_t' to 'long', possible loss of data
#include "fileio.h"
#include <stdio.h>
#include <stdexcept>
#include "tsassert.h"
#include "tdmsync.h"


namespace TdmSync {

StdioFile::StdioFile() {
    fh = nullptr;
}
StdioFile::~StdioFile() {
    if (fh)
        fclose((FILE*)fh);
}

void StdioFile::open(const char *filename, OpenMode mode) {
    if (fh)
        fclose((FILE*)fh);
    this->mode = mode;
    FILE *f = fopen(filename, mode == Read ? "rb" : "wb");
    TdmSyncAssertF(f, "Failed to open file %s for %s", filename, mode == Read ? "reading" : "writing");
    fh = f;
}

void StdioFile::read(void* data, size_t size) {
    TdmSyncAssert(fh && mode == Read);
    size_t bytes = fread(data, 1, size, (FILE*)fh);
    TdmSyncAssert(bytes == size);
}

void StdioFile::write(const void* data, size_t size) {
    TdmSyncAssert(fh && mode == Write);
    size_t bytes = fwrite(data, 1, size, (FILE*)fh);
    TdmSyncAssert(bytes == size);
}

void StdioFile::seek(uint64_t pos) {
    TdmSyncAssert(fh);
    fseek((FILE*)fh, pos, SEEK_SET);
}

uint64_t StdioFile::tell() {
    TdmSyncAssert(fh);
    return ftell((FILE*)fh);
}

uint64_t StdioFile::getSize() {
    TdmSyncAssert(fh);
    FILE *f = (FILE*)fh;
    uint64_t pos0 = ftell(f);
    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, pos0, SEEK_SET);
    return len;
}

void StdioFile::flush() {
    TdmSyncAssert(fh);
    FILE *f = (FILE*)fh;
    fflush(f);
}

}
