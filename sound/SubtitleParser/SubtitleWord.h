#pragma once
class SubtitleWord
{
private:
	std::string _text;
public:
	SubtitleWord(void);
	SubtitleWord(std::string text);
	virtual std::string getText() const;
	~SubtitleWord(void);
};

