BLOB = libmelpe.a
CC ?= gcc
LD ?= gcc

EXTRADEFS = -DPOSIX_TI_EMULATION
#-DFIXED_COMPLEX_MULTIPLY

CC_FLAGS_OS ?= -std=c99 -Wall -Wextra -pedantic
CFLAGS = $(CC_FLAGS_OS) $(EXTRADEFS)

OBJ = classify.o \
      coeff.o \
      dsp_sub.o \
      fec_code.o \
      fft_lib.o \
      fs_lib.o \
      fsvq_cb.o \
      global.o \
      harm.o \
      lpc_lib.o \
      mat_lib.o \
      math_lib.o \
      melp_ana.o \
      melp_chn.o \
      melp_sub.o \
      melp_syn.o \
      melpe.o \
      msvq_cb.o \
      npp.o \
      pit_lib.o \
      pitch.o \
      postfilt.o \
      qnt12.o \
      qnt12_cb.o \
      vq_lib.o

INCADD ?= -I. -IGCC -I../include

all: $(BLOB)

$(BLOB): $(OBJ)
	$(AR) -r $(BLOB) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) $(INCADD) -c $< -o $@

clean:
	rm -f $(OBJ) $(BLOB)

test:
	cppcheck --enable=all .
	scan-build $(MAKE)

.PHONY: all clean test

