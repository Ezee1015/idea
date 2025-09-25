BUILD_FOLDER := build
FLAGS := -Wall -Wextra -lncurses -ggdb
AVOID_CODE_INJECTION := sed 's/\\/\\\\\\\\/g; s/\"/\\\\\\\"/g'
VERSION_FLAG := -DCOMMIT="\"$(shell git log -1 --format='%h | %s' | $(AVOID_CODE_INJECTION))\""

IDEA_EXEC_NAME  := idea
IDEA_EXEC := $(BUILD_FOLDER)/$(IDEA_EXEC_NAME)
IDEA_CFILES := $(wildcard src/*.c) $(wildcard src/utils/*.c)

TESTS_EXEC := $(BUILD_FOLDER)/tests
TESTS_CFILES := $(wildcard tests/src/*.c) $(wildcard src/utils/*.c) src/parser.c

all: $(IDEA_EXEC) $(TESTS_EXEC)

$(IDEA_EXEC): $(IDEA_CFILES)
	mkdir -p $(BUILD_FOLDER)
	gcc $(IDEA_CFILES) -o $(IDEA_EXEC) $(FLAGS) $(VERSION_FLAG)

.PHONY = clean
clean:
	rm $(IDEA_EXEC)
	rm $(TESTS_EXEC)

.PHONY = run
run:
	./$(IDEA_EXEC)

.PHONY = install
install: $(IDEA_EXEC)
	sudo cp $(IDEA_EXEC) /usr/local/bin/

.PHONY = uninstall
uninstall: $(IDEA_EXEC)
	sudo rm /usr/local/bin/$(IDEA_EXEC_NAME)

$(TESTS_EXEC): $(TESTS_CFILES)
	mkdir -p $(BUILD_FOLDER)
	gcc $(TESTS_CFILES) -o $(TESTS_EXEC) $(FLAGS)

.PHONY = test
test: $(TESTS_EXEC) $(IDEA_EXEC)
	./$(TESTS_EXEC)
