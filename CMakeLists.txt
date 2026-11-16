cmake_minimum_required(VERSION 3.20)
file(STRINGS VERSION APP_VERSION LIMIT_COUNT 1)
project(cryptoapp VERSION ${APP_VERSION} LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)

add_library(cryptocore STATIC src/core.c)
target_include_directories(cryptocore PUBLIC src)
target_link_libraries(cryptocore PUBLIC PkgConfig::SODIUM)

add_executable(cryptoapp src/main.c)
target_compile_definitions(cryptoapp PRIVATE APP_VERSION="${PROJECT_VERSION}")
target_link_libraries(cryptoapp PRIVATE cryptocore)

install(TARGETS cryptoapp RUNTIME DESTINATION bin)
install(FILES packaging/cryptoapp.conf DESTINATION /etc/cryptoapp)

# Подключение тестирования
include(CTest)
if(BUILD_TESTING)
    add_executable(test_core tests/test_core.c)
    target_link_libraries(test_core PRIVATE cryptocore)
    add_test(NAME core COMMAND test_core)

    add_test(NAME cli_version COMMAND cryptoapp --version)
    set_tests_properties(cli_version PROPERTIES PASS_REGULAR_EXPRESSION "${PROJECT_VERSION}")

    add_test(NAME cli_bad_args COMMAND cryptoapp --no-such-option)
    set_tests_properties(cli_bad_args PROPERTIES WILL_FAIL TRUE)
endif()
