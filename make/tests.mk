include make/top.mk

TESTS_EXEC := $(BUILD_FOLDER)/tests
TESTS_CFILES := $(wildcard src/tests/*.c) $(UTILS_CFILES) src/idea/parser.c

all: $(TESTS_EXEC)

$(TESTS_EXEC): $(TESTS_CFILES)
	@echo "- Building the tests engine"
	mkdir -p $(BUILD_FOLDER)
	gcc $(TESTS_CFILES) -o $(TESTS_EXEC) $(FLAGS)

.PHONY: test
test: $(TESTS_EXEC) $(IDEA_EXEC)
	./$(TESTS_EXEC)

.PHONY: clean_tests
clean_tests:
	@echo "- Cleaning tests"
	rm -f $(TESTS_EXEC)
