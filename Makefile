CC=clang
CFLAGS=-std=c17 -Wall -Wextra -Wdeprecated -pedantic
INCLUDES=-I./ -I./vendor/raylib-6.0_macos/include/
LIBS=vendor/raylib-6.0_macos/lib/libraylib.a
FRAMEWORKS=-framework Cocoa -framework OpenGL -framework IOKit

run: main
	./$<

main: main.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) $(FRAMEWORKS) $^ -o $@

run2: main2
	./$<

run2-dbg: main2-dbg
	./$<

main2: main2.c
	$(CC) $(CFLAGS) -D"assertm(cond,...)=" -DGRID_W=2 -DPOINTSIZE=6.0f -O2 $(INCLUDES) $(LIBS) $(FRAMEWORKS) $^ -o $@

main2-dbg: main2.c
	$(CC) $(CFLAGS) -g $(INCLUDES) $(LIBS) $(FRAMEWORKS) $^ -o $@

tags:
	find -E `pwd` -type f -regex ".+\.(c|h)$$" > cscope.files
	cscope -b -q

clean:
	rm -fr *.dSYM main main2 main2-dbg cscope.*

.PHONY: run run2 tags clean
