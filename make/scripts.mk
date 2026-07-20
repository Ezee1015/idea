include make/top.mk
include make/scripts/auto_export_html.mk
include make/scripts/backup.mk

SCRIPTS := $(wildcard scripts/*)

.PHONY: install_scripts
install_scripts: install_scripts_auto_export_html install_scripts_backup

.PHONY: uninstall_scripts
uninstall_scripts: uninstall_scripts_auto_export_html uninstall_scripts_backup
