CC ?=gcc 				#пользуемый компилятор
LIBDIR ?=
CFLAGS ?=-Wall -fPIC -Wextra -g
LDFLAGS ?=-shared 			#linking flags

.PHONY: all clean

# Название модуля
TARGET ?= audiodev.so
# Исходные файлы
SOURCES ?=\
ad_oss.c
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
	$(CC) ${LDFLAGS} -o $@ $^

$(SOURCES:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) -MM $< >$@

clean:
	rm -f ${TARGET}	${OBJECTS}


