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

#include "fty_common_translation_classes.h"


#define VARIABLES "variables"
#define FILE_EXTENSION ".json"

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}


// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}


std::string Translation::readString(const std::string &line, size_t &start_pos, size_t &end_pos) {
    int temp = 0;
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
    std::string key, value, retval;
    size_t begin = 0, end = 0;
    key = readString (json, begin, end);
    begin = end + 1;
    value = readString (json, begin, end);
    auto mapping = language_translations_.find(key);
    if (language_translations_.end() == mapping) {
        throw TranslationNotFoundException();
    }
    retval = mapping->second.at (language_order_);
    // load variables if present
    try {
        begin = end + 1;
        key = readString (json, begin, end);
        if (!json.find (VARIABLES, begin)) {
            throw CorruptedLineException();
        }
        while (true) {
            begin = end + 1;
            key = readString (json, begin, end);
            begin = end + 1;
            value = readString (json, begin, end);
            size_t n = 0;
            while ( ( n = retval.find( key, n ) ) != std::string::npos ) {
                retval.replace( n, key.size(), value );
                n += value.size();
            }
        }
    } catch (NotFoundException &e) {
        // this is quite OK, there is no need to have variables
    }
    return retval;
}


void Translation::loadLanguage (const std::string language) {
    std::string filename = path_ + file_prefix_ + language + FILE_EXTENSION;
    std::ifstream language_file (filename.c_str (), std::ios::in | std::ios::binary);
    if (language_file) {
        std::string line;
        getline (language_file, line);
        if (line != "{") {
            throw CorruptedLineException ();
        }
        language_list_ordering_.emplace (language, language_list_ordering_.size());
        while (getline (language_file, line) && line != "}") {
            std::string key, value;
            size_t begin = 0, end = 0;
            // find key
            key = readString(line, begin, end);
            begin = end + 1;
            value = readString(line, begin, end);
            log_debug ("loaded [%s] => '%s'", key.c_str (), value.c_str ());
            language_translations_[key].push_back (value);
        }
        if (line != "}") {
            throw CorruptedLineException ();
        }
        // check if there are missing translations for loaded language
        for (auto item : language_translations_) {
            if (language_list_ordering_.size () != item.second.size ()) {
                item.second.push_back ("");
            }
        }
    } else {
        throw InvalidFileException ();
    }
}


Translation::Translation ()
    : language_order_ (-1)
    , agent_name_ ("") { }


Translation::~Translation () { };


void Translation::configure (const std::string agent_name, const std::string path, const std::string file_prefix) {
    agent_name_ = agent_name;
    path_ = path;
    if (path_[path_.length() - 1] != '/') {
        path_ += '/';
    }
    file_prefix_ = file_prefix;
    loadLanguage (default_language_);
}


void Translation::changeLanguage (const std::string language) {
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


int translation_initialize (const char *agent_name, const char *path, char *file_prefix) {
    try {
        Translation::getInstance().configure (agent_name, path, file_prefix);
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
        char *retval = (char *) malloc (sizeof (char) * tmp.length ());
        if (NULL == retval) {
            log_error ("Unable to allocate memory for translation C interface");
            return NULL;
        }
        memcpy(retval, tmp.c_str(), tmp.length() + 1);
        return retval;
    } catch (Translation::TranslationNotFoundException &e) {
        log_error ("Translation not found for '%s'", *json);
        return NULL;
    } catch (Translation::CorruptedLineException &e) {
        log_error ("Translation json is corrupted: '%s'", json);
        return NULL;
    } catch (...) {
        log_error ("Undefined error in translation");
        return NULL;
    };
}


char *translation_get_translated_text_language (const TRANSLATION_CONFIGURATION *conf, const char *json) {
    if (NULL == json || NULL == conf) {
        return NULL;
    }
    try {
        std::string tmp = Translation::getInstance ().getTranslatedText (*conf, json);
        char *retval = (char *) malloc (sizeof (char) * tmp.length ());
        if (NULL == retval) {
            log_error ("Unable to allocate memory for translation C interface");
            return NULL;
        }
        memcpy(retval, tmp.c_str(), tmp.length() + 1);
        return retval;
    } catch (Translation::TranslationNotFoundException &e) {
        log_error ("Translation not found for '%s'", *json);
        return NULL;
    } catch (Translation::CorruptedLineException &e) {
        log_error ("Translation json is corrupted: '%s'", json);
        return NULL;
    } catch (Translation::LanguageNotLoadedException &e) {
        log_error ("Language '%s' is not loaded", conf->language);
        return NULL;
    } catch (...) {
        log_error ("Undefined error in translation");
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
    std::cout << " * fty_common_translation_base: " << std::endl;
    std::string path = SELFTEST_DIR_RO;
    std::string file_prefix = "test_";
    std::string lang1 = "en_US";
    std::string lang2 = "cs_CZ";
    std::string lang3 = "fr_FR";

    //  @selftest
    //  Simple create/destroy test
    //  @end
    printf ("OK\n");
}
