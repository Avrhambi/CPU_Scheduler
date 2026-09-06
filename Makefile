CC = gcc
CFLAGS = -Wall -Wextra -g -I.

TARGET = CPU-Scheduler
SRCS = src/main.c src/state.c src/process.c src/dispatcher.c src/ui.c src/scheduler.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
