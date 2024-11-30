# Makefile for compiling a GTK+3 application

# Variables

# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Source and target
TARGET = viva
SRC = test.c

# OS detection
ifeq ($(OS),Windows_NT)  # Windows 환경
    CFLAGS += -Ietc/PDCursesMod-master
    LDFLAGS = -Letc/PDCursesMod-master -lpdcurses -lgdi32 -luser32 -lcomdlg32 -lwinmm
else                     # macOS 또는 Linux 환경
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)  # macOS
        LDFLAGS = -lncurses
    else                     # Linux
        LDFLAGS = -lncurses
    endif
endif

# 임시
# CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
# LDFLAGS = $(shell pkg-config --libs gtk+-3.0)

# Default target
all: $(TARGET)
	@echo "Building $(TARGET)... Done!"

# Build target
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
	chmod +x $(TARGET) 

# Clean target
clean:
	rm -f $(TARGET)