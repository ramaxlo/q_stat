CC = gcc
CFLAGS = $(shell pkg-config --cflags libavformat libavcodec libavutil)
CFLAGS += -Wall
LDFLAGS = $(shell pkg-config --libs libavformat libavcodec libavutil)
BIN = q_stat
OBJ = q_stat.o

.c.o:
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(BIN) $(OBJ)
