# Makefile for OpenWRT Tone Generator

# Compiler and flags
CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c99
LDFLAGS ?= -lrt

# Target executable
TARGET = tone
SOURCES = tone.c
OBJECTS = $(SOURCES:.c=.o)

# Installation directory (can be overridden)
INSTALL_DIR ?= /usr/bin

# Default target
all: $(TARGET)

# Compilation rule
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS) || $(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Object file compilation
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Install target (for router)
install: $(TARGET)
	@echo "Installing $(TARGET) to $(INSTALL_DIR)/"
	install -D -m 0755 $(TARGET) $(INSTALL_DIR)/$(TARGET)
	@echo "Installation complete. Run as root: $(TARGET) <freq_hz> <duration_ms>"

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Clean everything including compiled binary
distclean: clean
	rm -f $(TARGET)

# Help target
help:
	@echo "OpenWRT Tone Generator - Makefile targets:"
	@echo ""
	@echo "  make              - Build the tone binary"
	@echo "  make install      - Install to $(INSTALL_DIR)/ (requires root)"
	@echo "  make clean        - Remove object files"
	@echo "  make distclean    - Remove all generated files"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  CC                - C compiler (default: gcc)"
	@echo "  CFLAGS            - Compiler flags (default: $(CFLAGS))"
	@echo "  INSTALL_DIR       - Installation directory (default: $(INSTALL_DIR))"
	@echo ""
	@echo "Examples:"
	@echo "  make                           # Default build with gcc"
	@echo "  make CC=arm-openwrt-linux-gcc  # Cross-compile for OpenWRT"
	@echo "  make install INSTALL_DIR=/tmp  # Install to custom location"

.PHONY: all install clean distclean help
