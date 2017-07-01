#pragma once

#ifdef _MSC_VER
	#ifdef MAKEDLL
		#define EXTLIB __declspec(dllexport)
	#else
		#define EXTLIB __declspec(dllimport)
	#endif
#else
	#define EXTLIB __attribute__((visibility("default")))
#endif
