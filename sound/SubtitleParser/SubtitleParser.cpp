#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "SubtitleParser.h"

using namespace std;

std::vector<SubtitleItem*> SubtitleParser::getSubtitles()
{
	return _subtitles;
}

std::string SubtitleParser::getFileData()
{
	std::ifstream infile(_fileName);
	std::string allData = "";
	std::string line;
	while (std::getline(infile, line))
	{
		std::istringstream iss(line);
		allData+=line+'\n';
	}
	return allData;
}

SubtitleParser::SubtitleParser(void)
{
	
}

SubtitleParser::~SubtitleParser(void)
{
}
