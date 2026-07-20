include make/top.mk

TEMPLATE_HTML := $(TEMPLATE_FOLDER)/html/html
TEMPLATE_HTML_CFILE := $(TEMPLATE_BUILD_FOLDER)/html.c

TEMPLATE_RESOURCES := $(TEMPLATE_BUILD_FOLDER)/html_resources.h
TEMPLATE_RESOURCES_GEN := $(RESOURCES_FOLDER)/build_resources.sh

.PHONY: clean_templates_html
clean_templates_html:
	@echo "- Cleaning generated html template C file"
	rm -f $(TEMPLATE_HTML_CFILE)

$(TEMPLATE_HTML_CFILE): $(TEMPLATE_RESOURCES) $(TEMPLATE_GEN_EXEC) $(TEMPLATE_HTML)
	mkdir -p $(TEMPLATE_BUILD_FOLDER)
	./$(TEMPLATE_GEN_EXEC) $(TEMPLATE_HTML) $(TEMPLATE_HTML_CFILE)
