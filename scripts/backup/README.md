# Backup

This script makes a backup of idea's DB every time you save your changes in idea.

## Dependencies

- `inotify-tools`
- Internet connection for the `-c` argument to clone the idea repo (optional, you can switch to the offline version inside the script code)

## How it works?

1. You have to run this script at startup: `idea_backup --daemon`
2. Now, whenever you save your changes in idea, this script is going to copy the data of the ToDos in the backup directory

You can see the available options of the script with: `idea_backup.sh -h`
