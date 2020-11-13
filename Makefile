proj := MiniHTTP

src := $(wildcard src/*.c)
obj := $(src:.c=.o)
CC := gcc
CFLAGS := -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -Wall -std=c99 -O3

all: $(proj)

MiniHTTP: $(obj)
	$(CC) -o $@ $(CFLAGS) $^
	rm -f $(obj)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f $(obj) $(proj)

install:
	sudo cp MiniHTTP /usr/local/bin/
	sudo cp minihttp.service /etc/systemd/system/
	sudo systemctl enable minihttp.service
	sudo systemctl start minihttp.service
