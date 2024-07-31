/*  =========================================================================
    fty_common_translation - Singleton translation object

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

#include "fty_common_translation.h"
#include <fty_common.h>
#include <fty_log.h>

#include <cxxtools/string.h>
#include <cxxtools/utf8codec.h>
#include <cxxtools/serializationinfo.h>
#include <sstream>

#include <string>
#include <vector>

// replace any token by value in target
static void replaceToken(std::string& target, const std::string token, const std::string value)
{
    size_t n = 0;
    while ((n = target.find(token, n)) != std::string::npos) {
        target.replace(n, token.size(), value);
        n += value.size();
    }
}

static void replaceEscapedChars(std::string& target)
{
    replaceToken(target, "\\n", "\n");
}

// decode cxxtools::String to std::string (utf-8 support)
static std::string cxxStringToStdString(const cxxtools::String& cxs)
{
    std::ostringstream oss;
    oss << cxs;
    return oss.str();
}

std::string Translation::getTranslatedText(const std::string& json)
{
    return getTranslatedText(language_order_, json);
}

std::string Translation::getTranslatedText(const TRANSLATION_CONFIGURATION& conf, const std::string& json)
{
    auto order_it = language_list_ordering_.find(conf.language);
    if (order_it == language_list_ordering_.end()) {
        log_debug("LanguageNotLoadedException");
        throw LanguageNotLoadedException();
    }
    return getTranslatedText(order_it->second, json);
}

std::string Translation::getTranslatedText(const size_t order, const std::string& json, size_t depth)
{
    const bool verbose{false};

    if (verbose) { log_debug("getTranslatedText: order: %lu, json: '%s'", order, json.c_str()); }

    // assume json is formatted as: { "key": "<key>", "variables": {...} }
    cxxtools::SerializationInfo si;
    try {
        JSON::readFromString(json, si);
    }
    catch (const std::exception& e) {
        log_error("Exception reached: %s", e.what());
        throw CorruptedLineException();
    }

    // read key
    std::string key;
    {
        auto p = si.findMember("key");
        if (!p || p->isNull()) {
            log_error("Member 'key' not found");
            throw CorruptedLineException();
        }
        if (p->category() != cxxtools::SerializationInfo::Category::Value) {
            log_error("Invalid 'key' format");
            throw CorruptedLineException();
        }
        *p >>= key;

        if (key.empty()) {
            log_error("Member 'key' is empty");
            throw CorruptedLineException();
        }
    }
    if (verbose) { log_debug("key: '%s'", key.c_str()); }

    // read variables (optional)
    std::map<std::string, std::string> variables;
    if (auto p = si.findMember("variables"); p && !p->isNull()) {
        for (auto it = p->begin(); it != p->end(); ++it) {
            bool found{false};
            std::string value;
            if (it->category() == cxxtools::SerializationInfo::Category::Value) {
                // "<name>" : "<value>"
                cxxtools::String cxs;
                *it >>= cxs;
                value = cxxStringToStdString(cxs); // utf8
                found = true;
            }
            else if (it->category() == cxxtools::SerializationInfo::Category::Object) {
                if (auto p1 = it->findMember("value"); p1 && !p1->isNull()) {
                    // "<name>" : { "value": "<value>" }
                    cxxtools::String cxs;
                    *p1 >>= cxs;
                    value = cxxStringToStdString(cxs); // utf8
                    found = true;
                }
                else if (p1 = it->findMember("key"); p1 && !p1->isNull()) {
                    // recursive key variables scheme (variables part reference a key)
                    // "<name>" : { "key": "<key>", "variables": {...} }
                    try {
                        if (depth < 3) { // secured
                            const std::string jsonKey = JSON::writeToString(*it);
                            value = getTranslatedText(order, jsonKey, depth + 1);
                            found = true;
                        }
                        else {
                            log_warning("limit recursive key variables scheme (depth: %lu)", depth);
                        }
                    }
                    catch (...) {
                        log_error("Exception reached"); // ignore & continue...
                    }
                }
            }

            if (found) {
                variables[it->name()] = value;
            }
        }
    }

    // find translation text matching key
    std::string text;
    {
        auto mapping = language_translations_.find(key);
        if (mapping == language_translations_.end()) {
            throw TranslationNotFoundException();
        }
        text = mapping->second.at(order);
        if (text.empty() && (order != 0)) {
            text = mapping->second.at(0); // fallback to default language
        }
    }
    if (verbose) { log_debug("text: '%s'", text.c_str()); }

    // replace variables in text
    for (const auto& v : variables) {
        if (verbose) { log_debug("variable '%s': '%s'", v.first.c_str(), v.second.c_str()); }
        replaceToken(text, "{{" + v.first + "}}", v.second);
    }

    return text; // translation text
}

// read all key/value from a localization file
// set language_translations_ map
void Translation::loadLanguage(const std::string& language)
{
    const bool verbose{false};
    const bool verboseAll{false};

    const std::string filename = path_ + file_prefix_ + language + ".json";
    log_debug("Loading translation file '%s'", filename.c_str());

    cxxtools::SerializationInfo si;
    try {
        JSON::readFromFile(filename, si);
    }
    catch (const std::exception& e) {
        log_error("%s: %s (%s)", filename.c_str(), strerror(errno), e.what());
        throw InvalidFileException();
    }

    bool first{true};
    size_t line = 0;
    for (auto it = si.begin(); it != si.end(); ++it) {
        line++;
        if (it->category() != cxxtools::SerializationInfo::Category::Value) {
            log_error("%s: format is invalid (line: %lu)", filename.c_str(), line);
            throw InvalidFileException();
        }

        std::string key{it->name()};
        std::string value; // utf8
        {
            cxxtools::String cxxstr;
            *it >>= cxxstr;
            value = cxxStringToStdString(cxxstr);
        }

        if (value.empty()) { value = key; }
        replaceEscapedChars(key);
        replaceEscapedChars(value);

        if (verbose) { log_debug ("loaded [%s] => '%s'", key.c_str (), value.c_str ()); }

        if (first) {
            first = false;
            language_list_ordering_.emplace(language, language_list_ordering_.size());
        }

        language_translations_[key].push_back(value);
    }

    if (first) {
        log_error("%s: no key defined", filename.c_str());
        throw EmptyFileException();
    }

    log_debug("%s: %lu keys defined", filename.c_str(), line);

    // check if there are missing translations for loaded language
    for (auto& item : language_translations_) {
        if (language_list_ordering_.size() != item.second.size()) {
            item.second.push_back(std::string());
        }
    }

    if (verboseAll) {
        // content of loaded translations
        log_debug("=== Content of translations:");;
        for (auto x : language_translations_) {
            std::string xsec;
            for (auto y : x.second) {
                xsec += std::string(xsec.empty() ? "" : ", ") + "'" + y + "'";
            }
            log_debug("[%s] => {%s}", x.first.c_str(), xsec.c_str());
        }
    }
}

void Translation::configure(const std::string& agent_name, const std::string& path, const std::string& file_prefix)
{
    agent_name_ = agent_name;
    path_ = path;
    file_prefix_ = file_prefix;
    language_order_ = 0;

    if ((path_.length() > 0) && (path_[path_.length() - 1] != '/')) {
        path_ += '/';
    }

    loadLanguage(default_language_);
}

void Translation::changeLanguage(const std::string& language)
{
    // check if language is present, and if not, load it
    auto it = language_list_ordering_.find(language);
    if (it != language_list_ordering_.end()) {
        // language is present, use it
        language_order_ = it->second;
    }
    else {
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
    } catch (...) {
        return TE_Undefined;
    }
    return TE_OK;
}

char* translation_get_translated_text(const char* json)
{
    if (!json) {
        return nullptr;
    }

    try {
        std::string tmp = Translation::getInstance().getTranslatedText(json);
        return strdup(tmp.c_str());
    } catch (Translation::TranslationNotFoundException&) {
        log_error("Translation not found for '%s'", json);
    } catch (Translation::CorruptedLineException&) {
        log_error("Translation json is corrupted: '%s'", json);
    } catch (...) {
        log_error("Undefined error in translation, possibly invalid json '%s'", json);
    }

    return nullptr;
}

char* translation_get_translated_text_language(const TRANSLATION_CONFIGURATION* conf, const char* json)
{
    if (!(json && conf)) {
        return nullptr;
    }

    try {
        std::string tmp = Translation::getInstance().getTranslatedText(*conf, json);
        return strdup(tmp.c_str());
    } catch (Translation::TranslationNotFoundException&) {
        log_error("Translation not found for '%s'", json);
    } catch (Translation::CorruptedLineException&) {
        log_error("Translation json is corrupted: '%s'", json);
    } catch (Translation::LanguageNotLoadedException&) {
        log_error("Language '%s' is not loaded", conf->language);
    } catch (...) {
        log_error("Undefined error in translation, possibly invalid json '%s'", json);
    }

    return nullptr;
}
