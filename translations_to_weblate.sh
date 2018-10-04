#!/bin/bash
target='*.tsl'
output='BE_weblate.json'
if [[ ! -z $1 ]] ; then
    target=$1
fi
if [[ ! -z $2 ]] ; then
    output=$2
fi

echo "CHANGING FORMAT TO WEBLATE"
cat "${target}" > weblate_translations.tmp
awk '
        BEGIN { print "{"; }
        {   if (NR==TNR)
                {print "\t\""$0"\": \""$0"\"";}
              else
                  {print "\t\""$0"\": \""$0"\",";}
        }
        END { print "}" }
    ' weblate_translations.tmp >>"${output}"
rm -f weblate_translations.tmp
echo "DONE, OUTPUT PUT TO ${output}"
