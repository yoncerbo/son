CFLAGS = -std=c99 -O2 -g3 -Wall -Wextra
file = example.sm

build: src/main.c
	mkdir -p out
	gcc ${CFLAGS} -o out/main src/main.c -I ./src/headers

run: build
	./out/main $(file)

clean:
	rm out -rf
