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

TEMPLATE_RESOURCES := $(TEMPLATE_BUILD_FOLDER)/resources.h
TEMPLATE_RESOURCES_GEN := $(TEMPLATE_FOLDER)/build_resources.sh

RESOURCES_FOLDER := resources
ICON := $(RESOURCES_FOLDER)/icon/icon_192x192.png

IDEA_EXEC_NAME  := idea
IDEA_EXEC := $(BUILD_FOLDER)/$(IDEA_EXEC_NAME)
IDEA_CFILES := $(wildcard src/*.c)

TESTS_EXEC := $(BUILD_FOLDER)/tests
TESTS_CFILES := $(wildcard tests/src/*.c) $(wildcard src/utils/*.c) src/parser.c

SCRIPTS := $(wildcard scripts/*)
INSTALL_PATH ?= /usr/local/bin

all: $(IDEA_EXEC) $(TESTS_EXEC)

$(TEMPLATE_RESOURCES): $(TEMPLATE_RESOURCES_GEN) $(ICON)
	mkdir -p $(TEMPLATE_BUILD_FOLDER)
	ICON_PATH="$(ICON)" HEADER_PATH="$(TEMPLATE_RESOURCES)" ./$(TEMPLATE_RESOURCES_GEN)

$(TEMPLATE_GEN_EXEC): $(TEMPLATE_GEN_CFILE) $(UTILS_CFILES)
	mkdir -p $(BUILD_FOLDER)
	gcc $(UTILS_CFILES) $(TEMPLATE_GEN_CFILE) -o $(TEMPLATE_GEN_EXEC) $(FLAGS)

$(TEMPLATE_CFILE): $(TEMPLATE_RESOURCES) $(TEMPLATE_GEN_EXEC) $(TEMPLATE)
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
	@echo "- Installing idea in $(INSTALL_PATH)"
	cp $(IDEA_EXEC) "$(INSTALL_PATH)/"
	@echo "- Installing scripts"
	for script in $(SCRIPTS); do \
		cd "$$script" && $(MAKE) install INSTALL_PATH="$(INSTALL_PATH)" ICON_PATH="../../$(ICON)" ;\
		cd ../.. ;\
	done

.PHONY = uninstall
uninstall: $(IDEA_EXEC)
	@echo "- Uninstalling idea from $(INSTALL_PATH)"
	rm "$(INSTALL_PATH)/$(IDEA_EXEC_NAME)"
	@echo "- Uninstalling scripts"
	for script in $(SCRIPTS); do \
		cd "$$script" && $(MAKE) uninstall INSTALL_PATH="$(INSTALL_PATH)" ;\
	done

$(TESTS_EXEC): $(TESTS_CFILES)
	mkdir -p $(BUILD_FOLDER)
	gcc $(TESTS_CFILES) -o $(TESTS_EXEC) $(FLAGS)

.PHONY = test
test: $(TESTS_EXEC) $(IDEA_EXEC)
	./$(TESTS_EXEC)
