#include <pspsdk.h>
#include "debug.h"

#define GET_JUMP_TARGET(x) (0x80000000 | (((x) & 0x03FFFFFF) << 2))

int hook_function(unsigned int* jump, void* hook, unsigned int* result)
{
	unsigned int target;
	unsigned int func;
	int inst;

	target = GET_JUMP_TARGET(*jump);
	while (((inst = _lw(target+4)) & ~0x03FFFFFF) != 0x0C000000)	// search next JAL instruction
		target += 4;

	if((inst & ~0x03FFFFFF) != 0x0C000000)
	{
		printf("invalid!\n");
		return 1;
	}

	*result = GET_JUMP_TARGET(inst);
	func = (unsigned int) hook;
	func = (func & 0x0FFFFFFF) >> 2;
	_sw(0x0C000000 | func, target+4);

	return 0;
}
