all:
	gcc -o MiniHTTP -D_POSIX_C_SOURCE=200809L -Wall -std=c99 src/main.c src/request.c src/http_parser.c
