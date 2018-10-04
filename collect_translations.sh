#!/bin/bash
target='./'
output='all'
if [[ ! -z $1 ]] ; then
    target=$1
    output=$1
fi

# .tsl = translation string list
echo "COLLECTING TRANSLATIONS"
# gather first argument of TRANSLATE_ME
echo "" >"${output}.tsl"
for files in $(grep -rsIl --include="*.c" --include="*.cc" --include="*.cpp" --include="*.ecpp" --include="*.h" --include="*.hpp" --include="*.inc" --exclude-dir=".build" --exclude-dir=".srcclone" --exclude-dir=".install" TRANSLATE_ME "${target}"); do tr -d '\n' <"${files}" | sed 's/#define *TRANSLATE_ME//g;s/TRANSLATE_ME *(/\n/g' | sed 's/\([^\]\)".*$/\1/' | sort | uniq >"${output}.tsl" ; done
# gather whole content of TRANSLATE_LUA
grep -rsnI --include="*.rule" --include="*.c" --include="*.cc" --include="*.cpp" --include="*.ecpp" --include="*.h" --include="*.hpp" --include="*.inc" --exclude-dir=".build" --exclude-dir=".srcclone" --exclude-dir=".install" "TRANSLATE_LUA *(" ${target} | grep -v '#define TRANSLATE_LUA *(' | sed 's/^.*TRANSLATE_LUA *(//' | sed 's/\([^\]\)).*$/\1/' | sort | uniq >"${output}_lua.tsl" # .tsl = translation string list
if (grep "[^\\]\"" "${output}") ; then
    echo "ERROR, UNESCAPED QUOTE \" CHARACTER FOUND, YOU NEED TO PERFORM MANUAL CHECK"
    exit 1
fi
echo "DONE, OUTPUT PUT TO ${output}"
