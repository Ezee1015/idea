EXEC  := idea
CFILES := $(wildcard src/*.c) $(wildcard src/utils/*.c)
FLAGS := -Wall -Wextra -lncurses -ggdb
BUILD_FOLDER := build

all: $(EXEC)

$(EXEC): $(CFILES)
	mkdir -p $(BUILD_FOLDER)
	gcc $(CFILES) -o $(BUILD_FOLDER)/$(EXEC) $(FLAGS)

.PHONY = clean
clean:
	rm $(BUILD_FOLDER)/$(EXEC)

.PHONY = run
run:
	./$(BUILD_FOLDER)/$(EXEC)

.PHONY = install
install: $(EXEC)
	sudo cp $(BUILD_FOLDER)/$(EXEC) /usr/local/bin/

.PHONY = uninstall
uninstall: $(EXEC)
	sudo rm /usr/local/bin/$(EXEC)
