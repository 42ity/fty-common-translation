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
#include <catch2/catch.hpp>

using namespace std::literals;

static std::string translate(
    const std::string& input, const std::optional<TRANSLATION_CONFIGURATION>& config = std::nullopt)
{
    if (config == std::nullopt) {
        return Translation::getInstance().getTranslatedText(input);
    } else {
        return Translation::getInstance().getTranslatedText(*config, input);
    }
}

TEST_CASE("Translation base")
{
    REQUIRE_THROWS_AS(Translation::getInstance().configure("translation_test", "test/data", "test_wrong_"),
        Translation::InvalidFileException);
    REQUIRE_THROWS_AS(Translation::getInstance().configure("translation_test", "test/data", "test_empty_"),
        Translation::EmptyFileException);
//    REQUIRE_THROWS_AS(Translation::getInstance().configure("translation_test", "test/data", "test_corrupted_"),
//        Translation::CorruptedLineException);

    // test case 1 - loading language
    REQUIRE_NOTHROW(Translation::getInstance().configure("translation_test", "test/data", "test_"));

    {
        static std::string input = R"({ "key" : "first"})";
        std::string        res;

        CHECK(TE_OK == translation_change_language("en_US"));
        CHECK_NOTHROW(res = translate(input));
        CHECK(res == "first"s);

        CHECK(TE_OK == translation_change_language("cs_CZ"));
        CHECK_NOTHROW(res = translate(input));
        CHECK(res == "první"s);

        CHECK(TE_InvalidFile == translation_change_language("fr_FR"));
        CHECK_NOTHROW(res = translate(input));
        CHECK(res == "první"s);

        CHECK(TE_OK == translation_change_language("en_US"));
        CHECK_NOTHROW(res = translate(input));
        CHECK(res == "first"s);
    }

    // test case 2 - failed strings
    {
        CHECK_THROWS(translate("not a valid json format"));
        CHECK_THROWS(translate("{ corrupted }"));
        CHECK_THROWS(translate("{}"));
        CHECK_THROWS(translate("{ \"key\" }"));
        CHECK_THROWS(translate("{ \"key\" :}"));
        CHECK_THROWS(translate("{ \"key\" : \"\"}"));
        CHECK_THROWS(translate("{ \"key\" : \"not found\"}"));
    }

    TRANSLATION_CONFIGURATION config = {const_cast<char*>("cs_CZ")};

    // test case 3 - good strings
    {
        std::string res;
        {
            static std::string input = R"({"key" : "first"})";

            CHECK_NOTHROW(res = translate(input));
            CHECK(res == "first"s);

            CHECK_NOTHROW(res = translate(input, config));
            CHECK(res == "první"s);
        }

        {
            static std::string input = R"({"key" : "second"})";

            CHECK_NOTHROW(res = translate(input));
            CHECK(res == "second"s);

            CHECK_NOTHROW(res = translate(input, config));
            CHECK(res == "second"s);
        }

        {
            static std::string input = R"({"key" : "third", "variables" : { "variable" : "var1"}})";
            CHECK_NOTHROW(res = translate(input));
            CHECK(res == "a string with a var1"s);
        }

        {
            static std::string input =
                R"({"key" : "fourth", "variables" : { "multiple" : "var1", "nextvar" : "var2"}})";
            CHECK_NOTHROW(res = translate(input));
            CHECK(res == "a string with var1 variables var2"s);
        }
    }

    // test case 4 - advanced strings
    {
        std::string res;
        CHECK_NOTHROW(
            res = translate(R"({"key" : "fourth", "variables" : { "multiple" : "var1", "nextvar" : "var2"}})"));

        {
            static std::string input = R"({ "key" : "fifth", "variables" : { "var1" : "v1", "var2" : "v2" }})";
            CHECK_NOTHROW(res = translate(input));
            CHECK(res == "reverse order string with v1 and v2 variables"s);

            CHECK_NOTHROW(res = translate(input, config));
            CHECK(res == "reverse order string with v2 and v1 variables"s);
        }

        {
            static std::string input = R"({"key" : "sixth", "variables" : { "variable" : "var1"}})";
            CHECK_NOTHROW(res = translate(input));
            CHECK(res == "multiple instances of var1, var1, var1"s);
        }

        {
            CHECK_NOTHROW(res = translate(R"({"key" : "seventh" })"));
            CHECK(res == "string without {{variable}} replacement"s);
        }

        {
            static std::string input = R"({ "key" : "eleventh", "variables" : { "var1" : { "key" : "ninth", "variables" : { "variable" : { "key" : "eight" }}}, "var2" : {"key" : "tenth"}}})";
            CHECK_NOTHROW(res = translate(input));
            CHECK(res == "outer string with middle string with innermost string and second innermost string"s);
        }

        {
            static std::string input = R"({"key" : "eleventh", "variables": { "var1": {"key":"ninth", "variables": { "variable": {"key": "eight"}}}, "var2": {"key": "tenth"}}})";
            CHECK_NOTHROW(res = translate(input));
            CHECK(res == "outer string with middle string with innermost string and second innermost string"s);
        }

        {
            CHECK_NOTHROW(res = translate("{\"key\":\"twelfth\nthirteenth\" }"));
            CHECK(res == "string\nwith\nnewlines"s);
        }

        {
            static std::string input = R"b({"key" : "TRANSLATE_LUA(Phase imbalance in datacenter {{ename}} is high.)", "variables" : {"ename" : {"value" : "DC-Roztoky", "assetLink" : "datacenter-3"}}})b";
            CHECK_NOTHROW(res = translate(input));
            CHECK(res == "Phase imbalance in datacenter DC-Roztoky is high."s);
        }
    }
}
