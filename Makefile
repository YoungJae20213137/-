# Makefile for compiling a GTK+3 application

# Variables

# ======================Task============================
# makefile을 이용한 빌드 자동화
# 리눅스 OS, 맥 OS = ncurses
# 윈도우 OS = pdcurses
# 처리 후 실행 권한 부여 필수
# 터미널에 viva 입력 시 자동 컴파일
# ./viva viva.c 인자 처리
# ======================================================

CC = gcc
CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = $(shell pkg-config --libs gtk+-3.0)
TARGET = viva
SRC = main.c

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Clean target
clean:
	rm -f $(TARGET)
