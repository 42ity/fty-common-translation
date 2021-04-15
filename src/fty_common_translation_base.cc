/*  =========================================================================
    fty_common_translation_base - Singleton translation object

    Copyright (C) 2014 - 2020 Eaton

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

#include "fty_common_translation_base.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <fty_log.h>

#define VARIABLES      "variables"
#define VARIABLE       "variable"
#define KEY            "key"
#define VALUE          "value"
#define FILE_EXTENSION ".json"

std::string Translation::getTranslatedText(const std::string& json)
{
    return getTranslatedText(language_order_, json);
}


std::string Translation::getTranslatedText(const TRANSLATION_CONFIGURATION& conf, const std::string& json)
{
    auto order_it = language_list_ordering_.find(conf.language);
    if (order_it == language_list_ordering_.end()) {
        throw LanguageNotLoadedException();
    }
    return getTranslatedText(order_it->second, json);
}


std::string Translation::getTranslatedText(const size_t order, const std::string& json)
{
    // TODO add handling of special variables that might be just formated, such as { "variable" : "IPC 2000", "link":
    // "http://42ity.org/" }
    std::string key, value, retval;
    size_t      begin = 0, end = 0;
    // read basic "key" : "translation_key" pair and validate
    begin = json.find_first_not_of("\t ");
    if (json.at(begin) != '{') {
        throw CorruptedLineException();
    }
    if (json.at(json.find_last_not_of("\t ")) != '}') {
        throw CorruptedLineException();
    }
    ++begin;
    if (JSON::getNextObject(json, begin) == JT_String) {
        key = JSON::readString(json, begin, end);
    } else {
        throw CorruptedLineException();
    }
    begin = end + 1;
    if (JSON::getNextObject(json, begin) == JT_String) {
        value = JSON::readString(json, begin, end);
    } else {
        throw CorruptedLineException();
    }

    // handle special key 'value' inside ENAME
    if (key.find(VALUE) != std::string::npos) {
        return value;
    }
    // check for "key" keyword
    if (key.find(KEY) == std::string::npos && key.find(VARIABLE) == std::string::npos) {
        throw CorruptedLineException();
    }
    if (key.find(VARIABLE) != std::string::npos) {
        log_error("Unexpected input '%s', handling of \"variable\" is not implemented", json.c_str());
        throw std::logic_error("Not implemented");
    }
    // find translation string matching translation_key
    auto mapping = language_translations_.find(value);
    if (language_translations_.end() == mapping) {
        throw TranslationNotFoundException();
    }
    retval = mapping->second.at(order);
    if (retval.empty()) {
        // fallback to default language
        retval = mapping->second.at(0);
    }
    // load variables if present
    begin = end + 1;
    if (JSON::getNextObject(json, begin) == JT_String) {
        key = JSON::readString(json, begin, end);
        // check for "variables" keyword
        if (json.find(VARIABLES, begin) == std::string::npos) {
            throw CorruptedLineException();
        }
        // "variables" must contain object
        begin = end + 1;
        if (JSON::getNextObject(json, begin) != JT_Object) {
            throw CorruptedLineException();
        }
        end = begin; // no need to add extra +1, as it's done in the loop
        while (true) {
            bool done = false;
            begin     = end + 1;
            switch (JSON::getNextObject(json, begin)) {
                case JT_String:
                    key = JSON::readString(json, begin, end);
                    break;
                case JT_None:
                case JT_Object_End:
                    done = true;
                    break;
                case JT_Object:
                case JT_Invalid:
                    throw CorruptedLineException();
            }
            // detect whether there are no more variables
            if (done)
                break;
            key   = "{{" + key + "}}";
            begin = end + 1;
            switch (JSON::getNextObject(json, begin)) {
                case JT_String:
                    // strings are direct values
                    value = JSON::readString(json, begin, end);
                    break;
                case JT_Object:
                    // objects may contain translations or special variables
                    value = getTranslatedText(order, JSON::readObject(json, begin, end));
                    break;
                case JT_Invalid:
                case JT_None:
                case JT_Object_End:
                    throw CorruptedLineException();
            }
            size_t n = 0;
            while ((n = retval.find(key, n)) != std::string::npos) {
                retval.replace(n, key.size(), value);
                n += value.size();
            }
        }
    }
    return retval;
}


static void replaceEscapedChars(std::string& target)
{
    size_t      n     = 0;
    std::string key   = "\\n";
    std::string value = "\n";
    while ((n = target.find(key, n)) != std::string::npos) {
        target.replace(n, key.size(), value);
        n += value.size();
    }
}


void Translation::loadLanguage(const std::string& language)
{
    std::string filename = path_ + file_prefix_ + language + FILE_EXTENSION;
    log_debug("Loading translation file '%s'", filename.c_str());
    std::ifstream language_file(filename.c_str(), std::ios::in | std::ios::binary);
    if (language_file) {
        std::string line;
        while (std::getline(language_file, line) && line == "") {
            // skip empty lines
        }
        if (language_file.eof()) {
            throw EmptyFileException();
        }
        if (line != "{") {
            throw CorruptedLineException();
        }
        language_list_ordering_.emplace(language, language_list_ordering_.size());
        while (std::getline(language_file, line) && line != "}") {
            if (line == "") {
                // skip empty lines
                continue;
            }
            std::string key, value;
            size_t      begin = 0, end = 0;
            // find key
            key = JSON::readString(line, begin, end);
            replaceEscapedChars(key);
            begin = end + 1;
            value = JSON::readString(line, begin, end);
            replaceEscapedChars(value);
            // NOTE: keep this for debugging purposes, just comment it out
            // log_debug ("loaded [%s] => '%s'", key.c_str (), value.c_str ());
            language_translations_[key].push_back(value);
        }
        if (line != "}") {
            throw CorruptedLineException();
        }
        // check if there are missing translations for loaded language
        for (auto& item : language_translations_) {
            if (language_list_ordering_.size() != item.second.size()) {
                item.second.push_back(std::string());
            }
        }
    } else {
        throw InvalidFileException();
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


Translation::Translation()
    : language_order_(size_t(-1))
    , agent_name_("")
{
}


Translation::~Translation()
{
}


void Translation::configure(const std::string& agent_name, const std::string& path, const std::string& file_prefix)
{
    agent_name_ = agent_name;
    path_       = path;
    if (path_[path_.length() - 1] != '/') {
        path_ += '/';
    }
    file_prefix_ = file_prefix;
    loadLanguage(default_language_);
    language_order_ = 0;
}


void Translation::changeLanguage(const std::string& language)
{
    // check if language is present, and if not, reload it
    auto it = language_list_ordering_.find(language);
    if (it != language_list_ordering_.end()) {
        // language is present, use it
        language_order_ = it->second;
    } else {
        // language is not present, load it
        loadLanguage(language);
        language_order_ = language_list_ordering_.size() - 1;
    }
}


int translation_initialize(const char* agent_name, const char* path, const char* file_prefix)
{
    try {
        Translation::getInstance().configure(agent_name, path, file_prefix);
    } catch (Translation::InvalidFileException&) {
        return TE_InvalidFile;
    } catch (Translation::EmptyFileException&) {
        return TE_EmptyFile;
    } catch (Translation::CorruptedLineException&) {
        return TE_CorruptedLine;
    } catch (JSON::CorruptedLineException&) {
        return TE_CorruptedLine;
    } catch (...) {
        return TE_Undefined;
    }
    return TE_OK;
}


int translation_change_language(const char* language)
{
    try {
        Translation::getInstance().changeLanguage(language);
    } catch (Translation::InvalidFileException&) {
        return TE_InvalidFile;
    } catch (Translation::EmptyFileException&) {
        return TE_EmptyFile;
    } catch (Translation::CorruptedLineException&) {
        return TE_CorruptedLine;
    } catch (JSON::CorruptedLineException&) {
        return TE_CorruptedLine;
    } catch (...) {
        return TE_Undefined;
    }
    return TE_OK;
}


char* translation_get_translated_text(const char* json)
{
    if (nullptr == json) {
        return nullptr;
    }

    try {
        std::string tmp    = Translation::getInstance().getTranslatedText(json);
        char*       retval = static_cast<char*>(malloc(sizeof(char) * tmp.length() + 1));
        if (nullptr == retval) {
            log_error("Unable to allocate memory for translation C interface");
            return nullptr;
        }
        memcpy(retval, tmp.c_str(), tmp.length() + 1);
        return retval;
    } catch (Translation::TranslationNotFoundException&) {
        log_error("Translation not found for '%s'", json);
        return nullptr;
    } catch (Translation::CorruptedLineException&) {
        log_error("Translation json is corrupted: '%s'", json);
        return nullptr;
    } catch (JSON::CorruptedLineException&) {
        log_error("Translation json is corrupted: '%s'", json);
        return nullptr;
    } catch (...) {
        log_error("Undefined error in translation, possibly invalid json '%s'", json);
        return nullptr;
    }
}


char* translation_get_translated_text_language(const TRANSLATION_CONFIGURATION* conf, const char* json)
{
    if (nullptr == json || nullptr == conf) {
        return nullptr;
    }

    try {
        std::string tmp    = Translation::getInstance().getTranslatedText(*conf, json);
        char*       retval = static_cast<char*>(malloc(sizeof(char) * tmp.length() + 1));
        if (nullptr == retval) {
            log_error("Unable to allocate memory for translation C interface");
            return nullptr;
        }
        memcpy(retval, tmp.c_str(), tmp.length() + 1);
        return retval;
    } catch (Translation::TranslationNotFoundException&) {
        log_error("Translation not found for '%s'", json);
        return nullptr;
    } catch (Translation::CorruptedLineException&) {
        log_error("Translation json is corrupted: '%s'", json);
        return nullptr;
    } catch (JSON::CorruptedLineException&) {
        log_error("Translation json is corrupted: '%s'", json);
        return nullptr;
    } catch (Translation::LanguageNotLoadedException&) {
        log_error("Language '%s' is not loaded", conf->language);
        return nullptr;
    } catch (...) {
        log_error("Undefined error in translation, possibly invalid json '%s'", json);
        return nullptr;
    }
}
