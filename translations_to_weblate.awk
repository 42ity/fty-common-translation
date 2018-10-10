BEGIN               { print "{"; }
/^TRANSLATE_LUA/    {
                        line=$0;
                        en_text=line;
                        sub("TRANSLATE_LUA *\\(", "", en_text);
                        sub(")$", "", en_text);
                        if (NR==TNR)
                            {print "\t\""line"\": \""en_text"\"";}
                        else
                            {print "\t\""line"\": \""en_text"\",";}
                        next;
                    }
                    {
                        line=$0;
                        arg_count=1;
                        while (sub("% ?[^' ]", "$var"arg_count"$", line) == 1) {
                            arg_count+=1;
                        }
                        en_text=line;
                        if (NR==TNR)
                            {print "\t\""line"\": \""en_text"\"";}
                        else
                            {print "\t\""line"\": \""en_text"\",";}
                    }
END                 { print "}" }
