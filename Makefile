TOOL=mitxfru-tool
CROSS_COMPILE ?=
CROSS_ROOT?=
PREFIX ?= .
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -I./ -I$(CROSS_ROOT)/usr/include -DRECOVERY
LDFLAGS = -L$(CROSS_ROOT)/usr/lib
SOURCES = fru.c mitxfru-tool.c
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

all: $(TOOL)

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
	rm $(TOOL)
