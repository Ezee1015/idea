EXEC  := build/idea
CFILE := $(wildcard src/*.c)
FLAGS := -lncurses

all: $(EXEC)

$(EXEC): $(CFILE)
	gcc $(CFILE) -o $(EXEC) $(FLAGS)

.PHONY = debug
debug:
	$(MAKE) FLAGS="$(FLAGS) -g" all

.PHONY = clean
clean:
	rm $(EXEC)
