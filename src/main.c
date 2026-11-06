#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "core.h"

void load_default_config(char *default_password, size_t max_len) {
    const char *config_path = "/etc/crypto_tool/crypto_tool.conf";
    FILE *f = fopen(config_path, "r");
    if (!f) return;
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "password=", 9) == 0) {
            char *val = line + 9;
            val[strcspn(val, "\r\n")] = 0;
            strncpy(default_password, val, max_len - 1);
            break;
        }
    }
    fclose(f);
}

unsigned char *read_file(const char *filename, size_t *length) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Ошибка: Не удалось открыть файл для чтения: %s\n", filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0) {
        fclose(f);
        return NULL;
    }

    unsigned char *buffer = malloc(size);
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(buffer, 1, size, f);
    if (read_bytes != (size_t)size) {
        free(buffer);
        fclose(f);
        return NULL;
    }

    fclose(f);
    *length = (size_t)size;
    return buffer;
}

int write_file(const char *filename, const unsigned char *buffer, size_t length) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Ошибка: Не удалось открыть файл для записи: %s\n", filename);
        return -1;
    }

    size_t written = fwrite(buffer, 1, length, f);
    fclose(f);
    return (written == length) ? 0 : -1;
}

void print_help(const char *prog_name) {
    printf("Использование: %s [команда] [аргументы]\n\n", prog_name);
    printf("Команды:\n");
    printf("  encrypt <входной_файл> <выходной_файл> [пароль]\n");
    printf("  decrypt <входной_файл> <выходной_файл> [пароль]\n\n");
    printf("Опции:\n");
    printf("  --help     Показать эту справку\n");
    printf("  --version  Показать версию программы\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Ошибка: Не указаны аргументы. Используйте --help для справки.\n");
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0) {
        print_help(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0) {
        #ifdef APP_VERSION
            printf("crypto_tool версия %s\n", APP_VERSION);
        #else
            printf("crypto_tool версия unknown\n");
        #endif
        return 0;
    }

    int is_encrypt = (strcmp(argv[1], "encrypt") == 0);
    int is_decrypt = (strcmp(argv[1], "decrypt") == 0);

    if (!is_encrypt && !is_decrypt) {
        fprintf(stderr, "Ошибка: Неизвестная команда '%s'. Используйте --help.\n", argv[1]);
        return 1;
    }

    if (argc < 4) {
        fprintf(stderr, "Ошибка: Недостаточно аргументов. Использование: %s %s <входной_файл> <выходной_файл> [пароль]\n", argv[0], argv[1]);
        return 1;
    }

    const char *infile = argv[2];
    const char *outfile = argv[3];
    char password[256] = {0};

    if (argc >= 5) {
        strncpy(password, argv[4], sizeof(password) - 1);
    } else {
        load_default_config(password, sizeof(password));
    }

    if (strlen(password) == 0) {
        char *input_pass = getpass("Введите пароль: ");
        if (input_pass && strlen(input_pass) > 0) {
            strncpy(password, input_pass, sizeof(password) - 1);
        } else {
            fprintf(stderr, "Ошибка: Пароль не может быть пустым.\n");
            return 1;
        }
    }

    size_t in_len = 0;
    unsigned char *in_buf = read_file(infile, &in_len);
    if (!in_buf) return 1;

    unsigned char *out_buf = NULL;
    size_t out_len = 0;
    int res = 0;

    if (is_encrypt) {
        res = core_encrypt(in_buf, in_len, password, &out_buf, &out_len);
    } else {
        res = core_decrypt(in_buf, in_len, password, &out_buf, &out_len);
    }

    free(in_buf);

    if (res != 0) {
        fprintf(stderr, "Ошибка: Операция %s завершилась неудачей (код %d). Проверьте пароль.\n", argv[1], res);
        return 1;
    }

    if (write_file(outfile, out_buf, out_len) != 0) {
        free(out_buf);
        return 1;
    }

    free(out_buf);
    printf("Успешно: Файл %s сохранен.\n", outfile);
    return 0;
}
