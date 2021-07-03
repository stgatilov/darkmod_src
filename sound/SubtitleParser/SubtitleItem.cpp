#include <iostream>
#include <algorithm>
#include <regex>
#include "SubtitleItem.h"

SubtitleItem::SubtitleItem(void)
{
}

SubtitleItem::SubtitleItem(std::string startTime,std::string endTime, std::string text)
{
	_startTime = timeMSec(startTime);
	_endTime = timeMSec(endTime);
	_text = text;
}

long int SubtitleItem::timeMSec(std::string value)
{
	std::string szRegex = "([0-9]+):([0-9]{2}):([0-9]{2}),([0-9]{3})";
	std::regex subRegex (szRegex);
	int submatches[] = {1,2,3,4};
	std::regex_token_iterator<std::string::iterator> c ( value.begin(), value.end(), subRegex, submatches );
	std::regex_token_iterator<std::string::iterator> rend;
	std::vector<std::string> parts;
	while (c!=rend)
	{
		parts.push_back(*c++);
	}
	return atoi(parts[0].c_str()) * 3600000 + atoi(parts[1].c_str()) * 60000 + atoi(parts[2].c_str()) * 1000 + atoi(parts[3].c_str());
}

long int SubtitleItem::getStartTime() const
{
	return _startTime;
}
long int SubtitleItem::getEndTime() const
{
	return _endTime;
}

std::string SubtitleItem::getText() const
{
	return _text;
}

void SubtitleItem::setStartTime(long int startTime)
{
	_startTime = startTime;
}
void SubtitleItem::setEndTime(long int endTime)
{
	_endTime = endTime;
}
void SubtitleItem::setText(std::string text)
{
	_text = text;
}

SubtitleItem::~SubtitleItem(void)
{

}

