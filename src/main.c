#include <stdio.h>
#include <string.h>
#include "core.h"

void print_help(const char *prog_name) {
    printf("Использование: %s [команда] [аргументы]\n\n", prog_name);
    printf("Команды:\n");
    printf("  encrypt <входной_файл> <выходной_файл>  Шифрование файла\n");
    printf("  decrypt <входной_файл> <выходной_файл>  Расшифрование файла\n\n");
    printf("Опции:\n");
    printf("  --help     Показать эту справку\n");
    printf("  --version  Показать версию программы\n");
}

int main(int argc, char *argv[]) {
    // 1. Проверка на отсутствие аргументов
    if (argc < 2) {
        fprintf(stderr, "Ошибка: Не указаны аргументы. Используйте --help для справки.\n");
        return 1; // Ненулевой код при ошибке
    }

    // 2. Обработка системных флагов --help и --version
    if (strcmp(argv[1], "--help") == 0) {
        print_help(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0) {
        // Макрос APP_VERSION будет передан из CMakeLists.txt
        #ifdef APP_VERSION
            printf("crypto_tool версия %s\n", APP_VERSION);
        #else
            printf("crypto_tool версия unknown\n");
        #endif
        return 0;
    }

    // 3. Обработка подкоманд encrypt и decrypt (пока заглушки)
    if (strcmp(argv[1], "encrypt") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Ошибка: Для шифрования нужно указать входной и выходной файлы.\n");
            return 1;
        }
        printf("Запущено шифрование (заглушка): %s -> %s\n", argv[2], argv[3]);
        return 0;
    } 
    
    if (strcmp(argv[1], "decrypt") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Ошибка: Для расшифрования нужно указать входной и выходной файлы.\n");
            return 1;
        }
        printf("Запущено расшифрование (заглушка): %s -> %s\n", argv[2], argv[3]);
        return 0;
    }

    // Если введена неизвестная команда
    fprintf(stderr, "Ошибка: Неизвестная команда '%s'. Используйте --help.\n", argv[1]);
    return 1;
}
