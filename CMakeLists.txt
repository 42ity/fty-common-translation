cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

########################################################################################################################

project(fty_common_translation
    VERSION 1.0.0
    DESCRIPTION "Provides common translation library"
)

########################################################################################################################
find_package(fty-cmake PATHS ${CMAKE_BINARY_DIR}/fty-cmake)
########################################################################################################################

etn_target(shared ${PROJECT_NAME}
    PUBLIC_INCLUDE_DIR include
    PUBLIC
        fty_common_translation_base.h
        fty_common_translation.h
    SOURCES
        src/fty_common_translation_base.cc
    USES
        cxxtools
        fty_common
        fty_common_logging
)

########################################################################################################################

etn_test_target(${PROJECT_NAME}
    CONFIGS
        test/data/test_corrupted_en_US.json
        test/data/test_cs_CZ.json
        test/data/test_empty_en_US.json
        test/data/test_en_US.json
    SOURCES
        test/fty_common_translation_base.cc
        test/main.cpp
    USES
        pthread
    SUBDIR
        test
)

########################################################################################################################

install(PROGRAMS
        collect_translations.sh
        translations_to_weblate.sh
        translations_to_weblate.awk
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
########################################################################################################################

