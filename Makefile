all:
	gcc -o MiniHTTP -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -Wall -std=c99 src/main.c src/server.c src/request.c src/respond.c src/mime_types.c src/cgibase.c src/http_parser.c
