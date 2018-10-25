#!/bin/bash
MY_PATH="$(dirname "$0")"
TARGET='*.tsl'
OUTPUT='BE_weblate.json'
if [[ ! -z "$1" ]] ; then
    TARGET="$1"
fi
if [[ ! -z "$2" ]] ; then
    OUTPUT="$2"
fi
echo "=== CHANGING FORMAT TO WEBLATE (json) ==="
cat ${TARGET} | sort | uniq > weblate_translations.tmp
awk -v TNR="$(wc -l "weblate_translations.tmp" | cut -d' ' -f1)" -f "${MY_PATH}/translations_to_weblate.awk" weblate_translations.tmp >"${OUTPUT}"
rm -f weblate_translations.tmp
echo "=== DONE, OUTPUT PUT TO ${OUTPUT} ==="
