#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core.h"

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "Провал проверки в %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        return 1; \
    } \
} while (0)

// Тест 1: Обычное шифрование и расшифровка строки
int test_encrypt_decrypt_normal(void) {
    const char *msg = "Hello, DevOps CI/CD!";
    const char *pass = "StrongPassword123!";
    unsigned char *cipher = NULL;
    size_t cipher_len = 0;

    ASSERT_TRUE(core_encrypt((const unsigned char *)msg, strlen(msg), pass, &cipher, &cipher_len) == 0);
    ASSERT_TRUE(cipher != NULL);
    ASSERT_TRUE(cipher_len > strlen(msg));

    unsigned char *decrypted = NULL;
    size_t decrypted_len = 0;

    ASSERT_TRUE(core_decrypt(cipher, cipher_len, pass, &decrypted, &decrypted_len) == 0);
    ASSERT_TRUE(decrypted != NULL);
    ASSERT_TRUE(decrypted_len == strlen(msg));
    ASSERT_TRUE(strcmp(msg, (char *)decrypted) == 0);

    free(cipher);
    free(decrypted);
    return 0;
}

// Тест 2: Граничный случай — шифрование и расшифровка пустого ввода (0 байт)
int test_encrypt_decrypt_empty(void) {
    const char *pass = "Pass123";
    unsigned char dummy = 0;
    unsigned char *cipher = NULL;
    size_t cipher_len = 0;

    ASSERT_TRUE(core_encrypt(&dummy, 0, pass, &cipher, &cipher_len) == 0);
    ASSERT_TRUE(cipher != NULL);

    unsigned char *decrypted = NULL;
    size_t decrypted_len = 0;

    ASSERT_TRUE(core_decrypt(cipher, cipher_len, pass, &decrypted, &decrypted_len) == 0);
    ASSERT_TRUE(decrypted != NULL);
    ASSERT_TRUE(decrypted_len == 0);

    free(cipher);
    free(decrypted);
    return 0;
}

// Тест 3: Граничный случай — пустой пароль
int test_empty_password(void) {
    const char *msg = "Data with empty password";
    unsigned char *cipher = NULL;
    size_t cipher_len = 0;

    ASSERT_TRUE(core_encrypt((const unsigned char *)msg, strlen(msg), "", &cipher, &cipher_len) == 0);

    unsigned char *decrypted = NULL;
    size_t decrypted_len = 0;

    ASSERT_TRUE(core_decrypt(cipher, cipher_len, "", &decrypted, &decrypted_len) == 0);
    ASSERT_TRUE(decrypted_len == strlen(msg));
    ASSERT_TRUE(memcmp(msg, decrypted, decrypted_len) == 0);

    free(cipher);
    free(decrypted);
    return 0;
}

// Тест 4: Некорректный ввод — расшифровка с неверным паролем (ожидаем код -5)
int test_decrypt_wrong_password(void) {
    const char *msg = "Secret message";
    unsigned char *cipher = NULL;
    size_t cipher_len = 0;

    ASSERT_TRUE(core_encrypt((const unsigned char *)msg, strlen(msg), "RightPass", &cipher, &cipher_len) == 0);

    unsigned char *decrypted = NULL;
    size_t decrypted_len = 0;

    ASSERT_TRUE(core_decrypt(cipher, cipher_len, "WrongPass", &decrypted, &decrypted_len) == -5);

    free(cipher);
    return 0;
}

// Тест 5: Некорректный ввод — порча байтов шифротекста (ожидаем код -5)
int test_decrypt_tampered_data(void) {
    const char *msg = "Integrity check data";
    const char *pass = "MySecret";
    unsigned char *cipher = NULL;
    size_t cipher_len = 0;

    ASSERT_TRUE(core_encrypt((const unsigned char *)msg, strlen(msg), pass, &cipher, &cipher_len) == 0);

    // Портим последний байт шифротекста
    cipher[cipher_len - 1] ^= 0xFF;

    unsigned char *decrypted = NULL;
    size_t decrypted_len = 0;

    ASSERT_TRUE(core_decrypt(cipher, cipher_len, pass, &decrypted, &decrypted_len) == -5);

    free(cipher);
    return 0;
}

// Тест 6: Некорректный ввод — слишком короткий шифротекст (ожидаем код -2)
int test_decrypt_truncated_data(void) {
    unsigned char short_buf[] = {1, 2, 3, 4};
    unsigned char *decrypted = NULL;
    size_t decrypted_len = 0;

    ASSERT_TRUE(core_decrypt(short_buf, sizeof(short_buf), "Pass", &decrypted, &decrypted_len) == -2);
    return 0;
}

// Тест 7: Проверка уникальности salt и nonce (два шифрования дают разный результат)
int test_encrypt_randomized(void) {
    const char *msg = "Same message";
    const char *pass = "SamePass";
    unsigned char *cipher1 = NULL, *cipher2 = NULL;
    size_t len1 = 0, len2 = 0;

    ASSERT_TRUE(core_encrypt((const unsigned char *)msg, strlen(msg), pass, &cipher1, &len1) == 0);
    ASSERT_TRUE(core_encrypt((const unsigned char *)msg, strlen(msg), pass, &cipher2, &len2) == 0);
    ASSERT_TRUE(len1 == len2);
    ASSERT_TRUE(memcmp(cipher1, cipher2, len1) != 0);

    free(cipher1);
    free(cipher2);
    return 0;
}

// Тест 8: Граничный случай — большой блок данных (1 МБ)
int test_large_buffer(void) {
    size_t big_size = 1024 * 1024;
    unsigned char *big_data = malloc(big_size);
    ASSERT_TRUE(big_data != NULL);
    memset(big_data, 'A', big_size);

    const char *pass = "BigDataPass";
    unsigned char *cipher = NULL;
    size_t cipher_len = 0;

    ASSERT_TRUE(core_encrypt(big_data, big_size, pass, &cipher, &cipher_len) == 0);

    unsigned char *decrypted = NULL;
    size_t decrypted_len = 0;

    ASSERT_TRUE(core_decrypt(cipher, cipher_len, pass, &decrypted, &decrypted_len) == 0);
    ASSERT_TRUE(decrypted_len == big_size);
    ASSERT_TRUE(memcmp(big_data, decrypted, big_size) == 0);

    free(big_data);
    free(cipher);
    free(decrypted);
    return 0;
}

int main(void) {
    if (test_encrypt_decrypt_normal() != 0) return 1;
    if (test_encrypt_decrypt_empty() != 0) return 1;
    if (test_empty_password() != 0) return 1;
    if (test_decrypt_wrong_password() != 0) return 1;
    if (test_decrypt_tampered_data() != 0) return 1;
    if (test_decrypt_truncated_data() != 0) return 1;
    if (test_encrypt_randomized() != 0) return 1;
    if (test_large_buffer() != 0) return 1;

    printf("Все 8 модульных тестов успешно пройдены!\n");
    return 0;
}
