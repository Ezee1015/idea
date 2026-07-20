include make/top.mk

SCRIPT_AUTO_EXPORT_HTML := scripts/auto_export_html/idea_auto_export_html
ICON_PATH := resources/icon/icon_192x192.png

.PHONY: install
install_scripts_auto_export_html: $(SCRIPT_AUTO_EXPORT_HTML) $(ICON_PATH)
	@echo "- Installing $(SCRIPT_AUTO_EXPORT_HTML) script in $(INSTALL_PATH)"
	# Source: <https://www.baeldung.com/linux/envsubst-command>
	export ICON_BASE64="$(shell magick convert -scale 16x16 $(ICON_PATH) -strip PNG:- | base64 | tr -d '\n')" && \
	envsubst '$$ICON_BASE64' < "$(SCRIPT_AUTO_EXPORT_HTML)" > "$(INSTALL_PATH)/$(notdir $(SCRIPT_AUTO_EXPORT_HTML))"
	chmod +x "$(INSTALL_PATH)/$(notdir $(SCRIPT_AUTO_EXPORT_HTML))"

.PHONY: uninstall
uninstall_scripts_auto_export_html:
	@echo "- Uninstalling $(SCRIPT_AUTO_EXPORT_HTML) script from $(INSTALL_PATH)"
	rm "$(INSTALL_PATH)/$(notdir $(SCRIPT_AUTO_EXPORT_HTML))"
