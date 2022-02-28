BLOB = libmelpe.a
CC ?= gcc
LD ?= gcc

EXTRADEFS = -DPOSIX_TI_EMULATION -DDEBUG_PRINT -DRANDOM_GENERATOR_STDFAST

CC_FLAGS_OS ?= -std=c99 -Wall -Wextra -pedantic
CFLAGS = $(CC_FLAGS_OS) $(EXTRADEFS)

OBJ = coeff.o \
      dsp_sub.o \
      fec_code.o \
      fs_lib.o \
      fsvq_cb.o \
      lpc_lib.o \
      mat_lib.o \
      melpe.o \
      melp_ana.o \
      melp_chn.o \
      melp_sub.o \
      melp_syn.o \
      msvq_cb.o \
      pit_lib.o \
      vq_lib.o \
      posix/DSPF_sp_fftSPxSP_GCC.o \
      fft_sptable.o \
      fft_tw_256_ti.o \
      special.o \
      npp_fl.o

INCADD ?= -I. -Iposix -I../include

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

