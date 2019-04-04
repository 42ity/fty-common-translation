#!/bin/bash
#
# Copyright (C) 2018-2019 Eaton
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
#! \file    translations_to_weblate.sh
#  \brief   Helper script to push strings to be translated to Weblate
#  \author  Jim Klimov <EvgenyKlimov@Eaton.com>
#  \author  Jiri Kukacka <JiriKukacka@Eaton.com>


# Ensure results are reproducible
LANG=C
LC_ALL=C
TZ=UTC
export LANG LC_ALL TZ

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
RES=0
# Note: TARGET may include wildcards, so should not be quoted
cat ${TARGET} | sort | uniq > weblate_translations.tmp && \
awk -v TNR="$(wc -l "weblate_translations.tmp" | cut -d' ' -f1)" \
    -f "${MY_PATH}/translations_to_weblate.awk" \
    weblate_translations.tmp >"${OUTPUT}" || RES=$?
rm -f weblate_translations.tmp

echo "=== DONE, OUTPUT PUT TO ${OUTPUT}, exit-code is $RES ==="
exit $RES
