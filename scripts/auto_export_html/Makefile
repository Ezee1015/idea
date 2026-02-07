SCRIPT := idea_auto_export_html
INSTALL_PATH ?= /usr/local/bin

.PHONY: install
install: $(SCRIPT)
	@echo "- Installing $(SCRIPT) script in $(INSTALL_PATH)"
	cp $(SCRIPT) "$(INSTALL_PATH)"

.PHONY: uninstall
uninstall:
	@echo "- Uninstalling $(SCRIPT) script from $(INSTALL_PATH)"
	rm "$(INSTALL_PATH)/$(SCRIPT)"
