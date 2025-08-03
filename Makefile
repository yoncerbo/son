RELEASE_FLAGS = -O2
DEV_FLAGS = -O0 -g3
TEST_FLAGS = -O0 -g3
CFLAGS = -std=c99 -Wall -Wextra -I ./src/headers -I ./src
file = example.c

build-release:
	mkdir -p out
	gcc ${CFLAGS} ${RELEASE_FLAGS} -o out/release src/main.c

build-dev:
	mkdir -p out
	gcc ${CFLAGS} ${DEV_FLAGS} -o out/dev src/main.c

release: build-release
	./out/release $(file)

run: build-dev
	./out/dev $(file)

build-test: src/test.c
	mkdir -p out
	gcc ${TEST_FLAGS} ${CFLAGS} -o out/test src/test.c

graph: run
	dot -Tpng out/graph.dot -o out/graph.png
	swayimg out/graph.png

test: build-test
	./out/test

clean:
	rm out -rf
