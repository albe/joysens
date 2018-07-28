#include <pspiofilemgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "debug.h"
#include "kernelLibcEx.h"

//#define debuglog printf

#define BUFFER_SIZE 512
#define LINE_SIZE 128
static char buffer[BUFFER_SIZE];
static int bufferSize = 0;
static int bufferPos = 0;

__attribute ((__noinline__)) static char* readLine( cfgFile* file )
{
	static char line[LINE_SIZE];
	int i = 0;
	int fpos = 0;
	do
	{
		if (bufferSize==0)
		{
			bufferPos = 0;
			fpos = 0;
			file->fpos = sceIoLseek32( file->fd, 0, SEEK_CUR );
			bufferSize = sceIoRead( file->fd, buffer, BUFFER_SIZE );
			if (bufferSize<=0)
				return 0;
		}
		while (bufferSize>0 && i<LINE_SIZE-1 && buffer[bufferPos]!='\n' && buffer[bufferPos]!='\r')
		{
			line[i++] = buffer[bufferPos++];
			bufferSize--;
			fpos++;
		}
	}
	while (i<LINE_SIZE-1 && buffer[bufferPos]!='\n' && buffer[bufferPos]!='\r');

	if (bufferSize>0 && buffer[bufferPos]=='\r')
	{
		bufferPos++;
		bufferSize--;
		fpos++;
	}
	if (bufferSize>0 && buffer[bufferPos]=='\n')
	{
		bufferPos++;
		bufferSize--;
		fpos++;
	}
	file->fpos += fpos;
	line[i] = 0;
	return line;
}

static void fileSeek( cfgFile* file, int offs )
{
	debuglog("fileSeek: offs = %i, bufferPos = %i, bufferSize = %i\n", offs, bufferPos, bufferSize);
	if ((bufferPos + bufferSize > 0) && (file->fpos - bufferPos <= offs) && (file->fpos + bufferSize > offs))
	{
		bufferPos = offs - (file->fpos - bufferPos);
		bufferSize = (file->fpos + bufferSize) - offs;
		file->fpos = offs;
		sceIoLseek32( file->fd, offs+bufferSize, SEEK_SET );
		debuglog("        : buffer rewind! bufferPos = %i, bufferSize = %i\n", bufferPos, bufferSize);
		return;
	}
	file->fpos = sceIoLseek32( file->fd, offs, SEEK_SET );
	bufferPos = 0;
	bufferSize = 0;
}

#define NUM_CACHED_SECTIONS 2
static char last_section[NUM_CACHED_SECTIONS][64] = { { 0 } };
static int  last_section_offs[NUM_CACHED_SECTIONS] = { 0 };
static int	last_section_idx = 0;

static int cfgElementFind( cfgFile* cfg, const char* name )
{
	if (cfg == 0) return 0;
	
	debuglog("cfgElementFind: name = '%s'\n", name);
	int i = 0;
	for (i=0;i<NUM_CACHED_SECTIONS;i++)
	{
		debuglog("              : last_section = '%s', last_section_offs = %i\n", last_section[i], last_section_offs[i]);
		if (stricmp( name, last_section[i] )==0)
		{
			debuglog("              : cached!'%s' [0x%X]\n", last_section[i], last_section_offs[i]);
			fileSeek( cfg, last_section_offs[i] );
			char* line = readLine( cfg );
			debuglog("              : line = '%s'\n", line);
			return 1;
		}
	}
	
	fileSeek( cfg, 0 );
	int section_offs = cfg->fpos;
	char* line;
	while ((line=readLine( cfg ))!=0)
	{
		char section[64];
		if (sscanf( line, "\\s[%s]", section )==1)
		{
			if (stricmp( section, name )==0)
			{
				last_section_idx = (last_section_idx+1)%NUM_CACHED_SECTIONS;
				strcpy( last_section[last_section_idx], section );
				last_section_offs[last_section_idx] = section_offs;
				debuglog("              : '%s' [0x%X]\n", last_section[last_section_idx], last_section_offs[last_section_idx]);
				return 1;
			}
		}
		
		section_offs = cfg->fpos;
	}
	
	return 0;
}

cfgFile* cfgOpen( const char* filename )
{
	static cfgFile file;
	memset(&file,0,sizeof(file));
	
	file.fd = sceIoOpen( filename, PSP_O_RDWR | PSP_O_CREAT, 0777 );
	if (file.fd<0)
	{
		debuglog("Error opening config file '%s' (0x%08X).\n", filename, file.fd);
		return 0;
	}
	sceIoClose( file.fd );
	strcpy(file.name, filename);

	return &file;
}

void	cfgClose( cfgFile* cfg )
{
	if (cfg==0)
		return;
	
	sceIoClose( cfg->fd );
	cfg->fd = 0;
}

__attribute ((__noinline__)) char*	cfgGetString( cfgFile* cfg, const char* section, const char* element, const char* default_value )
{
	if (!cfg)
		return default_value;
	
	if (!section || !element || *section==0 || *element==0)
		return default_value;
	
	debuglog("cfgGetString: '%s', '%s', '%s'\n", section, element, default_value);
	
	cfg->fd = sceIoOpen( cfg->name, PSP_O_RDWR | PSP_O_CREAT, 0777 );
	if (!cfgElementFind( cfg, section ))
	{
		sceIoClose( cfg->fd );
		return default_value;
	}
	
	char* line;
	char name[64];
	static char value[128];
	int len = strlen(element)+1;
	while((line=readLine(cfg))!=0)
	{
		if (sscanf(line,"\\s[%s]", name)==1)
		{
			debuglog("            : end of section reached.\n");
			break;
		}
		
		line = strtok( line, ";#" );
		if (line && sscanf(line,"%s\\*=%s", name, value)==2)
		{
			debuglog("            : '%s' -> '%s'\n", name, value);
			if (strncasecmp( name, element, len )==0)
			{
				debuglog("            : match found.\n");
				sceIoClose( cfg->fd );
				return value;
			}
		}
	}
	debuglog("            : entry not found\n");
	sceIoClose( cfg->fd );
	return default_value;
}

const char* default_value_str = "0";

float	cfgGetFloat( cfgFile* cfg, const char* section, const char* element, float default_value )
{
	char* ret = cfgGetString( cfg, section, element, default_value_str );
	debuglog("cfgGetFloat: '%s' (%p)\n", ret, ret);
	if (ret==default_value_str)
		return default_value;
	else
		return __atof(&ret);
}

int		cfgGetInt( cfgFile* cfg, const char* section, const char* element, int default_value )
{
	char* ret = cfgGetString( cfg, section, element, default_value_str );
	if (ret==default_value_str)
		return default_value;
	else
		return atoi(ret);
}
/*
int		cfgGetFixed( cfgFile* cfg, const char* section, const char* element, int default_value, int base )
{
	char* ret = cfgGetString( cfg, section, element, default_value_str );
	if (ret==default_value_str)
		return default_value;
	
	int val, exp;
	__atof2( &ret, &val, &exp );
	if (exp==0)
	{
		return val*base;
	}
	int div = 10;
	int n = (exp>=0?exp:-exp);
	while (n--)
		div *= div;
	return (exp>=0?val*base*div:val*base/div);
}
*/

static	int	iretv[16];
/*
float*	cfgGetFloatv( cfgFile* cfg, const char* section, const char* element, float default_value[], int *sz )
{
	char* ret = cfgGetString( cfg, section, element, default_value_str );
	if (ret==default_value_str)
		return default_value;
	
	float* fretv = (float*)iretv;
	int i = 0;
	if (*ret=='<' || *ret=='(' || *ret=='[')
		ret++;
	while (*ret && *ret!='\n' && *ret!='>' && *ret!=')' && *ret!=']' && i++ < 16)
	{
		while (*ret==' ' || *ret=='\t')
			ret++;
		fretv[i-1] = __atof( &ret );
		int sep = 0;
		while ((*ret==' ' || *ret=='\t' || *ret==',' || *ret==';'))
		{
			if (*ret==',' || *ret==';')
			{
				if (sep) break; else sep = 1;
			}
			ret++;
		}
	}
	*sz = i;
	return fretv;
}
*/
int*	cfgGetIntv( cfgFile* cfg, const char* section, const char* element, int default_value[], int* sz )
{
	char* ret = cfgGetString( cfg, section, element, default_value_str );
	debuglog("cfgGetIntv: '%s' (%p)\n", ret, ret);
	if (ret==default_value_str)
		return default_value;
	
	int i = 0;
	if (*ret=='<' || *ret=='(' || *ret=='[')
		ret++;
	while (*ret && *ret!='\n' && *ret!='>' && *ret!=')' && *ret!=']' && i++ < 16)
	{
		while (*ret==' ' || *ret=='\t')
			ret++;
		iretv[i-1] = __atoi( &ret, 0 );
		debuglog("         : retv[%i] = '%i'\n", i-1, iretv[i-1]);
		int sep = 0;
		while (*ret==' ' || *ret=='\t' || *ret==',' || *ret==';')
		{
			if (*ret==',' || *ret==';')
			{
				if (sep) break; else sep = 1;
			}
			ret++;
		}
	}
	if (sz)
		*sz = i;
	debuglog("         : sz = %i\n", i);
	return iretv;
}

#ifndef CONFIG_NOSAVE
__attribute ((__noinline__)) int		cfgSetString( cfgFile* cfg, const char* section, const char* element, char* value )
{
	if (!cfg)
		return -1;
	
	if (!section || !element || *section==0 || *element==0)
		return -1;

	cfg->fd = sceIoOpen( cfg->name, PSP_O_RDWR | PSP_O_CREAT, 0777 );
	debuglog("cfgSetString: '%s' '%s' '%s'\n", section, element, value);

	if (!cfgElementFind( cfg, section ))
	{
		debuglog("            : section not found, appending to file.\n");
		if (strchr(value,' ')!=0)
		{
			sprintf(buffer, "\n[%s]\n%s = \"%s\"\n", section, element, value);
		}
		else
		{
			sprintf(buffer, "\n[%s]\n%s = %s\n", section, element, value);
		}
		int fpos = sceIoLseek32( cfg->fd, 0, SEEK_END );
		debuglog("            : writing at (%i) %i bytes\n", fpos, strlen(buffer));
		sceIoWrite( cfg->fd, buffer, strlen(buffer) );
		sceIoLseek32( cfg->fd, cfg->fpos+bufferSize, SEEK_SET );
		
		//Flush readbuffer!
		bufferSize = 0;
		bufferPos = 0;
		sceIoClose( cfg->fd );
		return 0;
	}

	char* line;
	int fpos = cfg->fpos;
	int skipline = 0;
	int lastfpos = fpos;
	while((line=readLine(cfg))!=0)
	{
		char name[64];
		if (sscanf(line,"\\s%c",name)==0)
		{
			fpos = cfg->fpos;
			continue;
		}
		if (sscanf(line,"\\s[%s]", name)==1)
		{
			break;
		}
		
		line = strtok( line, ";#" );
		char lvalue[128];
		if (sscanf(line,"%s\\*=%s", name, lvalue)==2)
		{
			if (stricmp( name, element )==0)
			{
				if (strcmp( value, lvalue )==0)
				{
					debuglog("            : element already set. Nothing done.\n");
					return 0;
				}
				
				skipline = 1;
				debuglog("            : found element (%i - %i)\n", fpos, cfg->fpos);
				break;
			}
		}
		
		fpos = cfg->fpos;
		lastfpos = fpos;
	}

	if (strchr(value,' ')!=0)
	{
		sprintf(buffer, "%s = \"%s\"\n", element, value);
	}
	else
	{
		sprintf(buffer, "%s = %s\n", element, value);
	}
	int toread;
	if (skipline==1)
	{
		toread = sceIoLseek32( cfg->fd, 0, SEEK_END ) - sceIoLseek32( cfg->fd, cfg->fpos, SEEK_SET );
	}
	else
	{
		if (fpos>lastfpos) fpos = lastfpos;
		toread = sceIoLseek32( cfg->fd, 0, SEEK_END ) - sceIoLseek32( cfg->fd, fpos, SEEK_SET );
	}

	char readbuffer[BUFFER_SIZE];
	int read;
	int readpos;
	int blen = strlen(buffer);
	while (toread>0 && (read = sceIoRead( cfg->fd, readbuffer, BUFFER_SIZE ))>0)
	{
		readpos = sceIoLseek32( cfg->fd, 0, SEEK_CUR );
		debuglog("            : toread (%i) - read(%i) - rpos(%i)\n", toread, read, readpos);
		if (read > toread) read = toread;
		toread -= read;
		//debuglog("            : readbuffer = \n%s\n", readbuffer);
		sceIoLseek32( cfg->fd, fpos, SEEK_SET );
		int write = BUFFER_SIZE-blen;
		if (write>read) write = read;
		debuglog("            : towrite (%i) - write(%i) - wpos(%i)\n", blen+write, write, fpos);
		memcpy( buffer+blen, readbuffer, write );
		//debuglog("            : buffer = \n%s\n", buffer);
		sceIoWrite( cfg->fd, buffer, blen+write );
		memcpy( buffer, readbuffer + write, read-write );
		blen = read-write;
		fpos = sceIoLseek32( cfg->fd, 0, SEEK_CUR );
		sceIoLseek32( cfg->fd, readpos, SEEK_SET );
	}
	//debuglog("            : buffer = \n%s\n", buffer);
	sceIoWrite( cfg->fd, buffer, blen );

	//Flush readbuffer!
	bufferSize = 0;
	bufferPos = 0;
	sceIoClose( cfg->fd );
	return 0;
}
/*
int		cfgSetFloat( cfgFile* cfg, const char* section, const char* element, float value )
{
	return cfgSetString( cfg, section, element, ftoa(value,6) );
}
*/
int		cfgSetInt( cfgFile* cfg, const char* section, const char* element, int value )
{
	char svalue[64];
	sprintf(svalue, "%i", value);
	return cfgSetString( cfg, section, element, svalue );
}

#endif
