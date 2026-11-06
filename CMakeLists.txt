cmake_minimum_required(VERSION 3.20)

# Читаем версию из файла VERSION
file(STRINGS VERSION APP_VERSION LIMIT_COUNT 1)
project(crypto_tool VERSION ${APP_VERSION} LANGUAGES C)

# Стандарт C11 и строгие флаги (требование методички: сборка без предупреждений)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

# Находим libsodium через PkgConfig (так как стандартного find_package для нее нет)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSODIUM REQUIRED IMPORTED_TARGET libsodium)

# Собираем модуль core в статическую библиотеку (для будущих тестов в ПР 2)
add_library(cryptocore STATIC src/core.c)
target_include_directories(cryptocore PUBLIC src)
target_link_libraries(cryptocore PUBLIC PkgConfig::LIBSODIUM)

# Собираем основной исполняемый файл программы
add_executable(crypto_tool src/main.c)
# Передаем макрос версии внутрь C-кода из CMake
target_compile_definitions(crypto_tool PRIVATE APP_VERSION="${PROJECT_VERSION}")
# Связываем программу с нашей статической библиотекой core
target_link_libraries(crypto_tool PRIVATE cryptocore)

# Инструкция для установки программы (потребуется для генерации deb-пакета)
install(TARGETS crypto_tool RUNTIME DESTINATION bin)
