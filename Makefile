BUILD_FOLDER := build

FLAGS := -Wall -Wextra -lncurses -ggdb
AVOID_CODE_INJECTION := sed 's/\\/\\\\\\\\/g; s/\"/\\\\\\\"/g'
VERSION_FLAG := -DCOMMIT="\"$(shell git log -1 --format='%h | %s' | $(AVOID_CODE_INJECTION))\""

UTILS_CFILES := $(wildcard src/utils/*.c)

TEMPLATE_FOLDER := src/template
TEMPLATE_GEN_EXEC_NAME := template_gen
TEMPLATE_GEN_EXEC := $(BUILD_FOLDER)/$(TEMPLATE_GEN_EXEC_NAME)
TEMPLATE_GEN_CFILE := $(TEMPLATE_FOLDER)/template_gen.c
TEMPLATE := $(TEMPLATE_FOLDER)/template.html

TEMPLATE_BUILD_FOLDER := $(BUILD_FOLDER)/src
TEMPLATE_CFILE := $(TEMPLATE_BUILD_FOLDER)/template.c

IDEA_EXEC_NAME  := idea
IDEA_EXEC := $(BUILD_FOLDER)/$(IDEA_EXEC_NAME)
IDEA_CFILES := $(wildcard src/*.c)

TESTS_EXEC := $(BUILD_FOLDER)/tests
TESTS_CFILES := $(wildcard tests/src/*.c) $(wildcard src/utils/*.c) src/parser.c

all: $(IDEA_EXEC) $(TESTS_EXEC)

$(TEMPLATE_GEN_EXEC): $(TEMPLATE_GEN_CFILE) $(UTILS_CFILES)
	mkdir -p $(BUILD_FOLDER)
	gcc $(UTILS_CFILES) $(TEMPLATE_GEN_CFILE) -o $(TEMPLATE_GEN_EXEC) $(FLAGS)

$(TEMPLATE_CFILE): $(TEMPLATE_GEN_EXEC) $(TEMPLATE)
	mkdir -p $(TEMPLATE_BUILD_FOLDER)
	./$(TEMPLATE_GEN_EXEC) $(TEMPLATE) $(TEMPLATE_CFILE)

$(IDEA_EXEC): $(IDEA_CFILES) $(UTILS_CFILES) $(TEMPLATE_CFILE)
	mkdir -p $(BUILD_FOLDER)
	gcc $(IDEA_CFILES) $(UTILS_CFILES) $(TEMPLATE_CFILE) -o $(IDEA_EXEC) $(FLAGS) $(VERSION_FLAG)

.PHONY = clean
clean:
	rm $(IDEA_EXEC)
	rm $(TESTS_EXEC)
	rm $(TEMPLATE_GEN_EXEC)
	rm $(TEMPLATE_CFILE)
	rmdir $(TEMPLATE_BUILD_FOLDER)

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
