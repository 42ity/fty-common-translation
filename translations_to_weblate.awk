BEGIN               { print "{"; }
/^TRANSLATE_LUA/    {
                        LINE=$0;
                        EN_TEXT=LINE;
                        sub("TRANSLATE_LUA *\\(", "", EN_TEXT);
                        sub(")$", "", EN_TEXT);
                        if (NR==TNR)
                            {print "\t\""LINE"\": \""EN_TEXT"\"";}
                        else
                            {print "\t\""LINE"\": \""EN_TEXT"\",";}
                        next;
                    }
                    {
                        LINE=$0;
                        ARG_COUNT=1;
                        while (sub("% ?[^' ]", "{{var"ARG_COUNT"}}", LINE) == 1) {
                            ARG_COUNT+=1;
                        }
                        EN_TEXT=LINE;
                        if (NR==TNR)
                            {print "\t\""LINE"\": \""EN_TEXT"\"";}
                        else
                            {print "\t\""LINE"\": \""EN_TEXT"\",";}
                    }
END                 { print "}" }
