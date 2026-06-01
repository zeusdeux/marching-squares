CC=clang
CFLAGS=-std=c17 -Wall -Wextra -Wdeprecated -pedantic -g
INCLUDES=-I./ -I./vendor/raylib-6.0_macos/include/
LIBS=vendor/raylib-6.0_macos/lib/libraylib.a
FRAMEWORKS=-framework Cocoa -framework OpenGL -framework IOKit

run: main
	./$<

main: main.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) $(FRAMEWORKS) $^ -o $@

tags:
	find -E `pwd` -type f -regex ".+\.(c|h)$$" > cscope.files
	cscope -b -q

clean:
	rm -fr *.dSYM main cscope.*

.PHONY: run tags clean
