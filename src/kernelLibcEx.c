#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "debug.h"

/*
typedef union {
	float	f;
	int		i;
} ftoi_t;

int __trunc( float f )
{
	int fi;
	asm("mfc1 %0, %1\n":"=r"(fi):"f"(f));
	int exp = (fi >> 23) & 0xff;
	if (exp==0)
		return 0;
	exp-=127;
	int mantissa = (fi & 0x7fffff) | 0x800000;
	if (exp >= 23)
	{
		return mantissa << (exp - 23);
	}
	else
	if (exp>=0)
	{
		return (mantissa >> (23 - exp));
	}
	else
	{
		return 0;
	}
}
*/

char* ftoa( float f, int precision )
{
	if (precision == 0) precision--;
	
	static char svalue[64];
#if 1
	int fi;
	asm("mfc1 %0, %1\n":"=r"(fi):"f"(f));
	int sign = (fi >> 31) & 1;
	int exp = (fi >> 23) & 0xff;
	int mantissa = fi & 0x7fffff;
	//printf("%i:%i:%i [%X]\n", sign, exp, mantissa, fi);

	if (exp == 255)
	{
		if (mantissa==0)
		{
			sprintf(svalue,"%cINFINITY", sign?'-':'+');
			return svalue;
		}
		else
		{
			sprintf(svalue,"NaN");
			return svalue;
		}
	}
	int bias = 127;
	if (exp==0)
	{
		if (mantissa==0)
		{
			sprintf(svalue,"%c0.0",sign?'-':'+');
			return svalue;
		}
		// Denormalized number
		bias--;
	}
	else
	{
		mantissa |= 0x800000;
	}
	
	exp -= bias;
	long int_part = 0, frac_part = 0;
	if (exp >= 23)
	{
		int_part = (long)mantissa << (exp - 23);
	}
	else
	if (exp>=0)
	{
		int_part = ((long)mantissa >> (23 - exp));
		frac_part = ((long)mantissa << (exp + 1)) & 0xFFFFFFll;
	}
	else
	{
		int_part = 0;
		frac_part = ((long)mantissa >> -(exp + 1));
	}
	sprintf(svalue, "%li.", sign?-int_part:int_part);

	int i = strlen(svalue);
	while (i<63 && frac_part!=0 && precision--!=0)
	{
		frac_part = (frac_part << 3) + (frac_part << 1);

		svalue[i] = (frac_part >> 24) + '0';
		frac_part &= 0xFFFFFF;
		i++;
	}
	while (svalue[i-1]=='0'||svalue[i-1]=='.')
		i--;
	svalue[i] = 0;
	return svalue;
#else
	int fi = (int)f;		// Creates an FPU Exception on trunc.w.s (??) ($fd = $fs = 0.0000000 in PSPLINK)
	sprintf(svalue, "%i.", fi);
	f -= fi;
	if (f==0.0f)
	{
		strcat(svalue,"0");
	}
	else
	{
		int i = strlen(svalue);
		while (i++<63 && f>1e-8f && precision--!=0)
		{
			f *= 10.0f;
			fi = (int)f;
			svalue[i] = '0' + fi;
			f -= fi;
		}
		svalue[i] = 0;
	}
	
	return svalue;
#endif
}


int __atoi( char** s, int max )
{
	if (!s) return 0;
	if (max==0) max--;
	char* str = *s;
	if (!str || *str==0) return 0;
	
	int sign = 1;
	switch (*str)
	{
		case '-': sign = -1;
		case '+': str++;
	}
	
	int val = 0;
	while (max--!=0 && *str<='9' && *str>='0')
	{
		val = val * 10 + (*str++ - '0');
	}
	
	*s = str;
	return val*sign;
}

void __atof2( char** s, int* value, int* exp )
{
	*value = 0;
	*exp = 0;
	if (!s) return;
	char* str = *s;
	if (!str || *str==0) return;
	
	int sign = 1;
	int dec = 0;
	switch (*str)
	{
		case '-': sign = -1;
		case '+': str++;
	}

	int flag = 0;
	while ((*str>='0' && *str<='9') || (!dec && *str=='.'))
	{
		flag |= 1;
		if (*str=='.')
			dec = 1;
		else
		{
			*value = *value * 10 + (*str - '0');
			if (dec)
				(*exp)--;
		}
		str++;
	}
	if (sign<0)
		*value = -*value;
	
	// Handle exponents
	if (flag && (*str=='e'|| *str=='E'))
	{
		str++;
		(*exp) += __atoi( &str, 3 );
	}
	*s = str;
}

float __atof( char** s )
{
	if (!s) return 0.0f;
	char* str = *s;
	if (!str || *str==0) return 0.0f;
	float mag = 10.f;
	int val = 0;
	int exp = 0;
	__atof2( &str, &val, &exp );

	int n = (exp>=0?exp:-exp);
	float value = (float)val;
	while (n>0)
	{
		if (n&1)
		{
			if (exp>0)
				value *= mag;
			else
				value /= mag;
		}
		mag *= mag;
		n >>= 1;
	}

	*s = str;
	return value;
}

float atof( const char* str )
{
	char* s = str;
	return __atof( &s );
}



int	strcasecmp( const char* s1, const char* s2 )
{
	return strncasecmp( s1, s2, strlen(s1) );
}


int	strncasecmp( const char* s1, const char* s2, size_t n )
{
	while (n && *s1 && *s2)
	{
		char c1 = *s1++;
		char c2 = *s2++;

		if ((c1 >= 'a') && (c1 <= 'z'))
			c1 -= 'a' - 'A';
		if ((c2 >= 'a') && (c2 <= 'z'))
			c2 -= 'a' - 'A';

		if (c1 > c2)
			return 1;
		if (c1 < c2)
			return -1;
		n--;
	}
	
	if (n && *s1)
		return 1;
	if (n && *s2)
		return -1;

	return 0;
}


int sscanf( const char* str, const char* fmt, ... )
{
	if (!str) return 0;
	int num = 0;
	va_list vl;
	va_start(vl, fmt);
	
	while (*str && *fmt)
	{
		if (*fmt == '%')
		{
			fmt++;
			int store = 1;
			if (*fmt=='*')
			{
				fmt++;
				store = 0;
			}

			int max = 0;
			while (*fmt>='0' && *fmt<='9')
			{
				max = max * 10 + (*fmt-'0');
				fmt++;
			}
			if (max<=0) max = -1;
			
			if (*fmt=='s')
			{
				//printf("parsing string...\n");
				while (*str==' ' || *str=='\t')
					str++;
				
				char* dst = 0;
				if (store)
				{
					dst = va_arg(vl,char*);
					if (dst==0) store = 0;
					num++;
				}

				int quoted = 0;
				char quote = 0;
				if (*str=='"' || *str=='\'')
				{
					quote = *str++;
					quoted = 1;
				}
				//printf("max = %i\nstore = %i\nquote = %i (%c)\n", max, store, quoted, quoted?quote:' ');
				
				while (max--!=0 && *str && ((!quoted && *str!=' ' && *str!='\t' && *str!='\n' && *str != fmt[1]) || (quoted && *str!=quote)))
				{
					if (store)
					{
						if (*str!='\n')
							*dst++ = *str;
					}
					str++;
				}
				if (store)
					*dst = 0;
				if (*str==0 || (!quoted && *str==fmt[1]))
					str--;
			}
			else
			if (*fmt=='i' || *fmt=='d')
			{
				//printf("parsing int...\n");
				while (*str==' ' || *str=='\t')
					str++;
				int val = __atoi( &str, max );
				str--;
				if (store)
				{
					int* i = va_arg(vl,int*);
					*i = val;
					num++;
				}
			}
			else
			if (*fmt=='f' || *fmt=='g')
			{
				//printf("parsing float...\n");
				//((float*)i)[num++] = atof(str);
				while (*str==' ' || *str=='\t')
					str++;
				float value = __atof( &str );
				
				str--;
				if (store)
				{
					//printf("Value = %i.%02i\n", (int)value,(int)((value-(int)(value))*100));
					float* i = va_arg(vl,float*);
					*i = value;
					num++;
				}
			}
			else
			if (*fmt=='c')
			{
				if (max<=0)
					max = 1;
				
				while (max--!=0 && *str)
				{
					if (store)
					{
						char* i = va_arg(vl,char*);
						*i = *str;
						num++;
					}
					str++;
				}
				str--;
			}
		}
		else
		if (*fmt==' ' || *fmt=='\t')
		{
			while (*str==' ' || *str=='\t')
				str++;
			str--;
		}
		else
		if (*fmt=='\\')
		{
			fmt++;
			if (*fmt=='s')
			{
				//printf("skipping whitespaces...\n");
				while (*str==' ' || *str=='\t')
					str++;
				str--;
			}
			else
			if (*fmt=='?')
			{
				//printf("skipping one char...\n");
			}
			else
			if (*fmt=='*')
			{
				//printf("skipping variable amount of chars...\n");
				while (*str && *str != fmt[1])
					str++;
				str--;
			}
			else
			if (*fmt != *str)
				break;
		}
		else
		if (*str != *fmt)
		{
			//printf("No match at: %c == %c\n", *str, *fmt);
			break;
		}
		str++;
		fmt++;
	}
	
	va_end(vl);
	return num;
}


char*	stristr( const char* s1, const char* s2 )
{
	const char *cp = s1;
	const char *str1, *str2;

	if (!*s2)
		return s1;

	while (*cp)
	{
		str1 = cp;
		str2 = s2;

		while ( *str1 && *str2 )
		{
			char c2 = *str2;
			if ((c2 >= 'a') && (c2 <= 'z'))
				c2 -= 'a' - 'A';
	
			char c1 = *str1;
			if ((c1 >= 'a') && (c1 <= 'z'))
				c1 -= 'a' - 'A';
			
			if (c1 != c2)
				break;
			
			str1++;
			str2++;
		}

		if (!*str2)
			return cp;

		cp++;
	}
	
	return 0;
}

/*
char*	strupr(char* str)
{
	char* s = str;
	
	while (*s!=0)
	{
		if (*s>='a' && *s<='z')
			*s -= ' ';
		s++;
	}
	
	return str;
}

char*	strlwr(char* str)
{
	char* s = str;
	
	while (*s!=0)
	{
		if (*s>='A' && *s<='Z')
			*s += ' ';
		s++;
	}
	
	return str;
}
*/
