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
    # sed 's/\\$//g' - remove backslashes at the end of the lines used in #define that create false escape sequences
    # tr -d '\n' <"${files}" - collapse all newlines, so every TRANSLATE_ME will start on new line later
    # sed 's/#define *TRANSLATE_ME//g;s/TRANSLATE_ME *( *"" *)//g;s/TRANSLATE_ME *( *"/\n/g' - remove #define TRANSLATE_ME, remove empty string TRANSLATE_ME(""), add newlines instead of TRANSLATE_ME
    # tail -n +2 - first line is just buzz, usually licence and includes, all the rest lines are content of TRANSLATE_ME
    # sed 's/\([^\]\)" *\(,\|)\).*$/\1/' - remove the rest of the line as we're interested only in first argument of TRANSLATE_ME
    sed 's/\\$//' "${FILE}" | tr -d '\n' | sed 's/#define *TRANSLATE_ME//;s/TRANSLATE_ME *( *"" *)//;s/TRANSLATE_ME *( *"/\n/g' | tail -n +2 | sed 's/\([^\]\)" *\(,\|)\).*$/\1/' >>"${OUTPUT}.ttsl"
    # fix trailing newline as previous step removed all newlines
    echo "" >>"${OUTPUT}.ttsl"
done
# remove empty lines and duplicates
sed '/^\s*$/d' "${OUTPUT}.ttsl" | sort | uniq >"${OUTPUT}.tsl"
# gather whole content of TRANSLATE_LUA
# search for all TRANSLATE_LUA strings, remove defines
# sed 's/^.*\(TRANSLATE_LUA *(\)/\1/' - remove start of the line up to TRANSLATE_LUA
# sed 's/\([^\])\)\(\\\|\)\(\"\|\x27\).*$/\1/' - \x27 is ', checking that TRANSLATE_LUA ends with )" or )\" or )' or )\'
# then also remove duplicates
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
