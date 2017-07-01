#ifndef COMPRESSION_PARAMETERS_H
#define COMPRESSION_PARAMETERS_H

//stgatilov: Some types of files should not be compressed by pk4
//this header is used both in FileSystem and in tdm_update
static const char* PK4_UNCOMPRESSED_EXTENSIONS[] = {
	"ogg",	//4504
	"roq",	//4507
	"avi", "mp4", "m4v"	//4519
};
static const int PK4_UNCOMPRESSED_EXTENSIONS_COUNT = sizeof(PK4_UNCOMPRESSED_EXTENSIONS) / sizeof(PK4_UNCOMPRESSED_EXTENSIONS[0]);

#endif
