include make/top.mk

SCRIPT_BACKUP := scripts/backup/idea_backup

.PHONY: install
install_scripts_backup: $(SCRIPT_BACKUP)
	@echo "- Installing $(SCRIPT_BACKUP) script in $(INSTALL_PATH)"
	cp $(SCRIPT_BACKUP) "$(INSTALL_PATH)"

.PHONY: uninstall
uninstall_scripts_backup:
	@echo "- Uninstalling $(SCRIPT_BACKUP) script from $(INSTALL_PATH)"
	rm "$(INSTALL_PATH)/$(notdir $(SCRIPT_BACKUP))"
