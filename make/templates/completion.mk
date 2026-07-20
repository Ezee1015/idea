include make/top.mk

COMPLETION_FOLDER := $(BUILD_FOLDER)/autocomplete

# Bash
TEMPLATE_BASH_COMPLETION := $(TEMPLATE_FOLDER)/bash_completion/bash_completion
TEMPLATE_BASH_COMPLETION_CFILE := $(TEMPLATE_BUILD_FOLDER)/$(notdir $(TEMPLATE_BASH_COMPLETION)).c
BASH_COMPLETION := $(COMPLETION_FOLDER)/bash
BASH_COMPLETION_INSTALL_PATH ?= /usr/share/bash-completion/completions/idea

# Zsh
TEMPLATE_ZSH_COMPLETION := $(TEMPLATE_FOLDER)/zsh_completion/zsh_completion
TEMPLATE_ZSH_COMPLETION_CFILE := $(TEMPLATE_BUILD_FOLDER)/$(notdir $(TEMPLATE_ZSH_COMPLETION)).c
ZSH_COMPLETION := $(COMPLETION_FOLDER)/zsh
ZSH_COMPLETION_INSTALL_PATH := /usr/local/share/zsh/site-functions/_idea

COMPLETIONS := $(BASH_COMPLETION) $(ZSH_COMPLETION)

all: $(COMPLETIONS)

# Bash
$(TEMPLATE_BASH_COMPLETION_CFILE): $(TEMPLATE_GEN_EXEC) $(TEMPLATE_BASH_COMPLETION)
	@echo "- Generating the Bash completion C file"
	mkdir -p $(TEMPLATE_BUILD_FOLDER)
	./$(TEMPLATE_GEN_EXEC) $(TEMPLATE_BASH_COMPLETION) $(TEMPLATE_BASH_COMPLETION_CFILE)

$(BASH_COMPLETION): $(IDEA_EXEC)
	@echo "- Generating the Bash completion script"
	mkdir -p $(COMPLETION_FOLDER)
	$(IDEA_EXEC) generate_autocomplete bash $(BASH_COMPLETION)

# Zsh
$(TEMPLATE_ZSH_COMPLETION_CFILE): $(TEMPLATE_GEN_EXEC) $(TEMPLATE_ZSH_COMPLETION)
	@echo "- Generating the Zsh completion C file"
	mkdir -p $(TEMPLATE_BUILD_FOLDER)
	./$(TEMPLATE_GEN_EXEC) $(TEMPLATE_ZSH_COMPLETION) $(TEMPLATE_ZSH_COMPLETION_CFILE)

$(ZSH_COMPLETION): $(IDEA_EXEC)
	@echo "- Generating the Zsh completion script"
	mkdir -p $(COMPLETION_FOLDER)
	$(IDEA_EXEC) generate_autocomplete zsh $(ZSH_COMPLETION)

.PHONY: clean_templates_completion
clean_templates_completion:
	@echo "- Cleaning generated completions C files"
	rm -f \
	    $(BASH_COMPLETION) \
	    $(ZSH_COMPLETION) \
	    $(TEMPLATE_BASH_COMPLETION_CFILE) \
	    $(TEMPLATE_ZSH_COMPLETION_CFILE)
	if [ -d $(COMPLETION_FOLDER) ]; then rmdir $(COMPLETION_FOLDER); fi

.PHONY: install_templates_completions
install_templates_completions: $(BASH_COMPLETION) $(ZSH_COMPLETION)
	@echo "- Install bash completion"
	cp $(BASH_COMPLETION) "$(BASH_COMPLETION_INSTALL_PATH)"
	@echo "- Install zsh completion"
	cp $(ZSH_COMPLETION) "$(ZSH_COMPLETION_INSTALL_PATH)"

.PHONY: uninstall_templates_completions
uninstall_templates_completions:
	@echo "- Uninstalling idea bash autocomplete from $(BASH_COMPLETION_INSTALL_PATH)"
	rm "$(BASH_COMPLETION_INSTALL_PATH)"
	@echo "- Uninstalling idea bash autocomplete from $(ZSH_COMPLETION_INSTALL_PATH)"
	rm "$(ZSH_COMPLETION_INSTALL_PATH)"
