/*  =========================================================================
    fty_common_translation_base - Singleton translation object

    Copyright (C) 2014 - 2018 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

/*
@header
    fty_common_translation_base - Singleton translation object
@discuss
@end
*/

#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cctype>
#include <iostream>
#include <stdexcept>

#include "fty_common_translation_classes.h"

#define VARIABLES "variables"
#define VARIABLE "variable"
#define KEY "key"
#define FILE_EXTENSION ".json"

// trim from start (in place)
static inline void ltrim (std::string &s) {
    s.erase (s.begin (), std::find_if (s.begin (), s.end (), [] (int ch) {
        return !std::isspace (ch);
    }));
}


// trim from end (in place)
static inline void rtrim (std::string &s) {
    s.erase (std::find_if (s.rbegin (), s.rend (), [] (int ch) {
        return !std::isspace (ch);
    }).base (), s.end ());
}

// trim from both ends (in place)
static inline void trim (std::string &s) {
    ltrim (s);
    rtrim (s);
}


JSON_TYPE Translation::getNextObject (const std::string &line, size_t &start_pos) {
    start_pos = line.find_first_not_of ("\t :,", start_pos);
    if (start_pos == std::string::npos) {
        return JT_None;
    }
    switch (line.at (start_pos)) {
        case '{':
            return JT_Object;
            break;
        case '}':
            return JT_Object_End;
            break;
        case '"':
            return JT_String;
            break;
        default:
            return JT_Invalid;
            break;
    }
}


std::string Translation::readObject (const std::string &line, size_t &start_pos, size_t &end_pos) {
    size_t temp = 0;
    end_pos = 0;
    start_pos = line.find_first_of ('{', start_pos);
    if (std::string::npos == start_pos) {
        throw NotFoundException ();
    }
    int count = 1;
    temp = start_pos + 1;
    while (end_pos == 0) {
        temp = line.find_first_of ("{}", temp + 1); // always searching at pos > 0
        if (std::string::npos == temp) {
            throw CorruptedLineException ();
        } else if (line.at (temp) == '{') {
            ++count;
        } else {
            --count; // closing curly bracket
        }
        if (count == 0) {
            end_pos = temp;
        }
    }
    return line.substr (start_pos, end_pos - start_pos + 1);
}


std::string Translation::readString (const std::string &line, size_t &start_pos, size_t &end_pos) {
    size_t temp = 0;
    end_pos = 0;
    start_pos = line.find_first_of ('"', start_pos);
    if (std::string::npos == start_pos) {
        throw NotFoundException ();
    }
    temp = start_pos + 1;
    while (end_pos == 0) {
        temp = line.find_first_of ('"', temp + 1); // always searching at pos > 0
        if (std::string::npos == temp) {
            throw CorruptedLineException ();
        }
        if (line[temp - 1] != '\\') {
            end_pos = temp;
        }
    }
    return line.substr (start_pos + 1, end_pos - start_pos - 1);
}


std::string Translation::getTranslatedText (const std::string &json) {
    return getTranslatedText (language_order_, json);
}


std::string Translation::getTranslatedText (const TRANSLATION_CONFIGURATION &conf, const std::string &json) {
    auto order_it = language_list_ordering_.find (conf.language);
    if (order_it == language_list_ordering_.end ()) {
        throw LanguageNotLoadedException ();
    }
    return getTranslatedText (order_it->second, json);
}


std::string Translation::getTranslatedText (const size_t order, const std::string &json) {
    // TODO add handling of special variables that might be just formated, such as { "variable" : "IPC 2000", "link": "http://42ity.org/" }
    std::string key, value, retval;
    size_t begin = 0, end = 0;
    // read basic "key" : "translation_key" pair and validate
    begin = json.find_first_not_of ("\t ");
    if (json.at (begin) != '{') {
        throw CorruptedLineException ();
    }
    if (json.at (json.find_last_not_of ("\t ")) != '}') {
        throw CorruptedLineException ();
    }
    ++begin;
    if (getNextObject (json, begin) == JT_String) {
        key = readString (json, begin, end);
    } else {
        throw CorruptedLineException ();
    }
    begin = end + 1;
    if (getNextObject (json, begin) == JT_String) {
        value = readString (json, begin, end);
    } else {
        throw CorruptedLineException ();
    }
    // check for "key" keyword
    if (key.find (KEY) == std::string::npos && key.find (VARIABLE) == std::string::npos) {
        throw CorruptedLineException ();
    }
    if (key.find (VARIABLE) != std::string::npos) {
        log_error ("Unexpected input '%s', handling of \"variable\" is not implemented", json.c_str ());
        throw std::logic_error ("Not implemented");
    }
    // find translation string matching translation_key
    auto mapping = language_translations_.find (value);
    if (language_translations_.end () == mapping) {
        throw TranslationNotFoundException ();
    }
    retval = mapping->second.at (order);
    if (retval.empty ()) {
        // fallback to default language
        retval = mapping->second.at (0);
    }
    // load variables if present
    begin = end + 1;
    if (getNextObject (json, begin) == JT_String) {
        key = readString (json, begin, end);
        // check for "variables" keyword
        if (json.find (VARIABLES, begin) == std::string::npos) {
            throw CorruptedLineException ();
        }
        // "variables" must contain object
        begin = end + 1;
        if (getNextObject (json, begin) != JT_Object) {
            throw CorruptedLineException ();
        }
        end = begin; // no need to add extra +1, as it's done in the loop
        while (true) {
            bool done = false;
            begin = end + 1;
            switch (getNextObject (json, begin)) {
                case JT_String:
                    key = readString (json, begin, end);
                    break;
                case JT_None:
                case JT_Object_End:
                    done = true;
                    break;
                case JT_Object:
                case JT_Invalid:
                default:
                    throw CorruptedLineException ();
                    break;
            }
            // detect whether there are no more variables
            if (done)
                break;
            key = "{{" + key + "}}";
            begin = end + 1;
            switch (getNextObject (json, begin)) {
                case JT_String:
                    // strings are direct values
                    value = readString (json, begin, end);
                    break;
                case JT_Object:
                    // objects may contain translations or special variables
                    value = getTranslatedText (order, readObject (json, begin, end)) ;
                    break;
                default:
                    throw CorruptedLineException ();
                    break;
            }
            size_t n = 0;
            while ( ( n = retval.find ( key, n ) ) != std::string::npos ) {
                retval.replace ( n, key.size (), value );
                n += value.size ();
            }
        }
    }
    return retval;
}


void replaceEscapedChars (std::string &target) {
    size_t n = 0;
    std::string key = "\\n";
    std::string value = "\n";
    while ( ( n = target.find ( key, n ) ) != std::string::npos ) {
        target.replace ( n, key.size (), value );
        n += value.size ();
    }
}


void Translation::loadLanguage (const std::string &language) {
    std::string filename = path_ + file_prefix_ + language + FILE_EXTENSION;
    log_debug ("Loading translation file '%s'", filename.c_str ());
    std::ifstream language_file (filename.c_str (), std::ios::in | std::ios::binary);
    if (language_file) {
        std::string line;
        while (std::getline (language_file, line) && line == "") {
            // skip empty lines
        }
        if (language_file.eof ()) {
            throw EmptyFileException ();
        }
        if (line != "{") {
            throw CorruptedLineException ();
        }
        language_list_ordering_.emplace (language, language_list_ordering_.size ());
        while (std::getline (language_file, line) && line != "}") {
            if (line == "") {
                // skip empty lines
                continue;
            }
            std::string key, value;
            size_t begin = 0, end = 0;
            // find key
            key = readString (line, begin, end);
            replaceEscapedChars (key);
            begin = end + 1;
            value = readString (line, begin, end);
            replaceEscapedChars (value);
            log_debug ("loaded [%s] => '%s'", key.c_str (), value.c_str ());
            language_translations_[key].push_back (value);
        }
        if (line != "}") {
            throw CorruptedLineException ();
        }
        // check if there are missing translations for loaded language
        for (auto &item : language_translations_) {
            if (language_list_ordering_.size () != item.second.size ()) {
                item.second.push_back (std::string ());
            }
        }
    } else {
        throw InvalidFileException ();
    }
    /* if you'd ever try to debug this and wonder about content of loaded translations, this might come handy
    std::cout << "Content of translations: ";
    for (auto x : language_translations_) {
    std::cout << "[" << x.first << "]=>{";
    for (auto y : x.second) {
    std::cout << y << ", ";
    }
    std::cout << "}, ";
    }
    std::cout << std::endl;
    */
}


Translation::Translation ()
    : language_order_ (-1)
    , agent_name_ ("") { }


Translation::~Translation () { }


void Translation::configure (const std::string &agent_name, const std::string &path, const std::string &file_prefix) {
    agent_name_ = agent_name;
    path_ = path;
    if (path_[path_.length () - 1] != '/') {
        path_ += '/';
    }
    file_prefix_ = file_prefix;
    loadLanguage (default_language_);
}


void Translation::changeLanguage (const std::string &language) {
    // check if language is present, and if not, reload it
    auto it = language_list_ordering_.find (language);
    if (it != language_list_ordering_.end ()) {
        // language is present, use it
        language_order_ = it->second;
    } else {
        // language is not present, load it
        loadLanguage (language);
        language_order_ = language_list_ordering_.size () - 1;
    }
}


int translation_initialize (const char *agent_name, const char *path, const char *file_prefix) {
    try {
        Translation::getInstance ().configure (agent_name, path, file_prefix);
    } catch (Translation::InvalidFileException &e) {
        return TE_InvalidFile;
    } catch (Translation::EmptyFileException &e) {
        return TE_EmptyFile;
    } catch (Translation::CorruptedLineException &e) {
        return TE_CorruptedLine;
    } catch (...) {
        return TE_Undefined;
    };
    return TE_OK;
}


int translation_change_language (const char *language) {
    try {
        Translation::getInstance ().changeLanguage (language);
    } catch (Translation::InvalidFileException &e) {
        return TE_InvalidFile;
    } catch (Translation::EmptyFileException &e) {
        return TE_EmptyFile;
    } catch (Translation::CorruptedLineException &e) {
        return TE_CorruptedLine;
    } catch (...) {
        return TE_Undefined;
    };
    return TE_OK;
}


char *translation_get_translated_text (const char *json) {
    if (NULL == json) {
        return NULL;
    }
    try {
        std::string tmp = Translation::getInstance ().getTranslatedText (json);
        char *retval = (char *) malloc (sizeof (char) * tmp.length () + 1);
        if (NULL == retval) {
            log_error ("Unable to allocate memory for translation C interface");
            return NULL;
        }
        memcpy (retval, tmp.c_str (), tmp.length () + 1);
        return retval;
    } catch (Translation::TranslationNotFoundException &e) {
        log_error ("Translation not found for '%s'", json);
        return NULL;
    } catch (Translation::CorruptedLineException &e) {
        log_error ("Translation json is corrupted: '%s'", json);
        return NULL;
    } catch (...) {
        log_error ("Undefined error in translation, possibly invalid json '%s'", json);
        return NULL;
    };
}


char *translation_get_translated_text_language (const TRANSLATION_CONFIGURATION *conf, const char *json) {
    if (NULL == json || NULL == conf) {
        return NULL;
    }
    try {
        std::string tmp = Translation::getInstance ().getTranslatedText (*conf, json);
        char *retval = (char *) malloc (sizeof (char) * tmp.length () + 1);
        if (NULL == retval) {
            log_error ("Unable to allocate memory for translation C interface");
            return NULL;
        }
        memcpy (retval, tmp.c_str (), tmp.length () + 1);
        return retval;
    } catch (Translation::TranslationNotFoundException &e) {
        log_error ("Translation not found for '%s'", json);
        return NULL;
    } catch (Translation::CorruptedLineException &e) {
        log_error ("Translation json is corrupted: '%s'", json);
        return NULL;
    } catch (Translation::LanguageNotLoadedException &e) {
        log_error ("Language '%s' is not loaded", conf->language);
        return NULL;
    } catch (...) {
        log_error ("Undefined error in translation, possibly invalid json '%s'", json);
        return NULL;
    };
}


//  --------------------------------------------------------------------------
//  Self test of this class

#define SELFTEST_DIR_RO "src/selftest-ro"
#define SELFTEST_DIR_RW "src/selftest-rw"

void
fty_common_translation_base_test (bool verbose)
{
    // most of the time just test C interface as it uses C++ one
    try {
        std::cout << " * fty_common_translation_base: " << std::endl;
        char *res;
        const char *path = SELFTEST_DIR_RW;
        const char *file_prefix = "test_";
        const char *lang1 = "en_US"; // default language, do not change
        const char *lang2 = "cs_CZ";
        const char *lang3 = "fr_FR";
        const char *agent_name = "translation_test";

        // test case 0 - initialization
        const char *empty_lang = "empty";
        const char *corrupted_lang = "corrupted";

        std::string cmd = std::string ("/bin/rm -f \'") + path + "/" + file_prefix + lang1 + FILE_EXTENSION "\'";
        assert (0 == system (cmd.c_str ()));
        assert (TE_InvalidFile == translation_initialize (agent_name, path, file_prefix));
        cmd = std::string ("/bin/cp --no-preserve=mode \'" SELFTEST_DIR_RO "/") + file_prefix + empty_lang + FILE_EXTENSION "\' \'" + path + "/" + file_prefix + lang1 + FILE_EXTENSION "\'";
        assert (0 == system (cmd.c_str ()));
        assert (TE_EmptyFile == translation_initialize (agent_name, path, file_prefix));
        cmd = std::string ("/bin/cp --no-preserve=mode \'" SELFTEST_DIR_RO "/") + file_prefix + corrupted_lang + FILE_EXTENSION "\' \'" + path + "/" + file_prefix + lang1 + FILE_EXTENSION "\'";
        assert (0 == system (cmd.c_str ()));
        assert (TE_CorruptedLine == translation_initialize (agent_name, path, file_prefix));
        // TE_Undefined shouldn't be returned under normal conditions
        cmd = std::string ("/bin/cp --no-preserve=mode \'" SELFTEST_DIR_RO "/") + file_prefix + lang1 + FILE_EXTENSION "\' \'" + path + "/" + file_prefix + lang1 + FILE_EXTENSION "\'";
        assert (0 == system (cmd.c_str ()));
        assert (TE_OK == translation_initialize (agent_name, path, file_prefix));

        // test case 1 - loading language
        cmd = std::string ("/bin/cp --no-preserve=mode \'" SELFTEST_DIR_RO "/") + file_prefix + lang1 + FILE_EXTENSION "\' \'" + path + "/" + file_prefix + lang1 + FILE_EXTENSION "\'";
        assert (0 == system (cmd.c_str ()));
        cmd = std::string ("/bin/cp --no-preserve=mode \'" SELFTEST_DIR_RO "/") + file_prefix + lang2 + FILE_EXTENSION "\' \'" + path + "/" + file_prefix + lang2 + FILE_EXTENSION "\'";
        assert (0 == system (cmd.c_str ()));
        const char *str_src = "{ \"key\" : \"first\"}";
        const char *str_exp_en = "first";
        const char *str_exp_cz = "první";
        assert (TE_OK == translation_change_language (lang1));
        res = translation_get_translated_text (str_src);
        assert (strcmp (res, str_exp_en) == 0);
        free (res);
        assert (TE_OK == translation_change_language (lang2));
        res = translation_get_translated_text (str_src);
        assert (strcmp (res, str_exp_cz) == 0);
        free (res);
        assert (TE_InvalidFile == translation_change_language (lang3));
        res = translation_get_translated_text (str_src);
        assert (strcmp (res, str_exp_cz) == 0);
        free (res);
        assert (TE_OK == translation_change_language (lang1));
        res = translation_get_translated_text (str_src);
        assert (strcmp (res, str_exp_en) == 0);
        free (res);

        // test case 2 - failed strings
        TRANSLATION_CONFIGURATION config = {(char *)"cs_CZ"};
        const char *str1_src = "not a valid json format";
        assert (NULL == translation_get_translated_text (str1_src));
        const char *str2_src = "{ corrupted }";
        assert (NULL == translation_get_translated_text (str2_src));
        const char *str3_src = "{}";
        assert (NULL == translation_get_translated_text (str3_src));
        const char *str4_src = "{ \"key\" }";
        assert (NULL == translation_get_translated_text (str4_src));
        const char *str5_src = "{ \"key\" :}";
        assert (NULL == translation_get_translated_text (str5_src));
        const char *str6_src = "{ \"key\" : \"\"}";
        assert (NULL == translation_get_translated_text (str6_src));
        const char *str7_src = "{ \"key\" : \"not found\"}";
        assert (NULL == translation_get_translated_text (str7_src));

        // test case 3 - good strings
        const char *str8_src = "{ \"key\" : \"first\"}";
        const char *str8_exp_en = "first";
        const char *str8_exp_cz = "první";
        res = translation_get_translated_text (str8_src);
        assert (strcmp (res, str8_exp_en) == 0);
        free (res);
        res = translation_get_translated_text (str8_src);
        assert (strcmp (res, str8_exp_en) == 0);
        free (res);
        res = translation_get_translated_text (str8_src);
        assert (strcmp (res, str8_exp_en) == 0);
        free (res);
        res = translation_get_translated_text_language (&config, str8_src);
        assert (strcmp (res, str8_exp_cz) == 0);
        free (res);
        const char *str9_src = "{ \"key\" : \"second\"}";
        const char *str9_exp_en = "second";
        const char *str9_exp_cz = "second";
        res = translation_get_translated_text (str9_src);
        assert (strcmp (res, str9_exp_en) == 0);
        free (res);
        res = translation_get_translated_text_language (&config, str9_src);
        assert (strcmp (res, str9_exp_cz) == 0);
        free (res);
        const char *str10_src = "{ \"key\" : \"third\", \"variables\" : { \"variable\" : \"var1\"}}";
        const char *str10_exp = "a string with a var1";
        res = translation_get_translated_text (str10_src);
        assert (strcmp (res, str10_exp) == 0);
        free (res);
        const char *str11_src = "{ \"key\" : \"fourth\", \"variables\" : { \"multiple\" : \"var1\", \"nextvar\" : \"var2\"}}";
        const char *str11_exp = "a string with var1 variables var2";
        res = translation_get_translated_text (str11_src);
        assert (strcmp (res, str11_exp) == 0);
        free (res);

        // test case 4 - advanced strings
        const char *str12_src = "{ \"key\" : \"fifth\", \"variables\" : { \"var1\" : \"v1\", \"var2\" : \"v2\"}}";
        const char *str12_exp_en = "reverse order string with v1 and v2 variables";
        const char *str12_exp_cz = "reverse order string with v2 and v1 variables";
        res = translation_get_translated_text (str12_src);
        assert (strcmp (res, str12_exp_en) == 0);
        free (res);
        res = translation_get_translated_text_language (&config, str12_src);
        assert (strcmp (res, str12_exp_cz) == 0);
        free (res);
        const char *str13_src = "{ \"key\" : \"sixth\", \"variables\" : { \"variable\" : \"var1\"}}";
        const char *str13_exp = "multiple instances of var1, var1, var1";
        res = translation_get_translated_text (str13_src);
        assert (strcmp (res, str13_exp) == 0);
        free (res);
        const char *str14_src = "{ \"key\" : \"seventh\" }";
        const char *str14_exp = "string without {{variable}} replacement";
        res = translation_get_translated_text (str14_src);
        assert (strcmp (res, str14_exp) == 0);
        free (res);
        const char *str15_src = "{ \"key\" : \"eleventh\", \"variables\" : { \"var1\" : { \"key\" : \"ninth\", \"variables\" : { \"variable\" : {\"key\" : \"eight\" } } }, \"var2\" : { \"key\" : \"tenth\" } } } }";
        const char *str15_exp = "outer string with middle string with innermost string and second innermost string";
        res = translation_get_translated_text (str15_src);
        assert (strcmp (res, str15_exp) == 0);
        free (res);
        const char *str16_src = " {\"key\":\"eleventh\",\"variables\":{\"var1\":{\"key\":\"ninth\",\"variables\":{\"variable\":{\"key\":\"eight\"}}},\"var2\":{\"key\":\"tenth\"}}}} ";
        const char *str16_exp = "outer string with middle string with innermost string and second innermost string";
        res = translation_get_translated_text (str16_src);
        assert (strcmp (res, str16_exp) == 0);
        free (res);
        const char *str17_src = " {\"key\":\"twelfth\nthirteenth\" }";
        const char *str17_exp = "string\nwith\nnewlines";
        res = translation_get_translated_text (str17_src);
        assert (strcmp (res, str17_exp) == 0);
        free (res);
    } catch (...) {
        log_error ("An exception was caught");
        assert (0);
    }
    //  @selftest
    //  Simple create/destroy test
    //  @end
    std::cout << "OK" << std::endl;
}
