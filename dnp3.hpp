
#ifndef DFP3_DEFINED

#include "log.hpp"
///#include "dfs_logger.hpp"
#include <time.h>

#define TAG_SIZE 20

// 
struct Dnp3Point{
		char pointType[3];	// type of this point  // needs init
        int pointIndex;		// index of this point
        float value;		// Value or error number
		char pointTag[50];	// Tag name (ht4 address) i.e."104B3"
        time_t timeTag;
};
#endif
