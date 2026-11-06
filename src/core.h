#ifndef CORE_H
#define CORE_H

#include <stddef.h>

// Функция шифрования буфера данных по паролю
// Возвращает 0 при успехе, ненулевой код при ошибке
int core_encrypt(const unsigned char *plaintext, size_t plaintext_len,
                 const char *password,
                 unsigned char **ciphertext, size_t *ciphertext_len);

// Функция расшифрования буфера данных по паролю
// Возвращает 0 при успехе, ненулевой код при ошибке
int core_decrypt(const unsigned char *ciphertext, size_t ciphertext_len,
                 const char *password,
                 unsigned char **plaintext, size_t *plaintext_len);

#endif // CORE_H
