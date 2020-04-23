#!/bin/bash
#
# Copyright (C) 2018 - 2020 Eaton
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
#! \file    collect_translations.sh
#  \brief   Helper script to collect strings to be translated
#  \author  Jim Klimov <EvgenyKlimov@Eaton.com>
#  \author  Jiri Kukacka <JiriKukacka@Eaton.com>

set -o pipefail

TARGET='./'
OUTPUT='all'
if [[ ! -z "$1" ]] ; then
    TARGET="$1"
    OUTPUT="$1"
fi

# .tsl = translation string list
echo "=== COLLECTING TRANSLATIONS ==="
# gather first argument of TRANSLATE_ME and TRANSLATE_ME_IGNORE_PARAMS
echo "" >"${OUTPUT}.ttsl"
# will match both TRANSLATE_ME and TRANSLATE_ME_IGNORE_PARAMS
for FILE in $(grep -rsIl --include="*.rule" --include="*.c" --include="*.cc" --include="*.cpp" --include="*.ecpp" --include="*.h" --include="*.hpp" --include="*.inc" --exclude-dir=".build" --exclude-dir=".srcclone" --exclude-dir=".install" TRANSLATE_ME "${TARGET}"); do
    # sed 's/\\$//g' - remove backslashes at the end of the lines used in #define that create false escape sequences
    # tr -d '\n' <"${files}" - collapse all newlines, so every TRANSLATE_ME will start on new line later
    # for the purpose of reading keys, TRANSLATE_ME is equal to TRANSLATE_ME_IGNORE_PARAMS
    # sed 's/TRANSLATE_ME_IGNORE_PARAMS/TRANSLATE_ME/g;s/#define *TRANSLATE_ME//;s/TRANSLATE_ME *( *"" *)//g;s/TRANSLATE_ME *( *"/\n/g' - turn TRANSLATE_ME_IGNORE_PARAMS into TRANSLATE_ME, remove #define TRANSLATE_ME, remove empty string TRANSLATE_ME(""), add newlines instead of TRANSLATE_ME
    # tail -n +2 - first line is just buzz, usually licence and includes, all the rest lines are content of TRANSLATE_ME
    # sed 's/\([^\]\)" *\(,\|)\).*$/\1/' - remove the rest of the line as we're interested only in first argument of TRANSLATE_ME
    sed 's/\\$//' "${FILE}" | tr -d '\n' | sed 's/TRANSLATE_ME_IGNORE_PARAMS/TRANSLATE_ME/g;s/#define *TRANSLATE_ME//;s/TRANSLATE_ME *( *"" *)//g;s/TRANSLATE_ME *( *"/\n/g' | tail -n +2 | sed 's/\([^\]\)" *\(,\|)\).*$/\1/' >>"${OUTPUT}.ttsl"
    # fix trailing newline as previous step removed all newlines
    echo "" >>"${OUTPUT}.ttsl"
    # process warranty rule specially as translation strings there are not quoted
    if [ -s fty-alert-engine/src/warranty.rule ] ; then
        # Legacy layout
        sed 's/\\$//' fty-alert-engine/src/warranty.rule | tr -d '\n' | sed 's/TRANSLATE_ME *( */\n/g' | tail -n +2 | sed 's/\([^\]\) *\(,\|)\).*$/\1/' >>"${OUTPUT}.ttsl" || exit
    elif [ -s fty-alert-engine/src/rule_templates/warranty.rule ]; then
        # After alert-refactoring
        sed 's/\\$//' fty-alert-engine/src/rule_templates/warranty.rule | tr -d '\n' | sed 's/TRANSLATE_ME *( */\n/g' | tail -n +2 | sed 's/\([^\]\) *\(,\|)\).*$/\1/' >>"${OUTPUT}.ttsl" || exit
    else
        echo "ERROR : fty-alert-engine/.../warranty.rule not found" >&2
        exit 22
    fi
    # fix trailing newline as previous step removed all newlines
    echo "" >>"${OUTPUT}.ttsl"
done
# remove empty lines and duplicates
sed '/^\s*$/d' "${OUTPUT}.ttsl" | sort | uniq >"${OUTPUT}.tsl"
# gather whole content of TRANSLATE_LUA
# search for all TRANSLATE_LUA strings, remove defines
# sed 's/\(TRANSLATE_LUA *(\)/\n\1/g' - ensure all TRANSLATE_LUA strings are properly located
# tail -n +2 - first line is just buzz, usually licence and includes, all the rest lines are content of TRANSLATE_LUA
# sed 's/\([^\])\)\(\\\|\)\(\"\|\x27\).*$/\1/' - \x27 is ', checking that TRANSLATE_LUA ends with )" or )\" or )' or )\'
# then also remove duplicates
grep -rsnI --include="*.rule" --include="*.c" --include="*.cc" --include="*.cpp" --include="*.ecpp" --include="*.h" --include="*.hpp" --include="*.inc" --exclude-dir=".build" --exclude-dir=".srcclone" --exclude-dir=".install" "TRANSLATE_LUA *(" ${TARGET} | grep -v '#define TRANSLATE_LUA *(' | sed 's/\(TRANSLATE_LUA *(\)/\n\1/g' | grep "TRANSLATE_LUA" | sed 's/\([^\])\)\(\\\|\)\(\"\|\x27\).*$/\1/' | sort | uniq >"${OUTPUT}_lua.tsl"
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
