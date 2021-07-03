#pragma once

class SubRipParser :
	public SubtitleParser
{
	void parse(std::string fileName);
public:
	SubRipParser(void);
	SubRipParser(std::string fileName);
	~SubRipParser(void);
};

