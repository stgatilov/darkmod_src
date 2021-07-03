# C++ Subtitle Parser Library
LibSub-Parser - A simple C++ Subtitle Parser library.
___
#### Formats
* SubRip

#### Sample Usage
```cpp
SubtitleParserFactory *subParserFactory = new SubtitleParserFactory("filename.srt");
SubtitleParser *parser = subParserFactory->getParser();
```
