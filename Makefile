exec = tac.out
sources = $(wildcard src/*.c)
objects = $(sources:.c=.o)
# Added -Isrc/include so gcc knows where to look for headers
flags = -g -Wall -lm -ldl -fPIC -rdynamic -Isrc/include

$(exec): $(objects)
	gcc $(objects) $(flags) -o $(exec)

# Removed the strict dependency on include/%.h to allow main.c to compile
%.o: %.c
	gcc -c $(flags) $< -o $@

clean:
	-rm -f *.out
	-rm -f *.o 
	-rm -f *.a 
	-rm -f src/*.o
	-rm -f a.s a.o a.out a.s.txt # Clean up the compiler's output too

lint:
	clang-tidy src/*.c src/include/*.h