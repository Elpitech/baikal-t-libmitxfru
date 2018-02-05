TOOL=mitxfru-tool
CROSS_COMPILE ?=
CROSS_ROOT?=
PREFIX ?= .
VERSION = $(shell python gsuf/gsuf.py --main-branch master)
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -I./ -I$(CROSS_ROOT)/usr/include -DRECOVERY -DVERSION="$(VERSION)"
LDFLAGS = -L$(CROSS_ROOT)/usr/lib
SOURCES = fru.c mitxfru-tool.c
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

all: prepare $(TOOL)

prepare:
	if [ ! -e gsuf ]; then git clone https://github.com/snegovick/gsuf.git; fi

$(TOOL): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: install
install:
ifneq ($(PREFIX),.)
	cp $(TOOL) $(PREFIX)
endif

.PHONY: clean
clean:
	rm $(TOOL) $(OBJECTS)
