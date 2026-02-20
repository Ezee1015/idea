SCRIPT := idea_auto_export_html
INSTALL_PATH ?= /usr/local/bin
ICON_PATH ?= ../../resources/icon/icon_192x192.png

.PHONY: install
install: $(SCRIPT) $(ICON_PATH)
	@echo "- Installing $(SCRIPT) script in $(INSTALL_PATH)"
	# Source: <https://www.baeldung.com/linux/envsubst-command>
	export ICON_BASE64="$(shell convert -scale 16x16 $(ICON_PATH) -strip PNG:- | base64 | tr -d '\n')" && \
	envsubst '$$ICON_BASE64' < "$(SCRIPT)" > "$(INSTALL_PATH)/$(SCRIPT)"
	chmod +x "$(INSTALL_PATH)/$(SCRIPT)"

.PHONY: uninstall
uninstall:
	@echo "- Uninstalling $(SCRIPT) script from $(INSTALL_PATH)"
	rm "$(INSTALL_PATH)/$(SCRIPT)"
