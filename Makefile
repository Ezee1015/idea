EXEC  := build/idea
CFILES := $(wildcard src/*.c) $(wildcard src/utils/*.c)
FLAGS := -lncurses -ggdb

all: $(EXEC)

$(EXEC): $(CFILES)
	gcc $(CFILES) -o $(EXEC) $(FLAGS)

.PHONY = clean
clean:
	rm $(EXEC)

.PHONY = run
run:
	./$(EXEC)

.PHONY = install
install: $(EXEC)
	sudo cp $(EXEC) /usr/local/bin/

.PHONY = uninstall
uninstall: $(EXEC)
	sudo rm /usr/local/bin/$(EXEC)
