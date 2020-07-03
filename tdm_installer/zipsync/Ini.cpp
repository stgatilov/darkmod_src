#include "Ini.h"
#include "Utils.h"
#include "StdString.h"
#include "Logging.h"
#include "ZipUtils.h"
#include "Hash.h"
#include <string.h>


namespace ZipSync {

void WriteIniFile(const char *path, const IniData &data, IniMode mode) {
    ZipSyncAssertF(path[0], "Path to write INI file is empty");
    std::string text;
    for (const auto &pNS : data) {
        char buffer[SIZE_LINEBUFFER];
        sprintf(buffer, "[%s]\n", pNS.first.c_str());
        text.append(buffer);
        for (const auto &pKV : pNS.second) {
            sprintf(buffer, "%s=%s\n", pKV.first.c_str(), pKV.second.c_str());
            text.append(buffer);
        }
        text.append("\n");
    }
    if (mode == IniMode::Auto)
        mode = (path[strlen(path)-1] == 'z' ? IniMode::Zipped : IniMode::Plain);
    if (mode == IniMode::Zipped) {
        std::string hash = "zsMH:" + Hasher().Update(text.data(), text.size()).Finalize().Hex();
        ZipFileHolder zf(path);
        zip_fileinfo info = {0};
        info.dosDate = 0x28210000;  //1 January 2000 --- set it just to make date valid
        SAFE_CALL(zipOpenNewFileInZip(zf, "hash.txt", &info, NULL, 0, NULL, 0, NULL, Z_NO_COMPRESSION, Z_NO_COMPRESSION));
        SAFE_CALL(zipWriteInFileInZip(zf, hash.data(), hash.size()));
        SAFE_CALL(zipCloseFileInZip(zf));
        SAFE_CALL(zipOpenNewFileInZip(zf, "data.ini", &info, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION));
        SAFE_CALL(zipWriteInFileInZip(zf, text.data(), text.size()));
        SAFE_CALL(zipCloseFileInZip(zf));
    }
    else {
        StdioFileHolder f(path, "wb");
        fwrite(text.data(), 1, text.size(), f);
    }
}

IniData ReadIniFile(const char *path, IniMode mode) {
    std::vector<char> text;
    if (mode == IniMode::Auto)
        mode = (path[strlen(path)-1] == 'z' ? IniMode::Zipped : IniMode::Plain);
    if (mode == IniMode::Zipped) {
        UnzFileHolder zf(path);
        SAFE_CALL(unzLocateFile(zf, "data.ini", true));
        unz_file_info info;
        SAFE_CALL(unzGetCurrentFileInfo(zf, &info, NULL, 0, NULL, 0, NULL, 0));
        SAFE_CALL(unzOpenCurrentFile(zf));
        text.resize(info.uncompressed_size);
        int read = unzReadCurrentFile(zf, text.data(), text.size());
        ZipSyncAssert(read == text.size());
        SAFE_CALL(unzCloseCurrentFile(zf));
    }
    else {
        StdioFileHolder f(path, "rb");
        char buffer[SIZE_FILEBUFFER];
        while (int bytes = fread(buffer, 1, SIZE_FILEBUFFER, f))
            text.insert(text.end(), buffer, buffer+bytes);
    }

    IniData ini;
    IniSect sec;
    std::string name;
    auto CommitSec = [&]() {
        if (!name.empty())
            ini.push_back(std::make_pair(std::move(name), std::move(sec)));
        name.clear();
        sec.clear();
    };
    size_t textPos = 0;
    while (textPos < text.size()) {
        size_t eolPos = std::find(text.begin() + textPos, text.end(), '\n') - text.begin();
        std::string line(text.begin() + textPos, text.begin() + eolPos);
        textPos = eolPos + 1;

        stdext::trim(line);
        if (line.empty())
            continue;
        if (line[0] == '#') //comment
            continue;
        if (line.front() == '[' && line.back() == ']') {
            CommitSec();
            name = line.substr(1, line.size() - 2);
        }
        else {
            size_t pos = line.find('=');
            ZipSyncAssertF(pos != std::string::npos, "Cannot parse ini line: %s", line.c_str());
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos+1);
            sec.push_back(std::make_pair(std::move(key), std::move(value)));
        }
    }
    CommitSec();
    return ini;
}

}
