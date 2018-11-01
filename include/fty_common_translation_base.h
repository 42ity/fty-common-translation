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

#ifndef FTY_COMMON_TRANSLATION_BASE_H_INCLUDED
#define FTY_COMMON_TRANSLATION_BASE_H_INCLUDED

#include <fty_common_json.h>

typedef struct {
    char *language;
} TRANSLATION_CONFIGURATION;

typedef enum {
    TE_OK = 0,
    TE_Undefined = INT_MIN,
    TE_InvalidFile,
    TE_EmptyFile,
    TE_CorruptedLine,
    TE_LanguageNotLoaded,
    TE_TranslationNotFound,
    TE_NotFound
} TRANSLATION_CRETVALS;

#ifdef __cplusplus

#include <string>
#include <vector>

class Translation {
    public:
        static Translation& getInstance ()
        {
            static Translation instance;
            return instance;
        }
    private:
        const std::string default_language_="en_US";
        // language order stored for getting current language from language_list_ordering
        size_t language_order_;
        // preloaded translation strings in map: "key" -> language list [use language_order as a key]
        std::map<std::string, std::vector<std::string>> language_translations_;
        // pairing language string to index with default en_US: "en_US" -> 0, ...
        std::map<std::string, size_t> language_list_ordering_;
        // store agent name for malamute communication
        std::string agent_name_;
        // store prefix for translation files
        std::string file_prefix_;
        // store path to translation files
        std::string path_;
        // avoid use of the following procedures/functions as this should be a singleton
        Translation ();
        ~Translation ();
        // load language to structures, throws errors in case of failure
        void loadLanguage (const std::string &language);
        // get translated text inner function
        std::string getTranslatedText (const size_t order, const std::string &json);
    public:
        // singleton, deleted functions should be public for better error handling
        Translation (const Translation&) = delete;
        Translation& operator= (const Translation&) = delete;
        // prepare configuration
        void configure (const std::string &agent_name, const std::string &path, const std::string &file_prefix);
        // change default used language
        void changeLanguage (const std::string &language);
        // get translated text from selected language
        std::string getTranslatedText (const std::string &json);
        std::string getTranslatedText (const TRANSLATION_CONFIGURATION &conf, const std::string &json);
        class InvalidFileException {};
        class EmptyFileException {};
        class CorruptedLineException {};
        class LanguageNotLoadedException {};
        class TranslationNotFoundException {};
        class NotFoundException {};
};

extern "C" {
#endif

// Wrapper for initialization
int translation_initialize (const char *agent_name, const char *path, const char *file_prefix);

// Wrapper for changing language
int translation_change_language (const char *language);

// Wrapper for getting translated text
char *translation_get_translated_text (const char *json);

// Wrapper for getting translated text
char *translation_get_translated_text_language (const TRANSLATION_CONFIGURATION *conf, const char *json);

//  Self test of this class
FTY_COMMON_TRANSLATION_EXPORT void
    fty_common_translation_base_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
