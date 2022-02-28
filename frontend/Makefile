#CGT_ARM_PREFIX="/home/user/iep/host/usr/bin/arm-none-linux-gnueabi-"
#sudo apt install gcc-arm-linux-gnueabi
CGT_ARM_PREFIX="arm-linux-gnueabi-"

#CGT_ARM_PREFIX          ?= arm-gentoo-linux-gnueabi-
SRC_MELP ?= melpe_fxp
#SRC_MELP ?= melp_fp
MODE := NORMAL
ifeq ($(DEVROOT),)
SYS_OPT = 
else
SYS_OPT = --sysroot="$(DEVROOT)"
endif

CC_OPTIM := -O3 -D NDEBUG 
#-ggdb -D DEBUG

#CC_FLAGS_OS ?= -O3 -std=gnu99 -march=armv5te -mtune=arm926ej-s -mfloat-abi=soft -Wall -pedantic $(SYS_OPT)
CC_FLAGS_OS ?= -static -O3 -std=gnu99 -march=armv5te -mtune=arm926ej-s -mfloat-abi=soft -Wall -pedantic $(SYS_OPT)
CC_FLAGS_COMMON := $(CC_FLAGS_OS)
LD_FLAGS_COMMON := $(SYS_OPT)

CC :=  $(CGT_ARM_PREFIX)gcc
LD :=  $(CGT_ARM_PREFIX)ld
AR := $(CGT_ARM_PREFIX)ar


CFLAGS := $(OS_FLAGS_COMMON) $(CC_FLAGS_COMMON) $(CC_DEFINES)
LD_FLAGS := $(LD_FLAGS_COMMON) $(LD_FLAGS_OS)

SRC := vocoder_frontend.c
#SRC_TEST := ../test/votest.c

INC := -I. -I../include

INC_TEST := $(INC)

DEFINES =  -D_REENTRANT \
           -Dxdc_target_name__=GCArmv5T \
           -Dxdc_target_types__=gnu/targets/arm/std.h \
           -DVOCODERLIB_USE_MELPE \
           -DVOCODERLIB_USE_TETRA

LDFLAGS_APP =   -L. -lvocoder_frontend \
                -L../$(SRC_MELP) -lmelpe \
                -L../TETRA -ltetra \
                -lpthread -lc -lrt -lm

all: libvocoder_frontend.a ../$(SRC_MELP)/libmelpe.a ../TETRA/libtetra.a

#sw emulation layer MELP
../$(SRC_MELP)/libmelpe.a:
	$(MAKE) -C ../$(SRC_MELP) CC=$(CGT_ARM_PREFIX)gcc LD=$(CGT_ARM_PREFIX)ld AR=$(CGT_ARM_PREFIX)ar CC_FLAGS_OS="$(CC_FLAGS_OS)" OS=posix MODE=$(MODE)

#sw emulation layer TETRA
../TETRA/libtetra.a:
	$(MAKE) -C ../TETRA CC=$(CGT_ARM_PREFIX)gcc LD=$(CGT_ARM_PREFIX)ld AR=$(CGT_ARM_PREFIX)ar CC_FLAGS_OS="$(CC_FLAGS_OS)" OS=posix SUBTARGET=armv5te MODE=$(MODE)

#common frontend (can use sw emulation if hw not found)
libvocoder_frontend.a: $(SRC)
	$(CC) $(CFLAGS) $(INC) $(DEFINES) -c $(filter %.c, $^)
	$(AR) -r $@ $(patsubst %.c, %.o, $(notdir $(filter %.c, $^)))

#votest: all ../$(SRC_MELP)/libmelpe.a ../TETRA/libtetra.a
#	$(CC) ../test/votest.c $(CFLAGS) $(INC) $(DEFINES) -o votest $(LD_FLAGS) $(LDFLAGS_APP)

#codec: all ../$(SRC_MELP)/libmelpe.a ../TETRA/libtetra.a
#	$(CC) ../test/codec.c  $(CFLAGS) $(INC) $(DEFINES) -o codec $(LD_FLAGS) $(LDFLAGS_APP)

#votest_mt: all ../$(SRC_MELP)/libmelpe.a ../TETRA/libtetra.a
#	$(CC) ../test/votest_mt.c ../test/ut_md5.c $(CFLAGS) $(INC) $(DEFINES) -o votest_mt $(LD_FLAGS) $(LDFLAGS_APP)

clean:
	rm -f *.o *.d
	rm -f *.a
#	rm -f votest codec votest_mt
	$(MAKE) -C  ../$(SRC_MELP) OS=posix MODE=$(MODE) clean
	$(MAKE) -C  ../TETRA OS=posix SUBTARGET=armv5te MODE=$(MODE) clean

.PHONY: all clean

$(info $$CGT_ARM_PREFIX is [${CGT_ARM_PREFIX}])

$(info $$SRC_MELP is [${SRC_MELP}])
$(info $$MODE is [${MODE}])
$(info $$DEVROOT is [${DEVROOT}])
$(info $$SYS_OPT is [${SYS_OPT}])
$(info $$CC_OPTIM is [${CC_OPTIM}])
$(info $$CC_FLAGS_OS is [${CC_FLAGS_OS}])
$(info $$CC_FLAGS_COMMON is [${CC_FLAGS_COMMON}])
$(info $$LD_FLAGS_COMMON is [${LD_FLAGS_COMMON}])
$(info $$CC is [${CC}])
$(info $$LD is [${LD}])
$(info $$AR is [${AR}])
$(info $$CFLAGS is [${CFLAGS}])
$(info $$OS_FLAGS_COMMON is [${OS_FLAGS_COMMON}])
$(info $$CC_DEFINES is [${CC_DEFINES}])
$(info $$LD_FLAGS is [${LD_FLAGS}])
$(info $$LD_FLAGS_COMMON is [${LD_FLAGS_COMMON}])
$(info $$LD_FLAGS_OS is [${LD_FLAGS_OS}])
$(info $$INC is [${INC}])
$(info $$INC_TEST is [${INC_TEST}])
$(info $$MAKE is [${MAKE}])
$(info $$INC is [${INC}])
$(info $$DEFINES is [${DEFINES}])
$(info $$LDFLAGS_APP is [${LDFLAGS_APP}])

