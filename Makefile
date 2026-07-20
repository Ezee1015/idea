include make/top.mk
include make/templates.mk
include make/tests.mk
include make/scripts.mk
include make/idea.mk

.PHONY: clean
clean: clean_idea clean_templates clean_tests
	if [ -d $(BUILD_FOLDER) ]; then rmdir $(BUILD_FOLDER); fi

.PHONY: install
install: install_idea install_templates install_scripts

.PHONY: uninstall
uninstall: uninstall_idea uninstall_templates uninstall_scripts
