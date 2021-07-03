#include <string>
#include "SubtitleWord.h"

SubtitleWord::SubtitleWord(void)
{
	_text = "";
}

SubtitleWord::SubtitleWord(std::string text)
{
	_text = text;
}

std::string SubtitleWord::getText() const
{
	return _text;
}

SubtitleWord::~SubtitleWord(void)
{
}
