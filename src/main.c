#include <pspkernel.h>
#include <pspinit.h>
#include <pspsdk.h>
#include <psprtc.h>
#include <pspumd.h>
#include <pspdisplay.h>
#include <pspdisplay_kernel.h>
#include <pspctrl.h>
#include <pspctrl_kernel.h>
#include <psputilsforkernel.h>
#include <pspiofilemgr.h>
#include <psppower.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NOLIBM
#include <math.h>
#else
#include "minimath.h"
#endif

#include "hook.h"
#include "debug.h"
#include "kernelLibcEx.h"
#ifdef CONFIG
#include "config.h"
#endif


PSP_MODULE_INFO("JoySens", PSP_MODULE_KERNEL, 1, 5);
#ifdef JOYSENS_LITE
#define JOYSENSVERSION "1.5lite"
#else
#define JOYSENSVERSION "1.5"
#endif

int (*g_ctrl_common)(SceCtrlData *, int count, int type);
int (*g_setframebuf)(int unk, void* addr, int width, int psm, int sync);


#ifndef JOYSENS_LITE

#define DEBUGMSG_SIZE 256
static char debugmsg[DEBUGMSG_SIZE+DEBUGMSG_SIZE/4];
char* debugcur = debugmsg;

extern u8 msx[];

void dbgprint(const char* text, void* where, int psm)
{
	if (text==0 || *text==0 || where==0) return;
	int c, i, j, l;
	unsigned char *font;
	unsigned short *vram;
	void *baseptr = 0;
	int x = 2, y = 2;

	//sceKernelDcacheWritebackAll();
	baseptr = (void*)((unsigned int)where&~0x40000000);
	if (!((unsigned int)baseptr>=0x4000000 && (unsigned int)baseptr<0x4200000) &&
		!((unsigned int)baseptr>=0x8800000 && (unsigned int)baseptr<0xA000000)) return;

	int start_x = x, start_y = y;
	for (c = 0; c < DEBUGMSG_SIZE; c++) {
		char ch = text[c];
		if (ch==0) break;
		if (ch!='\n')
		{
			vram = (unsigned short*)baseptr + (x + y * 512);
			if (psm==3)
				vram += (x + y * 512);
			
			font = &msx[ (int)ch * 8 ];
			for (i = l = 0; i < 8; i++, l += 8, font++) {
				unsigned short* vram_ptr  = vram;
				for (j = 0; j < 8; j++) {
					if ((*font & (128 >> j)))
					{
						if (psm==3)
						{
							*((unsigned int*)vram_ptr) = 0xFFFFFFFF;
							*((unsigned int*)vram_ptr+1+512) = 0;
						}
						else
						{
							*vram_ptr = 0xFFFF;
							*(vram_ptr+1+512) = 0;
						}
					}
					vram_ptr++;
					if (psm==3) vram_ptr++;
				}
				vram += 512;
				if (psm==3)
					vram += 512;
			}
			x += 7;
		}
		else
		{
			x = start_x;
			y += 8;
			if (y>=272-8) y = start_y;
		}
		if (x>=480-7)
		{
			x = start_x;
			y += 8;
			if (y >= 272-8) y = start_y;
		}
	}
	
	sceKernelDcacheWritebackRange(baseptr,((unsigned int)vram-(unsigned int)baseptr));
}

#define DEBUG_RESET() { debugcur = debugmsg; memset(debugmsg,0,DEBUGMSG_SIZE); }
#define DEBUG_PRINTF( ... ) { if (debugcur<debugmsg+DEBUGMSG_SIZE) debugcur += sprintf( debugcur, __VA_ARGS__ ); }

#else // JOYSENS_LITE

#define DEBUG_RESET()
#define DEBUG_PRINTF( ... )

#endif // JOYSENS_LITE

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define CLAMP127(x) ((x) < -128 ? - 128 : (x) > 127 ? 127 : (x))


typedef struct
{
	short 	sensitivity;
	short	adjust;
	short	smooth;
	char 	enabled;
	char 	center_x, center_y;
	char	min_x, max_x;
	char	min_y, max_y;
	char	remap;
	char	forceanalog;
	char	remapmodes[7];
	char	num_remapmodes;
	char	threshold;
	char	idlestop, idleback;
} JoySettings;

static JoySettings g_settings = { 100, 30, 100, 1, 0, 0, -128,127,-128,127, 0, 1, { 0, 1, 2, 3, 4, 5, 6 }, 7, 112, 32, 32 };

#ifdef CONFIG
static cfgFile*	g_config = 0;
#endif

// BUTTONS
static int g_primarybtn = PSP_CTRL_NOTE;
static int g_mapbtn = PSP_CTRL_START;
#ifndef JOYSENS_LITE
static int g_enablebtn = PSP_CTRL_SELECT;
static int g_sensupbtn = PSP_CTRL_RTRIGGER;
static int g_sensdownbtn = PSP_CTRL_LTRIGGER;
static int g_infobtn = PSP_CTRL_TRIANGLE;
static int g_resetbtn = PSP_CTRL_CROSS;
static int g_centerbtn = PSP_CTRL_SQUARE;
static int g_adjustupbtn = PSP_CTRL_UP;
static int g_adjustdownbtn = PSP_CTRL_DOWN;
static int g_smoothupbtn = PSP_CTRL_RIGHT;
static int g_smoothdownbtn = PSP_CTRL_LEFT;
static int g_savebtn = PSP_CTRL_CIRCLE;
static int g_forcebtn = 0;
static int g_calibbtn = PSP_CTRL_SCREEN;
static int g_thresholdupbtn = 0;
static int g_thresholddownbtn = 0;
#endif

static char g_calibrate = 0;
static char g_info = 0;
static char g_quit = 0;
static char g_savesettings = 0;

static char* g_executable = 0;
static char g_stringbuffer[12];

#ifndef JOYSENS_LITE
static char* g_remapmodes[] = { "None", "DPad->analog", "Analog->DPad", "Analog<->DPad", "Buttons->analog", "Analog->buttons", "Analog<->buttons" };
#endif
static int g_remapidx = 0;

__attribute ((__noinline__)) int map_axis(int center, int min, int max, int real, int old)
{
	if (g_settings.adjust==0)
		return 128;
	
	int val1, val2;

	val1 = real - 128;

	// Rescale value into range -128 to 127
	if (val1 < 0 && min>-128)
	{
		if (min)
			val1 = CLAMP127(val1 * -128 / min);
	}
	else
	if (val1 > 0 && max < 127)
	{
		if (max)
			val1 = CLAMP127(val1 * 127 / max);
	}

	// Center value correctly
	if (center)
	{
		val1 -= center;
		if (val1<0 && center > -128)
			val1 = (val1 * 128) / (128 + center);
		else
		if (val1>0 && center < 127)
			val1 = (val1 * 127) / (127 - center);
	}

	// Apply adjust
	if (val1!=0 && g_settings.adjust!=10)
	{
		float val = val1<0?((float)(-val1) / 128.f):((float)val1 / 127.f);
		if (val < 0.9999f)
		{
			val1 = (int)((float)val1 * powf( val, (g_settings.adjust-10)/10.f ));
		}
	}
	
	val2 = old - 128;
	// Apply smooth
	// Interpolate linearly between val1 and val2
	int delta = (val1 - val2) * g_settings.smooth / 100;
	if ((val1 - val2)!=0 && delta==0)
		delta = (val1>val2?1:-1);	// Make sure delta is not zero if there is still a difference
	val1 = val2 + delta;
	
	// Apply sensitivity and convert back to unsigned char
	return ( 128 + CLAMP127( val1 * g_settings.sensitivity / 100 ) );
}


void adjust_values(SceCtrlData *pad_data, int count, int neg)
{
	int i;
	int intc;
	u64 this_tick;
	sceRtcGetCurrentTick( &this_tick );

	static SceCtrlData last_pad = { 0 };
	static SceCtrlData last_real = { 0 };

	intc = pspSdkDisableInterrupts();
	for(i = 0; i < count; i++)
	{
		int buttons = pad_data[i].Buttons;
		if (neg) buttons = ~buttons;
		int oldbuttons = buttons;
		
		if ((buttons&PSP_CTRL_HOLD)==PSP_CTRL_HOLD)
		{
			pad_data[i].Lx = 128;
			pad_data[i].Ly = 128;
			continue;
		}

		last_real = pad_data[i];
		#ifndef JOYSENS_LITE
		if (g_calibrate)
		{
			if (pad_data[i].Lx-128 < g_settings.min_x) g_settings.min_x = pad_data[i].Lx-128;
			if (pad_data[i].Lx-128 > g_settings.max_x) g_settings.max_x = pad_data[i].Lx-128;
			if (pad_data[i].Ly-128 < g_settings.min_y) g_settings.min_y = pad_data[i].Ly-128;
			if (pad_data[i].Ly-128 > g_settings.max_y) g_settings.max_y = pad_data[i].Ly-128;
		}
		#endif


		if (g_settings.enabled)
		{
			pad_data[i].Lx = map_axis(g_settings.center_x, g_settings.min_x, g_settings.max_x, pad_data[i].Lx,last_pad.Lx);
			pad_data[i].Ly = map_axis(g_settings.center_y, g_settings.min_y, g_settings.max_y, pad_data[i].Ly,last_pad.Ly);
		}
		last_pad.Lx = pad_data[i].Lx;
		last_pad.Ly = pad_data[i].Ly;


		#ifndef JOYSENS_LITE
		if (g_primarybtn && (buttons&g_primarybtn)==g_primarybtn)
		{
			static int minx = 127, miny = 127;
			static int maxx = -128, maxy = -128;
			if (g_calibbtn && (buttons&g_calibbtn)==g_calibbtn)
			{
				static u64 last_tick = 0;
				if (this_tick - last_tick > 1000000)
				{
					g_calibrate ^= 1;
					if (g_calibrate)
					{
						g_settings.min_x = 0;
						g_settings.max_x = 0;
						g_settings.min_y = 0;
						g_settings.max_y = 0;
					}
					last_tick = this_tick;
				}
				buttons &= ~g_calibbtn;
			}
			else
			if (g_forcebtn && (buttons&g_forcebtn)==g_forcebtn)
			{
				static u64 last_tick = 0;
				if (this_tick - last_tick > 1000000)
				{
					g_settings.forceanalog ^= 1;
					last_tick = this_tick;
				}
				buttons &= ~g_forcebtn;
			}
			else
			if (g_mapbtn && (buttons&g_mapbtn)==g_mapbtn)
			{
				static u64 last_tick = 0;
				if (this_tick - last_tick > 1000000)
				{
					g_remapidx = (g_remapidx+1)%g_settings.num_remapmodes;
					g_settings.remap = g_settings.remapmodes[g_remapidx];
					last_tick = this_tick;
				}
				buttons &= ~g_mapbtn;
			}
			else
			if (g_enablebtn && (buttons&g_enablebtn)==g_enablebtn)
			{
				static u64 last_tick = 0;
				if (this_tick - last_tick > 1000000)
				{
					g_settings.enabled ^= 1;
					last_tick = this_tick;
				}
				buttons &= ~g_enablebtn;
			}
			else
			if (g_sensdownbtn && (buttons&g_sensdownbtn)==g_sensdownbtn)
			{
				static u64 last_tick = 0;
				if (this_tick - last_tick > 250000)
				{
					g_settings.sensitivity -= (g_settings.sensitivity<=70?5:(g_settings.sensitivity>200?20:10));
					if (g_settings.sensitivity<10) g_settings.sensitivity = 10;
					last_tick = this_tick;
				}
				buttons &= ~g_sensdownbtn;
			}
			else
			if (g_sensupbtn && (buttons&g_sensupbtn)==g_sensupbtn)
			{
				static u64 last_tick = 0;
				if (this_tick - last_tick > 250000)
				{
					g_settings.sensitivity += (g_settings.sensitivity<70?5:(g_settings.sensitivity>=200?20:10));
					if (g_settings.sensitivity>400) g_settings.sensitivity = 400;
					last_tick = this_tick;
				}
				buttons &= ~g_sensupbtn;
			}
			else
			if (g_smoothdownbtn && (buttons&g_smoothdownbtn)==g_smoothdownbtn)
			{
				static u64 last_tick = 0;
				if (this_tick - last_tick > 250000)
				{
					g_settings.smooth -= (g_settings.smooth<=70?5:(g_settings.smooth>200?20:10));
					if (g_settings.smooth<10) g_settings.smooth = 10;
					last_tick = this_tick;
				}
				buttons &= ~g_smoothdownbtn;
			}
			else
			if (g_smoothupbtn && (buttons&g_smoothupbtn)==g_smoothupbtn)
			{
				static u64 last_tick = 0;
				if (this_tick - last_tick > 250000)
				{
					g_settings.smooth += (g_settings.smooth<70?5:(g_settings.smooth>=200?20:10));
					if (g_settings.smooth>400) g_settings.smooth = 400;
					last_tick = this_tick;
				}
				buttons &= ~g_smoothupbtn;
			}
			else
			if (g_adjustdownbtn && (buttons&g_adjustdownbtn)==g_adjustdownbtn)
			{
				static u64 last_tick = 0;
				if (this_tick - last_tick > 250000)
				{
					g_settings.adjust -= (g_settings.adjust<=100?1:5);
					if (g_settings.adjust<0) g_settings.adjust = 0;
					last_tick = this_tick;
				}
				buttons &= ~g_adjustdownbtn;
			}
			else
			if (g_adjustupbtn && (buttons&g_adjustupbtn)==g_adjustupbtn)
			{
				static u64 last_tick = 0;
				if (this_tick - last_tick > 250000)
				{
					g_settings.adjust += (g_settings.adjust<100?1:5);
					if (g_settings.adjust>320) g_settings.adjust = 320;
					last_tick = this_tick;
				}
				buttons &= ~g_adjustupbtn;
			}
			else
			if (g_centerbtn && (buttons&g_centerbtn)==g_centerbtn)
			{
				static u64 last_tick = 0;
				if (this_tick - last_tick > 25000)
				{
					if (miny>(last_real.Ly-128)) miny = (last_real.Ly-128);
					if (maxy<(last_real.Ly-128)) maxy = (last_real.Ly-128);
					
					if ((last_real.Ly-128)<0)
					{
						miny = (miny*15 + (last_real.Ly-128)) / 16;
					}
					else
					{
						maxy = (maxy*15 + (last_real.Ly-128)) / 16;
					}
					/*miny = (miny*15 + (last_real.Ly-128)) / 16;
					maxy = (maxy*15 + (last_real.Ly-128)) / 16;*/
					g_settings.center_y = (maxy+miny)/2;
					
					if (minx>(last_real.Lx-128)) minx = (last_real.Lx-128);
					if (maxx<(last_real.Lx-128)) maxx = (last_real.Lx-128);				
					
					if ((last_real.Lx-128)<0)
					{
						minx = (minx*15 + (last_real.Lx-128)) / 16;
					}
					else
					{
						maxx = (maxx*15 + (last_real.Lx-128)) / 16;
					}
					/*minx = (minx*15 + (last_real.Lx-128)) / 16;
					maxx = (maxx*15 + (last_real.Lx-128)) / 16;
					*/
					g_settings.center_x = (maxx+minx)/2;
					last_tick = this_tick;
				}
				buttons &= ~g_centerbtn;
			}
			else
			if (g_resetbtn && (buttons&g_resetbtn)==g_resetbtn)
			{
				g_settings.center_x = g_settings.center_y = 0;
				g_settings.sensitivity = 100;
				g_settings.adjust = 30;
				g_settings.smooth = 100;
				g_settings.remap = 0;
				minx = miny = 127;
				maxx = maxy = -128;
				g_settings.min_x = -128;
				g_settings.max_x = 127;
				g_settings.min_y = -128;
				g_settings.max_y = 127;
				buttons &= ~g_resetbtn;
			}
			else
			if (g_infobtn && (buttons&g_infobtn)==g_infobtn)
			{
				static u64 last_tick = 0;
				if (this_tick - last_tick > 250000)
				{
					g_info ^= 1;
					last_tick = this_tick;
				}
				buttons &= ~g_infobtn;
			}
			else
			if (g_savebtn && (buttons&g_savebtn)==g_savebtn)
			{
				static u64 last_tick = 0;
				if (this_tick - last_tick > 5000000)
				{
					g_savesettings = 1;
				}
				buttons &= ~g_savebtn;
			}
			else
			if (g_thresholdupbtn && (buttons&g_thresholdupbtn)==g_thresholdupbtn)
			{
				static u64 last_tick = 0;
				if (this_tick - last_tick > 250000)
				{
					if (g_settings.threshold < 126) g_settings.threshold++;
				}
			}
			else
			if (g_thresholddownbtn && (buttons&g_thresholddownbtn)==g_thresholddownbtn)
			{
				static u64 last_tick = 0;
				if (this_tick - last_tick > 250000)
				{
					if (g_settings.threshold > 0) g_settings.threshold--;
				}
			}
		}	// primarybtn
		#else // JOYSENS_LITE
		if (g_primarybtn && (buttons&g_primarybtn)==g_primarybtn)
		{
			if (g_mapbtn && (buttons&g_mapbtn)==g_mapbtn)
			{
				static u64 last_tick = 0;
				if (this_tick - last_tick > 1000000)
				{
					g_remapidx = (g_remapidx+1)%g_settings.num_remapmodes;
					g_settings.remap = g_settings.remapmodes[g_remapidx];
					last_tick = this_tick;
				}
				buttons &= ~g_mapbtn;
			}
		}
		#endif // JOYSENS_LITE
		
		// Remap DPad->Analog
		if ((g_settings.remap==3 || g_settings.remap==1) && (buttons&g_primarybtn)==0)
		{
			if (g_settings.remap==3)
			{
				pad_data[i].Lx = 128;
				pad_data[i].Ly = 128;
			}
			if (buttons&PSP_CTRL_LEFT)
			{
				pad_data[i].Lx = -128 + 128;
				buttons &= ~PSP_CTRL_LEFT;
			}
			if (buttons&PSP_CTRL_RIGHT)
			{
				pad_data[i].Lx = 127 + 128;
				buttons &= ~PSP_CTRL_RIGHT;
			}
			if (buttons&PSP_CTRL_UP)
			{
				pad_data[i].Ly = -128 + 128;
				buttons &= ~PSP_CTRL_UP;
			}
			if (buttons&PSP_CTRL_DOWN)
			{
				pad_data[i].Ly = 127 + 128;
				buttons &= ~PSP_CTRL_DOWN;
			}
		}
		
		// Remap Analog->DPad
		if (g_settings.remap==3 || g_settings.remap==2)
		{
			if (g_settings.remap==3)
			{
				buttons &= ~(PSP_CTRL_LEFT|PSP_CTRL_RIGHT|PSP_CTRL_UP|PSP_CTRL_DOWN);
			}
			else
			{
				pad_data[i].Lx = 128;
				pad_data[i].Ly = 128;
			}
			
			if (last_pad.Lx < -g_settings.threshold + 128)
			{
				buttons |= PSP_CTRL_LEFT;
			}
			else
			if (last_pad.Lx > g_settings.threshold + 128)
			{
				buttons |= PSP_CTRL_RIGHT;
			}
			if (last_pad.Ly < -g_settings.threshold + 128)
			{
				buttons |= PSP_CTRL_UP;
			}
			else
			if (last_pad.Ly > g_settings.threshold + 128)
			{
				buttons |= PSP_CTRL_DOWN;
			}
		}
		

		// Remap Buttons->Analog
		if ((g_settings.remap==6 || g_settings.remap==4) && (buttons&g_primarybtn)==0)
		{
			if (g_settings.remap==6)
			{
				pad_data[i].Lx = 128;
				pad_data[i].Ly = 128;
			}
			if (buttons&PSP_CTRL_SQUARE)
			{
				pad_data[i].Lx = -128 + 128;
				buttons &= ~PSP_CTRL_SQUARE;
			}
			if (buttons&PSP_CTRL_CIRCLE)
			{
				pad_data[i].Lx = 127 + 128;
				buttons &= ~PSP_CTRL_CIRCLE;
			}
			if (buttons&PSP_CTRL_TRIANGLE)
			{
				pad_data[i].Ly = -128 + 128;
				buttons &= ~PSP_CTRL_TRIANGLE;
			}
			if (buttons&PSP_CTRL_CROSS)
			{
				pad_data[i].Ly = 127 + 128;
				buttons &= ~PSP_CTRL_CROSS;
			}
		}
		
		// Remap Analog->Buttons
		if (g_settings.remap==6 || g_settings.remap==5)
		{
			if (g_settings.remap==6)
			{
				buttons &= ~(PSP_CTRL_SQUARE|PSP_CTRL_CIRCLE|PSP_CTRL_TRIANGLE|PSP_CTRL_CROSS);
			}
			else
			{
				pad_data[i].Lx = 128;
				pad_data[i].Ly = 128;
			}
			
			if (last_pad.Lx < -g_settings.threshold + 128)
			{
				buttons |= PSP_CTRL_SQUARE;
			}
			else
			if (last_pad.Lx > g_settings.threshold + 128)
			{
				buttons |= PSP_CTRL_CIRCLE;
			}
			if (last_pad.Ly < -g_settings.threshold + 128)
			{
				buttons |= PSP_CTRL_TRIANGLE;
			}
			else
			if (last_pad.Ly > g_settings.threshold + 128)
			{
				buttons |= PSP_CTRL_CROSS;
			}
		}
		
		#ifndef JOYSENS_LITE
		if (g_info || (g_primarybtn && (buttons&g_primarybtn)==g_primarybtn))
		{
			DEBUG_RESET()
			#ifdef DEBUG
			DEBUG_PRINTF( "Adjusted analog axes: <%+04i,%+04i> -> <%+04i,%+04i>\n", last_real.Lx-128,last_real.Ly-128, pad_data[i].Lx-128, pad_data[i].Ly-128 )
			#else
			DEBUG_PRINTF( "Adjusted analog axes: <%i,%i> -> <%i,%i>\n", last_real.Lx-128,last_real.Ly-128, pad_data[i].Lx-128, pad_data[i].Ly-128 )
			#endif
			DEBUG_PRINTF( "JoySens: version %s - %s (info %s)\n", JOYSENSVERSION, g_settings.enabled?"on":"off",g_info?"pinned":"unpinned" )
			DEBUG_PRINTF( "Sensitivity: %i%%\n", g_settings.sensitivity )
			DEBUG_PRINTF( "Smooth: %i%%\n", g_settings.smooth )
			DEBUG_PRINTF( "Adjust: %i.%i\n", (g_settings.adjust/10), (g_settings.adjust-(g_settings.adjust/10*10)) )
			DEBUG_PRINTF( "Center: <%i,%i> (%i,%i)->(%i,%i)\n", g_settings.center_x, g_settings.center_y, g_settings.min_x, g_settings.min_y, g_settings.max_x, g_settings.max_y )
			DEBUG_PRINTF( "Remap: %s\n", g_remapmodes[g_settings.remap] )
		}
		else
		{
			DEBUG_RESET()
		}
		#endif // JOYSENS_LITE
		
		pad_data[i].Buttons = (neg?~buttons:buttons);
	}
	
	pspSdkEnableInterrupts(intc);
	/*
	if (last_pad.Lx < -32 + 128 || last_pad.Lx > 32 + 128 || last_pad.Ly < -32 + 128 || last_pad.Ly > 32 + 128)
		scePowerTick(0);
	*/
}



int ctrl_hook_func(SceCtrlData *pad_data, int count, int type)
{
	int ret;

	if (g_settings.forceanalog)
		sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	ret = g_ctrl_common(pad_data, count, type);
	if(ret <= 0)
	{
		return ret;
	}

	adjust_values(pad_data, ret, type&1);

	return ret;
}

#ifndef JOYSENS_LITE
int setframebuf_hook_func(int unk, void* addr, int width, int psm, int sync)
{
	//debuglog("Hooked sceDisplaySetFrameBuf( %i, %p, %i, %i )\n", unk, addr, width, psm);
	//sprintf(debugcur,"Hooked sceDisplaySetFrameBuf( %i, %p, %i, %i, %i )\n", unk, addr, width, psm, sync);
	dbgprint( debugmsg, addr, psm );
	if (!g_info) DEBUG_RESET()
	
	return g_setframebuf(unk, addr, width, psm, sync);
}
#endif // JOYSENS_LITE


#ifdef CONFIG
__attribute ((__noinline__)) int map_button( char* str )
{
	if (!str || *str==0) return 0;

	if (strnicmp(str,"START",5)==0)
		return PSP_CTRL_START;
	if (strnicmp(str,"SELECT",6)==0)
		return PSP_CTRL_SELECT;
	
	if (strnicmp(str,"RTRIGGER",8)==0)
		return PSP_CTRL_RTRIGGER;
	if (strnicmp(str,"LTRIGGER",8)==0)
		return PSP_CTRL_LTRIGGER;
	
	if (strnicmp(str,"CROSS",5)==0)
		return PSP_CTRL_CROSS;
	if (strnicmp(str,"SQUARE",6)==0)
		return PSP_CTRL_SQUARE;
	if (strnicmp(str,"CIRCLE",6)==0)
		return PSP_CTRL_CIRCLE;
	if (strnicmp(str,"TRIANGLE",8)==0)
		return PSP_CTRL_TRIANGLE;
	
	if (strnicmp(str,"LEFT",4)==0)
		return PSP_CTRL_LEFT;
	if (strnicmp(str,"RIGHT",5)==0)
		return PSP_CTRL_RIGHT;
	if (strnicmp(str,"UP",2)==0)
		return PSP_CTRL_UP;
	if (strnicmp(str,"DOWN",4)==0)
		return PSP_CTRL_DOWN;
	
	if (strnicmp(str,"SCREEN",6)==0)
		return PSP_CTRL_SCREEN;
	
	if (strnicmp(str,"VOLUP",5)==0)
		return PSP_CTRL_VOLUP;
	if (strnicmp(str,"VOLDOWN",7)==0)
		return PSP_CTRL_VOLDOWN;
	
	if (strnicmp(str,"NOTE",4)==0)
		return PSP_CTRL_NOTE;

	return 0;
}


__attribute ((__noinline__)) char* button( int btn )
{
	switch(btn)
	{
		case PSP_CTRL_START:	return "START";
		case PSP_CTRL_SELECT:	return "SELECT";
		case PSP_CTRL_LTRIGGER:	return "LTRIGGER";
		case PSP_CTRL_RTRIGGER:	return "RTRIGGER";
		case PSP_CTRL_LEFT:		return "LEFT";
		case PSP_CTRL_RIGHT:	return "RIGHT";
		case PSP_CTRL_UP:		return "UP";
		case PSP_CTRL_DOWN:		return "DOWN";
		case PSP_CTRL_TRIANGLE:	return "TRIANGLE";
		case PSP_CTRL_CROSS:	return "CROSS";
		case PSP_CTRL_SQUARE:	return "SQUARE";
		case PSP_CTRL_CIRCLE:	return "CIRCLE";
		case PSP_CTRL_SCREEN:	return "SCREEN";
		case PSP_CTRL_VOLUP:	return "VOLUP";
		case PSP_CTRL_VOLDOWN:	return "VOLDOWN";
		case PSP_CTRL_NOTE:		return "NOTE";
		default:				return "";
	}
}
#ifndef JOYSENS_LITE
#endif // JOYSENS_LITE

int load_config()
{
	g_config = cfgOpen( "ms0:/SEPLUGINS/joysens.ini" );
	if (g_config==0)
	{
		return 0;
	}

	g_settings.enabled		= cfgGetInt( g_config, g_executable, "enable",		cfgGetInt( g_config, "SETTINGS", "enable", g_settings.enabled ) ) & 1;
	g_settings.remap		= cfgGetInt( g_config, g_executable, "remap",		cfgGetInt( g_config, "SETTINGS", "remap", g_settings.remap ) ) % 7;
	g_settings.forceanalog	= cfgGetInt( g_config, g_executable, "forceanalog",	cfgGetInt( g_config, "SETTINGS", "forceanalog", g_settings.forceanalog ) ) & 1;
	g_settings.sensitivity	= cfgGetInt( g_config, g_executable, "sensitivity",	cfgGetInt( g_config, "SETTINGS", "sensitivity", g_settings.sensitivity ) );
	g_settings.smooth		= cfgGetInt( g_config, g_executable, "smooth",		cfgGetInt( g_config, "SETTINGS", "smooth", g_settings.smooth ) );
//	g_settings.adjust		= cfgGetFixed( g_config, g_executable, "adjust",	cfgGetFixed( g_config, "SETTINGS", "adjust", g_settings.adjust, 10 ), 10 );
	float adjust			= cfgGetFloat( g_config, g_executable, "adjust",	cfgGetFloat( g_config, "SETTINGS", "adjust", g_settings.adjust / 10.0f ) );
	g_settings.adjust		= (adjust<=0?0:(adjust>=32.0?320:(int)((adjust+0.05f)*10.0f)));


	g_settings.threshold	= cfgGetInt( g_config, "SETTINGS", "threshold", g_settings.threshold );
	g_settings.idlestop		= cfgGetInt( g_config, "SETTINGS", "idlestop", g_settings.idlestop );
	g_settings.idleback		= cfgGetInt( g_config, "SETTINGS", "idleback", g_settings.idleback );
	
	g_remapidx = g_settings.remap;
	int count;
	int *c					= cfgGetIntv( g_config, "SETTINGS", "center", 0, &count );
	if (c && count>=2)
	{
		g_settings.center_x		= (c[0]<-64?-64:(c[0]>64?64:c[0]));
		g_settings.center_y		= (c[1]<-64?-64:(c[1]>64?64:c[1]));
	}
	int *min				= cfgGetIntv( g_config, "SETTINGS", "min", 0, &count );
	if (min && count>=2)
	{
		g_settings.min_x		= (min[0]<-128?-128:(min[0]>127?127:min[0]));
		g_settings.min_y		= (min[1]<-128?-128:(min[1]>127?127:min[1]));
	}
	int *max				= cfgGetIntv( g_config, "SETTINGS", "max", 0, &count );
	if (max && count>=2)
	{
		g_settings.max_x		= (max[0]<-128?-128:(max[0]>127?127:max[0]));
		g_settings.max_y		= (max[1]<-128?-128:(max[1]>127?127:max[1]));
	}
	count = g_settings.num_remapmodes;
	int *modes				= cfgGetIntv( g_config, g_executable, "remapmodes",		cfgGetIntv( g_config, "SETTINGS", "remapmodes", g_settings.remapmodes, &count ), &count );
	if (modes && count>0)
	{
		int i = 0;
		g_settings.num_remapmodes = count;
		while (i < g_settings.num_remapmodes)
		{
			g_settings.remapmodes[i]	=  modes[i];
			if (g_settings.remap == modes[i])
				g_remapidx = i;
			i++;
		}
		while (i < 7)
		{
			g_settings.remapmodes[i++] = 0;
		}
	}
	else
	{
		g_settings.num_remapmodes = 1;
		int i = 0;
		while (i < 7)
		{
			g_settings.remapmodes[i++] = 0;
		}
	}
	debuglog("g_enabled = %i\ng_sensitivity = %i\ng_center = <%i,%i>\n", g_settings.enabled, g_settings.sensitivity, g_settings.center_x, g_settings.center_y);
	debuglog("g_adjust = %i.%i\ng_smooth = %i\ng_remap = %i\n", (g_settings.adjust/10),(g_settings.adjust-(g_settings.adjust/10*10)), g_settings.smooth, g_settings.remap);
	debuglog("g_forceanalog = %i\n", g_settings.forceanalog );
	
	
	g_primarybtn			= map_button( cfgGetString( g_config, "BUTTONS", "primarybtn", button( g_primarybtn ) ) );
	g_mapbtn				= map_button( cfgGetString( g_config, "BUTTONS", "mapbtn", button( g_mapbtn ) ) );
	
	#ifndef JOYSENS_LITE
	g_enablebtn				= map_button( cfgGetString( g_config, "BUTTONS", "enablebtn", button( g_enablebtn ) ) );
	g_sensupbtn				= map_button( cfgGetString( g_config, "BUTTONS", "sensupbtn", button( g_sensupbtn ) ) );
	g_sensdownbtn			= map_button( cfgGetString( g_config, "BUTTONS", "sensdownbtn", button( g_sensdownbtn ) ) );
	g_infobtn				= map_button( cfgGetString( g_config, "BUTTONS", "infobtn", button( g_infobtn ) ) );
	g_resetbtn				= map_button( cfgGetString( g_config, "BUTTONS", "resetbtn", button( g_resetbtn ) ) );
	g_centerbtn				= map_button( cfgGetString( g_config, "BUTTONS", "centerbtn", button( g_centerbtn ) ) );
	g_calibbtn				= map_button( cfgGetString( g_config, "BUTTONS", "calibbtn", button( g_calibbtn ) ) );
	g_adjustupbtn			= map_button( cfgGetString( g_config, "BUTTONS", "adjustupbtn", button( g_adjustupbtn ) ) );
	g_adjustdownbtn			= map_button( cfgGetString( g_config, "BUTTONS", "adjustdownbtn", button( g_adjustdownbtn ) ) );
	g_smoothupbtn			= map_button( cfgGetString( g_config, "BUTTONS", "smoothupbtn", button( g_smoothupbtn ) ) );
	g_smoothdownbtn			= map_button( cfgGetString( g_config, "BUTTONS", "smoothdownbtn", button( g_smoothdownbtn ) ) );
	g_forcebtn				= map_button( cfgGetString( g_config, "BUTTONS", "forcebtn", button( g_forcebtn ) ) );
	g_thresholdupbtn		= map_button( cfgGetString( g_config, "BUTTONS", "thresholdupbtn", button( g_thresholdupbtn ) ) );
	g_thresholddownbtn		= map_button( cfgGetString( g_config, "BUTTONS", "thresholddownbtn", button( g_thresholddownbtn ) ) );
	
	debuglog("g_enablebtn = %X\ng_infobtn = %X\ng_resetbtn = %X\n", g_enablebtn, g_infobtn, g_resetbtn);
	debuglog("g_sensupbtn = %X\ng_sensdownbtn = %X\n", g_sensupbtn, g_sensdownbtn );
	debuglog("g_adjustupbtn = %X\ng_adjustdownbtn = %X\n", g_adjustupbtn, g_adjustdownbtn );
	debuglog("g_smoothupbtn = %X\ng_smoothdownbtn = %X\n", g_smoothupbtn, g_smoothdownbtn );
	debuglog("g_centerbtn = %X\ng_savebtn = %X\ng_mapbtn = %X\ng_forcebtn = %X\n", g_centerbtn, g_savebtn, g_mapbtn, g_forcebtn );
	#endif
	return 1;
}

int save_config()
{
#ifndef CONFIG_NOSAVE
	debuglog("Saving configuration...\n");
	char buffer[64];
	
	cfgSetInt( g_config, g_executable, "forceanalog", g_settings.forceanalog );
	cfgSetInt( g_config, g_executable, "remap", g_settings.remap );
	cfgSetInt( g_config, g_executable, "smooth", g_settings.smooth );
	sprintf(buffer, "%i.%i", (int)g_settings.adjust / 10, (int)g_settings.adjust % 10 );
	cfgSetString( g_config, g_executable, "adjust", buffer );
	cfgSetInt( g_config, g_executable, "sensitivity", g_settings.sensitivity );
	cfgSetInt( g_config, g_executable, "enable", g_settings.enabled );
	
	int i = 1;
	char* bptr = buffer;
	bptr += sprintf(bptr, "%i", (int)g_settings.remapmodes[0]);
	for (;i<g_settings.num_remapmodes;i++)
	{
		bptr += sprintf(bptr, ",%i", (int)g_settings.remapmodes[i]);
	}
	cfgSetString( g_config, g_executable, "remapmodes", buffer );
	
	sprintf(buffer, "<%i,%i>", g_settings.center_x, g_settings.center_y );
	cfgSetString( g_config, "SETTINGS", "center", buffer );
	sprintf(buffer, "<%i,%i>", g_settings.min_x, g_settings.min_y );
	cfgSetString( g_config, "SETTINGS", "min", buffer );
	sprintf(buffer, "<%i,%i>", g_settings.max_x, g_settings.max_y );
	cfgSetString( g_config, "SETTINGS", "max", buffer );
	cfgSetInt( g_config, "SETTINGS", "idlestop", g_settings.idlestop );
	cfgSetInt( g_config, "SETTINGS", "idleback", g_settings.idleback );
	cfgSetInt( g_config, "SETTINGS", "threshold", g_settings.threshold );
#endif

	return 1;
}

#else
#define load_config()  
#define save_config()  
#endif

int main_thread(SceSize args, void *argp)
{
	sceKernelDelayThread(10000);
	//debuglog("\n");
	debuglog("Hooking ctrl functions...\n");
	int ret = 0;
	ret |= hook_function( (unsigned int*) sceCtrlReadBufferPositive, ctrl_hook_func, &g_ctrl_common );
	ret |= hook_function( (unsigned int*) sceCtrlPeekBufferPositive, ctrl_hook_func, &g_ctrl_common );
	ret |= hook_function( (unsigned int*) sceCtrlReadBufferNegative, ctrl_hook_func, &g_ctrl_common );
	ret |= hook_function( (unsigned int*) sceCtrlPeekBufferNegative, ctrl_hook_func, &g_ctrl_common );
	if (ret)
	{
		debuglog("Could not hook controller functions\n");
	}

	#ifndef JOYSENS_LITE
	debuglog("\nHooking sceDisplaySetFrameBuf...\n");
	ret = hook_function( (unsigned int*) sceDisplaySetFrameBuf, setframebuf_hook_func, &g_setframebuf );
	if (ret)
	{
		debuglog("Could not hook setframebuf function\n");
	}
	#endif // JOYSENS_LITE

	sceKernelDcacheWritebackInvalidateAll();
	sceKernelIcacheInvalidateAll();

	#ifdef CONFIG
	debuglog("\nLoading configuration...\n");
	load_config();
	debuglog("\nDone.\n");
	#endif
	sceCtrlSetIdleCancelThreshold(g_settings.idlestop, g_settings.idleback);
	#ifndef CONFIG_NOSAVE
	while (!g_quit)
	{
		sceKernelDelayThread(100000);
		if (g_savesettings)
		{
			g_savesettings = 0;
			save_config();
		}
	}
	#endif

	return 0;
}

/*
enum PSPInitApitype
{
	PSP_INIT_APITYPE_DISC = 0x120,
	PSP_INIT_APITYPE_DISC_UPDATER = 0x121,
	PSP_INIT_APITYPE_MS1 = 0x140,
	PSP_INIT_APITYPE_MS2 = 0x141,
	PSP_INIT_APITYPE_MS3 = 0x142,
	PSP_INIT_APITYPE_MS4 = 0x143,
	PSP_INIT_APITYPE_MS5 = 0x144,
	PSP_INIT_APITYPE_VSH1 = 0x210, / ExitGame /
	PSP_INIT_APITYPE_VSH2 = 0x220, / ExitVSH /
};
*/

/* Entry point */
int module_start(SceSize args, void *argp)
{
	int thid;

	#ifndef RELEASE
	//sceIoRemove("ms0:/SEPLUGINS/joysens.log");
	#endif
	debuglog("\n\napitype: %x\n", sceKernelInitApitype() );
	
	if (sceKernelInitApitype()<=0x100)
		return 0;
	
	if (sceKernelInitApitype()>=0x210)
	{
		strcpy(g_stringbuffer,"VSH");
		g_executable = g_stringbuffer;
	}
	else
	if (sceKernelInitApitype()==0x143)
	{
		strcpy(g_stringbuffer,"POPS");
		g_executable = g_stringbuffer;
	}
	else
	if (sceKernelInitApitype()==0x120)
	{
		// If UMD present
		if (sceUmdCheckMedium())
		{
			// Mount UMD to disc0: file system
			sceUmdActivate(1,"disc0:");
			
			// Wait for init
			sceUmdWaitDriveStat(UMD_WAITFORINIT);
		
			// Find umd id string
			int fdUMD = sceIoOpen("disc0:/UMD_DATA.BIN",PSP_O_RDONLY,0);
			char umdID[11];
			if (fdUMD >= 0)
			{
				// Read id
				sceIoRead(fdUMD,umdID,10);
				
				// End string
				umdID[10] = 0;
				
				// Close file
				sceIoClose(fdUMD);
			}
			else
				strcpy(umdID,"unknown");
			
			sceUmdDeactivate(1,"disc0:");

			debuglog("UMD ID: %s\n", umdID);
			
			g_executable = g_stringbuffer;
			strcpy(g_executable, umdID);
		}
	}
	else
	{
		g_executable = sceKernelInitFileName();
	}
	debuglog("executable: %s\n", g_executable);
	
	if (stristr(g_executable,"PSPLINK")!=0)
	{
		debuglog("\nQuitting... this plugin will not start in PSPLink");
		return 0;
	}
	debuglog("\n<-------------------------------------------------------------------->");
	debuglog("\nCreating thread...");
	thid = sceKernelCreateThread("JoySens", main_thread, 0x18, 4*1024, 0, NULL);
	if(thid >= 0)
	{
		debuglog("success.\n");
		sceKernelStartThread(thid, args, argp);
		sceKernelDeleteThread(thid);
	}
	else
		debuglog("failed!\n");
	
	return 0;
}

/* Module stop entry */
int module_stop(SceSize args, void *argp)
{
	debuglog("\nStopping module...\n");
	g_quit = 1;
	return 0;
}


int module_reboot_before(SceSize args, void *argp)
{
	debuglog("\nReboot module...\n");
	g_quit = 1;
	return 0;
}
