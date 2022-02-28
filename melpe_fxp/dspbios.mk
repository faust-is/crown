
#Compiler version 7.x can be changed

TOOL_PATH ?=  $(wildcard /opt/ti/c6000_7.*)

CC := "$(TOOL_PATH)/bin/cl6x"
AR := "$(TOOL_PATH)/bin/ar6x"

LIBNAME := libmelpe
OUT_DIR := .

SRC_C :=  $(wildcard *.c) \
	$(wildcard C674X/*.c)

OBJS_C := $(patsubst %.c, %.o67, $(SRC_C))

SRC_ASM := $(wildcard ./C674X/*.asm)
OBJS_ASM := $(patsubst %.asm, %.o67, $(SRC_ASM))

SRC := $(SRC_C) $(SRC_ASM)
OBJS := $(OBJS_C) $(OBJS_ASM)

#DSP-bios generic defines
DEFINES := -DOMAPL138 \
	-Domapl138

#-DFIXED_COMPLEX_MULTIPLY

INC := --include_path="$(TOOL_PATH)/include" \
	--include_path="./" \
	--include_path="../include" \
	--include_path="./C674X" \

CC_FLAGS_OS ?= -mv6740 -O3 --gcc --abi=eabi --display_error_number --diag_warning=225 -pdv --mem_model:data=far --mem_model:const=far
#--long_precision_bits=40 --mem_model:const=data

all: $(OUT_DIR)/$(LIBNAME).lib

$(OBJS_C): %.o67 : %.c  $(SRC_C)
	$(CC) -c $(CC_FLAGS_OS) $(INC) $(DEFINES) $< -fe $@

$(OBJS_ASM): %.o67 : %.asm  $(SRC_ASM)
	$(CC) -c $(CC_FLAGS_OS) $(INC) $(DEFINES) $< -fe $@

$(OUT_DIR)/$(LIBNAME).lib: $(OBJS)
	-@echo -r> $(LIBNAME).lkf
	-@echo "$(OUT_DIR)/$(LIBNAME).lib">> $(LIBNAME).lkf
	-@echo "$(OBJS)">> $(LIBNAME).lkf
	$(AR) @"$(LIBNAME).lkf"

clean:
	rm -f $(OBJS) $(OUT_DIR)/$(LIBNAME).lib $(LIBNAME).lkf



