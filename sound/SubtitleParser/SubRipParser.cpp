#include <sstream>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <regex>
#include <deque>
#include "SubtitleParser.h"
#include "SubRipParser.h"

SubRipParser::SubRipParser(void)
{
}

void SubRipParser::parse(std::string fileName)
{
	try 
	{
		std::string szRegex = "([0-9]+)\n([0-9]{2}:[0-9]{2}:[0-9]{2},[0-9]{3}) --> ([0-9]{2}:[0-9]{2}:[0-9]{2},[0-9]{3})";
		std::regex subRegex (szRegex);
		std::string fileData = getFileData();
		// default constructor = end-of-sequence:
		std::regex_token_iterator<std::string::iterator> rend;
		int submatches[] = {-1,1,2,3};
		std::regex_token_iterator<std::string::iterator> c ( fileData.begin(), fileData.end(), subRegex, submatches );
		std::deque<std::string> match;
		while (c!=rend)
		{
			match.push_back(*c++);
		}
		match.pop_front();
		if(match.size()%4)
		{
			std::cout<<"File is incorrect!";
			return;
		}
			
		for(int i=0;i != match.size();i+=4)
		{
			_subtitles.push_back(new SubtitleItem(match[i+1],match[i+2],match[i+3]));
		}
	}
	catch(std::regex_error& e) {
		// Syntax error in the regular expression
		std::cout<<e.what()<<std::endl;
	}
}

SubRipParser::SubRipParser(std::string fileName)
{
	_fileName = fileName;
	parse(fileName);
}

SubRipParser::~SubRipParser(void)
{
    for(int i=0;i != _subtitles.size();++i)
    {
        if(_subtitles[i])
            delete _subtitles[i];
    }
}
