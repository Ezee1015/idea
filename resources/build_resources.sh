#!/bin/bash

if [[ -z $HEADER_PATH ]]; then
    echo "You must set \$HEADER_PATH"
    exit 1
fi

if [[ -z $ICON_PATH ]]; then
    echo "You must set \$ICON_PATH"
    exit 1
fi

echo "- Generating resources.h..."

echo "#ifndef RESOURCES_H" > $HEADER_PATH
echo "#define RESOURCES_H" >> $HEADER_PATH
echo "#define ICON \"$(convert -scale 16x16 $ICON_PATH -strip PNG:- | base64 | tr -d '\n')\"" >> $HEADER_PATH
echo "#endif // RESOURCES_H" >> $HEADER_PATH
