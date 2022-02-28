BLOB = libtetra.a
CC ?= gcc
LD ?= gcc

EXTRADEFS = -DPOSIX_TI_EMULATION
SUBTARGET ?= posix

CC_FLAGS_OS ?= -std=c99 -Wall -Wextra -pedantic
CFLAGS = $(CC_FLAGS_OS) $(EXTRADEFS)

OBJ = fbas_tet.o \
        fexp_tet.o \
        fmat_tet.o \
        scod_tet.o \
        sdec_tet.o \
        sub_dsp.o \
        sub_sc_d.o \
        tetra_api_impl.o \
        $(SUBTARGET)/tetra_op.o

#test utils scoder and sdecoder can be used for bit-compatibility tests with original impl

TEST_CFLAGS  =       -I. -I$(SUBTARGET) $(EXTRADEFS)

SRC_SCODER          =       scoder.c

SRC_SDECODER           =    sdecoder.c

INCADD ?= -I. -I$(SUBTARGET) -I../include

all: $(BLOB) scoder sdecoder

$(BLOB): $(OBJ)
	$(AR) -r $(BLOB) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) $(INCADD) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) $(INCADD) -c $< -o $@

clean:
	rm -f $(OBJ) $(BLOB) scoder sdecoder


scoder: $(BLOB)
	$(CC) $(SRC_SCODER) $(TEST_CFLAGS) -o scoder -ltetra -L.

sdecoder: $(BLOB)
	$(CC) $(SRC_SDECODER) $(TEST_CFLAGS) -o sdecoder -ltetra -L.

test:
	cppcheck --enable=all .
	scan-build $(MAKE)

.PHONY: all clean test

