#include <stdio.h>
#ifdef RELEASE
//#define printf(...)   
//printf(" ")
#define debuglog(...)   
#else
extern void dbgfileprint( char* msg );
#define debuglog(...) { char temp[256]; sprintf(temp,__VA_ARGS__); dbgfileprint(temp); }
#endif
