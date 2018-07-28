TARGET = joysens
OBJS = main.o debug.o hook.o exports.o

BUILD_PLUGIN = 1

# Use only kernel libraries
USE_KERNEL_LIBS = 1

INCDIR = 
CFLAGS = -Os -G0 -Wall -nostdlib -fno-builtin-printf -fno-builtin-log2
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LDFLAGS =
LIBS = -lpsprtc_driver -lpspumd_driver #-lpsppower_driver

ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG
else
CFLAGS += -DRELEASE
endif

ifeq ($(NOCONFIG),1)
else
CFLAGS += -DCONFIG
OBJS += config.o kernelLibcEx.o
ifeq ($(NOCONFIGSAVE),1)
CFLAGS += -DCONFIG_NOSAVE
endif
endif

ifeq ($(LIBM),1)
USE_PSPSDK_LIBC = 1
LIBS += -lm -lpspkernel
else
# Use the kernel's small inbuilt libc
USE_KERNEL_LIBC = 1
CFLAGS += -DNOLIBM
OBJS += minimath.o
endif

ifeq ($(LITE),1)
CFLAGS += -DJOYSENS_LITE -DCONFIG_NOSAVE
TARGET = joysens_lite
endif

ifdef BUILD_PLUGIN
CFLAGS += -DBUILD_PLUGIN
endif

full:
	@make LITE=0 all

lite:
	@make LITE=1 all

#We make a fresh build every time, because we have lots of conditional code
all: clobber

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak

release: lite full
	@$(CP) joysens.prx joysens_lite.prx ../release/SEPLUGINS

clobber:
	@echo "cleaning up"
	@$(RM) -f *.o *.elf

clean:
	@$(RM) -f *.prx *.o *.elf