CC ?=gcc 				#пользуемый компилятор
LIBDIR ?=
CFLAGS ?=-c -Wall
LDFLAGS ?=				#linking flags

LIBS ?=
INC ?=-I../audiodev

.PHONY: all clean

# Название модуля
TARGET ?= continuous
# Исходные файлы
SOURCES ?= continuous.c
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
	$(CC) ${LDFLAGS} -o $@ $^

%.o: %.c
	$(CC) ${CFFLAGS} $(INC) -c $(SOURCES) -o $(OBJECTS)

#$(TARGET): $(SOURCES)
#	$(CC) ${LDFLAGS} $(INC) -c $(SOURCES) -o $(OBJECTS)
#$(CC) $(CFLAGS) -MM $< >$@
clean:
	rm -f ${TARGET}	${OBJECTS}
