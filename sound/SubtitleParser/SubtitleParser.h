#pragma once
#include "SubtitleItem.h"

class SubtitleParser
{
protected:
	std::vector<SubtitleItem*> _subtitles;
	std::string _fileName;
	virtual void parse(std::string fileName) = 0;
public:
	virtual std::vector<SubtitleItem*> getSubtitles();
	std::string getFileData();
	SubtitleParser(void);
        virtual ~SubtitleParser(void);
};

