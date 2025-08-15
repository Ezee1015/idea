BUILD_FOLDER := build
FLAGS := -Wall -Wextra -lncurses -ggdb
VERSION_FLAG := -DCOMMIT="\"$(shell git log -1 --format="%h | %s")\""

IDEA_EXEC_NAME  := idea
IDEA_EXEC := $(BUILD_FOLDER)/$(IDEA_EXEC_NAME)
IDEA_CFILES := $(wildcard src/*.c) $(wildcard src/utils/*.c)


all: $(IDEA_EXEC)

$(IDEA_EXEC): $(IDEA_CFILES)
	mkdir -p $(BUILD_FOLDER)
	gcc $(IDEA_CFILES) -o $(IDEA_EXEC) $(FLAGS) $(VERSION_FLAG)

.PHONY = clean
clean:
	rm $(IDEA_EXEC)

.PHONY = run
run:
	./$(IDEA_EXEC)

.PHONY = install
install: $(IDEA_EXEC)
	sudo cp $(IDEA_EXEC) /usr/local/bin/

.PHONY = uninstall
uninstall: $(IDEA_EXEC)
	sudo rm /usr/local/bin/$(IDEA_EXEC_NAME)
