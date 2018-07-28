#ifndef __CONFIG_H__
#define __CONFIG_H__

typedef struct cfgFile
{
	int			fd;
	int			fpos;
	char		name[128];
} cfgFile;


cfgFile*	cfgOpen( const char* filename );
void		cfgClose( cfgFile* cfg );

char*	cfgGetString( cfgFile* cfg, const char* section, const char* element, const char* default_value );
float	cfgGetFloat( cfgFile* cfg, const char* section, const char* element, float default_value );
//int		cfgGetFixed( cfgFile* cfg, const char* section, const char* element, int default_value, int base );
int		cfgGetInt( cfgFile* cfg, const char* section, const char* element, int default_value );
float*	cfgGetFloatv( cfgFile* cfg, const char* section, const char* element, float default_value[], int* sz );
int*	cfgGetIntv( cfgFile* cfg, const char* section, const char* element, int default_value[], int* sz );

#ifndef CONFIG_NOSAVE
int		cfgSetString( cfgFile* cfg, const char* section, const char* element, char* value );
int		cfgSetFloat( cfgFile* cfg, const char* section, const char* element, float value );
int		cfgSetInt( cfgFile* cfg, const char* section, const char* element, int value );
#endif

#endif
