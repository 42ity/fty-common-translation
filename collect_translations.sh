#!/bin/bash
TARGET='./'
OUTPUT='all'
if [[ ! -z $1 ]] ; then
    TARGET=$1
    OUTPUT=$1
fi

# .tsl = translation string list
echo "=== COLLECTING TRANSLATIONS ==="
# gather first argument of TRANSLATE_ME
echo "" >"${OUTPUT}.ttsl"
for FILE in $(grep -rsIl --include="*.c" --include="*.cc" --include="*.cpp" --include="*.ecpp" --include="*.h" --include="*.hpp" --include="*.inc" --exclude-dir=".build" --exclude-dir=".srcclone" --exclude-dir=".install" TRANSLATE_ME "${TARGET}"); do
    tr -d '\n' <"${FILE}" | sed 's/#define *TRANSLATE_ME//g;s/TRANSLATE_ME *( *"" *)//g;s/TRANSLATE_ME *( *"/\n/g' | tail -n +2 | sed 's/\([^\]\)" *\(,\|)\).*$/\1/' >>"${OUTPUT}.ttsl"
    echo "" >>"${OUTPUT}.ttsl"
done
sed '/^\s*$/d' "${OUTPUT}.ttsl" | sort | uniq >"${OUTPUT}.tsl"
# gather whole content of TRANSLATE_LUA
grep -rsnI --include="*.rule" --include="*.c" --include="*.cc" --include="*.cpp" --include="*.ecpp" --include="*.h" --include="*.hpp" --include="*.inc" --exclude-dir=".build" --exclude-dir=".srcclone" --exclude-dir=".install" "TRANSLATE_LUA *(" ${TARGET} | grep -v '#define TRANSLATE_LUA *(' | sed 's/^.*\(TRANSLATE_LUA *(\)/\1/' | sed 's/\([^\])\)\(\\\|\)\(\"\|\x27\).*$/\1/' | sort | uniq >"${OUTPUT}_lua.tsl"
echo "=== CLEAN UP ==="
rm "${OUTPUT}.ttsl"
if (grep "[^\\]\"" "${OUTPUT}.tsl") ; then
    echo "=== ERROR IN ${OUTPUT}.tsl, UNESCAPED QUOTE \" CHARACTER FOUND, YOU NEED TO PERFORM MANUAL CHECK ==="
    exit 1
fi
if (grep "[^\\]\"" "${OUTPUT}_lua.tsl") ; then
    echo "=== ERROR IN ${OUTPUT}_lua.tsl, UNESCAPED QUOTE \" CHARACTER FOUND, YOU NEED TO PERFORM MANUAL CHECK ==="
    exit 1
fi
echo "=== DONE, OUTPUT PUT TO ${OUTPUT}.tsl AND ${OUTPUT}_lua.tsl ==="
