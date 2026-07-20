FLAGS := -Wall -Wextra -lncurses -ggdb

UTILS_CFILES := $(wildcard src/utils/*.c)

# Directories

## Build directories
BUILD_FOLDER := build
TEMPLATE_BUILD_FOLDER := $(BUILD_FOLDER)/src

## Source directories
RESOURCES_FOLDER := resources
TEMPLATE_FOLDER := src/idea/templates

# Executables
IDEA_EXEC := $(BUILD_FOLDER)/idea
TEMPLATE_GEN_EXEC := $(BUILD_FOLDER)/template_gen

INSTALL_PATH ?= /usr/local/bin
