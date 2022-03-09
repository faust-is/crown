CC ?= gcc
LD ?= gcc
LIBDIR ?= /usr/lib
INST ?= install -pD -m 644

MELP_SRC ?= melpe_fxp
#melpe_fxp or melp_fp

# NORMAL DEBUG
MODE := NORMAL
#MODE_NORMAL ?= -
MODE_DEBUG ?= -O2 -g -ggdb

CC_FLAGS_OS ?= -std=gnu99 -Wall -Wextra -pedantic
CFLAGS = $(CC_FLAGS_OS) $(MODE_$(MODE)) -DVOCODERLIB_USE_MELPE -DVOCODERLIB_USE_TETRA

INCADD ?= -I../include -I../audiodev

all: build continuous reciver

build: ../$(MELP_SRC)/libmelpe.a ../TETRA/libtetra.a libvocoder_frontend.a

continuous: continuous.o err.o filename.o ../frontend/vocoder_frontend.o
	$(CC)  $(CFLAGS) $^ -o $@ -L../$(MELP_SRC) -L../TETRA -L../audiodev -lmelpe -lm -ltetra -laudiodev -Wl,-rpath=../audiodev
	
reciver: reciver.o ../frontend/vocoder_frontend.o
	$(CC)  $(CFLAGS) $^ -o $@ -L../$(MELP_SRC) -L../TETRA -lmelpe -lm -ltetra

libvocoder_frontend.a: ../frontend/vocoder_frontend.o
	$(AR) -r $@ $^

../$(MELP_SRC)/libmelpe.a:
	$(MAKE) -C ../$(MELP_SRC) OS=posix MODE=$(MODE)

../TETRA/libtetra.a:
	$(MAKE) -C ../TETRA OS=posix MODE=$(MODE)

%.o: %.c
	$(CC) $(CFLAGS) $(INCADD) -c $< -o $@

clean:
	$(MAKE) -C  ../$(MELP_SRC) OS=posix MODE=$(MODE) clean
	$(MAKE) -C  ../TETRA OS=posix MODE=$(MODE) clean
	rm -f *.o ../frontend/vocoder_frontend.o continuous libvocoder_frontend.a

test:
	cppcheck --enable=all .
	scan-build $(MAKE)

install: build
	$(INST) ../$(MELP_SRC)/libmelpe.a -t $(DESTDIR)/$(LIBDIR)
	$(INST) ../files/melpe_standalone.pc $(DESTDIR)/$(LIBDIR)/pkgconfig/melpe.pc
	$(INST) ../include/*.h -t $(DESTDIR)/usr/include
	$(INST) libvocoder_frontend.a -t $(DESTDIR)/$(LIBDIR)
	$(INST) ../files/vocoder_frontend.pc $(DESTDIR)/$(LIBDIR)/pkgconfig/vocoder_frontend.pc
	$(INST) ../TETRA/libtetra.a -t $(DESTDIR)/$(LIBDIR)
	$(INST) ../files/tetra_standalone.pc $(DESTDIR)/$(LIBDIR)/pkgconfig/tetra.pc

.PHONY: all clean test install


$(info $$CC is [${$CC}])
$(info $$LD is [${LD}])
$(info $$LIBDIR is [${LIBDIR}])
$(info $$INST is [${INST}])
$(info $$MELP_SRC is [${MELP_SRC}])
$(info $$MODE is [${MODE}])
$(info $$MODE_DEBUG is [${MODE_DEBUG}])
$(info $$CC_FLAGS_OS is [${CC_FLAGS_OS}])
$(info $$CFLAGS is [${CFLAGS}])
$(info $$INCADD is [${INCADD}])
$(info $$AR is [${AR}])
$(info $$MAKE is [${MAKE}])
$(info $$DESTDIR is [${DESTDIR}])



