TOOL=mitxfru-tool
CROSS_COMPILE ?=
CROSS_ROOT?=
PREFIX ?= .
CC = $(CROSS_COMPILE)gcc

all: $(TOOL)

$(TOOL):
	gcc -DRECOVERY fru.c mitxfru-tool.c -o $@

.PHONY: install
install:
ifneq ($(PREFIX),.)
	cp $(TOOL) $(PREFIX)
endif

.PHONY: clean
clean:
	rm $(TOOL)
