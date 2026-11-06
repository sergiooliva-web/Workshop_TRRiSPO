cmake_minimum_required(VERSION 3.20)

# Читаем версию из файла VERSION
file(STRINGS VERSION APP_VERSION LIMIT_COUNT 1)
project(crypto_tool VERSION ${APP_VERSION} LANGUAGES C)

# Стандарт C11 и строгие флаги (требование методички: без варнингов)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

# Находим libsodium через PkgConfig
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSODIUM REQUIRED IMPORTED_TARGET libsodium)

# Собираем модуль core в статическую библиотеку
add_library(cryptocore STATIC src/core.c)
target_include_directories(cryptocore PUBLIC src)
target_link_libraries(cryptocore PUBLIC PkgConfig::LIBSODIUM)

# Собираем исполняемый файл программы
add_executable(crypto_tool src/main.c)
# Передаем версию программы внутрь C-кода
target_compile_definitions(crypto_tool PRIVATE APP_VERSION="${PROJECT_VERSION}")
target_link_libraries(crypto_tool PRIVATE cryptocore)

# Настройка для инсталляции (пригодится для сборки deb-пакета)
install(TARGETS crypto_tool RUNTIME DESTINATION bin)
