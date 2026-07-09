# Makefile for otone

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c99
LDFLAGS ?= -lm
TARGET = otone
SOURCES = otone.c
OBJECTS = $(SOURCES:.c=.o)
INSTALL_DIR ?= /usr/bin

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	install -D -m 0755 $(TARGET) $(INSTALL_DIR)/$(TARGET)
	@echo "Installed $(TARGET) to $(INSTALL_DIR)/$(TARGET)"

clean:
	rm -f $(OBJECTS) $(TARGET)

distclean: clean
	rm -f $(TARGET)

help:
	@echo "Targets: all install clean distclean help"
	@echo "Examples:"
	@echo "  make"
	@echo "  make install"
	@echo "  make CC=arm-openwrt-linux-gcc"

.PHONY: all install clean distclean help
