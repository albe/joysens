#ifndef RELEASE

#include <pspiofilemgr.h>
#include <string.h>

void dbgfileprint( char* msg )
{
	int fd = sceIoOpen( "ms0:/SEPLUGINS/joysens.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
	if (fd<0) return;

	sceIoWrite( fd, msg, strlen(msg) );
	sceIoClose( fd );
}
#endif
