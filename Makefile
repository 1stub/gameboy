CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = gameboy 
SRCS = main.c emulator.c cpu.c mmu.c 
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
