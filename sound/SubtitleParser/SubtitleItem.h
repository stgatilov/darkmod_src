#pragma once
#include <vector>
#include "SubtitleWord.h"

class SubtitleItem
{
private:
	long int _startTime; //in milliseconds
	long int _endTime;
	std::string _text;
	long int timeMSec(std::string value);
public:
	long int getStartTime() const;
	long int getEndTime() const;
	std::string getText() const;

	void setStartTime(long int startTime);
	void setEndTime(long int endTime);
	void setText(std::string text);

	SubtitleItem(void);
	SubtitleItem(int startTime,int endTime,std::string text);
	SubtitleItem(std::string startTime,std::string endTime, std::string text);
	~SubtitleItem(void);
};

