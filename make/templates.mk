include make/top.mk
include make/templates/completion.mk
include make/templates/html.mk

TEMPLATE_GEN_FOLDER := src/template_engine
TEMPLATE_GEN_CFILE := $(TEMPLATE_GEN_FOLDER)/template_gen.c

ICON := $(RESOURCES_FOLDER)/icon/icon_192x192.png
TOOTHLESS := $(RESOURCES_FOLDER)/toothless/toothless.gif

$(TEMPLATE_RESOURCES): $(TEMPLATE_RESOURCES_GEN) $(ICON) $(TOOTHLESS)
	@echo "- Building the template resources"
	mkdir -p $(TEMPLATE_BUILD_FOLDER)
	TOOTHLESS_PATH="$(TOOTHLESS)" ICON_PATH="$(ICON)" HEADER_PATH="$(TEMPLATE_RESOURCES)" ./$(TEMPLATE_RESOURCES_GEN)

$(TEMPLATE_GEN_EXEC): $(TEMPLATE_GEN_CFILE) $(UTILS_CFILES)
	@echo "- Building the template generator"
	mkdir -p $(BUILD_FOLDER)
	gcc $(UTILS_CFILES) $(TEMPLATE_GEN_CFILE) -o $(TEMPLATE_GEN_EXEC) $(FLAGS)

.PHONY: clean_templates
clean_templates: clean_templates_html clean_templates_completion
	@echo "- Cleaning the template generator"
	rm -f \
	    $(TEMPLATE_GEN_EXEC) \
	    $(TEMPLATE_RESOURCES)
	if [ -d $(TEMPLATE_BUILD_FOLDER) ]; then rmdir $(TEMPLATE_BUILD_FOLDER); fi

.PHONY: install_templates
install_templates: install_templates_completions

.PHONY: uninstall_templates
uninstall_templates: uninstall_templates_completions
