#!/bin/bash
MY_PATH="`dirname \"$0\"`"
target='*.tsl'
output='BE_weblate.json'
if [[ ! -z $1 ]] ; then
    target=$1
fi
if [[ ! -z $2 ]] ; then
    output=$2
fi
echo "-== CHANGING FORMAT TO WEBLATE (json) ==-"
cat ${target} > weblate_translations.tmp
awk -v TNR="$(wc -l "weblate_translations.tmp" | cut -d' ' -f1)" -f "${MY_PATH}/translations_to_weblate.awk" weblate_translations.tmp >"${output}"
rm -f weblate_translations.tmp
echo "-== DONE, OUTPUT PUT TO ${output} ==-"
