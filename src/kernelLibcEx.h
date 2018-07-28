#ifndef KERNELLIBCEX_H
#define KERNELLIBCEX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char*	ftoa( float f, int precision );
void __atof2( char** s, int* value, int* exp );
float __atof( char** s );
int __atoi( char** s, int max );

#define strcasestr stristr
char*	stristr( const char* s1, const char* s2 );

/* Functions declared in libc: */
/*
float	atof( const char* str );
#define strcasecmp stricmp
int		strcasecmp( const char* s1, const char* s2 );
#define strncasecmp strnicmp
int		strncasecmp( const char* s1, const char* s2, size_t n );
int		sscanf( const char* str, const char* fmt, ... );
char*	strupr(char* str);
char*	strlwr(char* str);
*/
#endif
