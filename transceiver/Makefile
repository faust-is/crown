CGT_ARM_PREFIX="arm-linux-gnueabi-"

SRC_MELP ?= melpe_fxp

ifeq ($(DEVROOT),)
SYS_OPT = 
else
SYS_OPT = --sysroot="$(DEVROOT)"
endif

CC_FLAGS_OS ?= -static -O3 -std=gnu99 -march=armv5te -mtune=arm926ej-s -mfloat-abi=soft -Wall -pedantic $(SYS_OPT)
CC_FLAGS_COMMON := $(CC_FLAGS_OS)
LD_FLAGS_COMMON := $(SYS_OPT)

CC := $(CGT_ARM_PREFIX)gcc
LD := $(CGT_ARM_PREFIX)ld
AR := $(CGT_ARM_PREFIX)ar

LIBDIR ?= -L. -L../frontend\
-L../TETRA\
-L../$(SRC_MELP)

#CFLAGS ?= -c -Wall
CFLAGS := $(OS_FLAGS_COMMON) $(CC_FLAGS_COMMON) $(CC_DEFINES)
LD_FLAGS := $(LD_FLAGS_COMMON) $(LD_FLAGS_OS) -lm


LIBS ?= -laudiodev\
-lvocoder_frontend\
-ltetra\
-lmelpe


INC ?=-I../audiodev -I../include

.PHONY: all clean

# Название модуля
TARGET ?= continuous
# Исходные файлы
SOURCES ?= continuous.c\
err.c\
filename.c

HEADERS ?=\
ad.h\
sphinxbase_export.h\
sphinx_config.h\
prim_type.h
# Объектные файлы
OBJECTS = $(SOURCES:.c=.o)

all: ${TARGET}
# linking
$(TARGET): $(OBJECTS)
	$(CC) ${LDFLAGS} -o $@ $^ $(LIBDIR) $(LIBS) $(DEFINES) -Wl,-rpath=.

$(OBJECTS): %.o: %.c
	$(CC) ${CFFLAGS} $(INC) -c $<

clean:
	rm -f ${TARGET}	${OBJECTS}
