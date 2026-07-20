include make/top.mk

AVOID_CODE_INJECTION := sed 's/\\/\\\\\\\\/g; s/\"/\\\\\\\"/g; s/`/\\`/g'
VERSION_FLAG := -DCOMMIT="\"$(shell git log -1 --format='%h | %s' | $(AVOID_CODE_INJECTION))\""

IDEA_CFILES := $(wildcard src/idea/*.c)

$(IDEA_EXEC): $(IDEA_CFILES) $(UTILS_CFILES) $(TEMPLATE_HTML_CFILE) $(TEMPLATE_BASH_COMPLETION_CFILE) $(TEMPLATE_ZSH_COMPLETION_CFILE)
	@echo "- Building idea"
	mkdir -p $(BUILD_FOLDER)
	gcc $(IDEA_CFILES) $(UTILS_CFILES) $(TEMPLATE_HTML_CFILE) $(TEMPLATE_BASH_COMPLETION_CFILE) $(TEMPLATE_ZSH_COMPLETION_CFILE) -o $(IDEA_EXEC) $(FLAGS) $(VERSION_FLAG)

all: $(IDEA_EXEC)

.PHONY: clean_idea
clean_idea:
	@echo "- Cleaning idea executable"
	rm -f $(IDEA_EXEC)

.PHONY: install_idea
install_idea: $(IDEA_EXEC)
	@echo "- Installing idea in $(INSTALL_PATH)"
	cp $(IDEA_EXEC) "$(INSTALL_PATH)/"

.PHONY: uninstall_idea
uninstall_idea:
	@echo "- Uninstalling idea from $(INSTALL_PATH)"
	rm "$(INSTALL_PATH)/$(notdir $(IDEA_EXEC))"
