#CGT_ARM_PREFIX="arm-linux-gnueabi-"
CGT_ARM_PREFIX ?=

ifeq ($(DEVROOT),)
SYS_OPT = 
else
SYS_OPT = --sysroot="$(DEVROOT)"
endif

CC :=  $(CGT_ARM_PREFIX)gcc
LD :=  $(CGT_ARM_PREFIX)ld
LIBDIR ?=
CFLAGS ?=-Wall -fPIC -Wextra -c -static -O3 -std=gnu99 -pedantic $(SYS_OPT)
#CFLAGS ?=-Wall -fPIC -Wextra -c -static -O3 -std=gnu99 -march=armv5te -mtune=arm926ej-s -mfloat-abi=soft -pedantic $(SYS_OPT)
LDFLAGS ?=-shared 			#linking flags

.PHONY: all clean

# Название модуля
TARGET ?= libaudiodev.so
# Исходные файлы
SOURCES ?= ad_pulse.c
#ad_oss.c
#ad_alsa.c\
#ad_pulse.c\
#ad_win32.c\
#ad_openal.c\
#ad_base.c\

HEADERS ?=\
ad.h\
sphinxbase_export.h\
sphinx_config.h\
prim_type.h
# Объектные файлы
OBJECTS = $(SOURCES:.c=.o)

all: ${TARGET}

$(TARGET): $(OBJECTS)
	$(CC) ${LDFLAGS} -o $@ $^ -lpulse-simple
#	$(CC) ${LDFLAGS} -o $@ $^ #для OSS

$(SOURCES:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) -MM $< >$@

clean:
	rm -f ${TARGET}	${OBJECTS}


