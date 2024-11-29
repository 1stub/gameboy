CC = gcc
CFLAGS = -Wall -Wextra -g -std=c23
TARGET = gameboy 
SRCS = mmu.c cpu.c ppu.c interrupt.c timer.c emulator.c main.c   
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
