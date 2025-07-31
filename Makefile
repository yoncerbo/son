RELEASE_FLAGS = -std=c99 -O2 -Wall -Wextra
DEV_FLAGS = -std=c99 -O0 -g3 -Wall -Wextra
file = example.c

build-release:
	mkdir -p out
	gcc ${RELEASE_FLAGS} -o out/release src/main.c -I ./src/headers

build-dev:
	mkdir -p out
	gcc ${DEV_FLAGS} -o out/dev src/main.c -I ./src/headers

release: build-release
	./out/release $(file)

run: build-dev
	./out/dev $(file)

build-test: src/test.c
	mkdir -p out
	gcc ${CFLAGS} -o out/test src/test.c -I ./src/headers

test: build-test
	./out/test

clean:
	rm out -rf
