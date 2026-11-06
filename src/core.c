#include "core.h"
#include <sodium.h>
#include <stdlib.h>
#include <string.h>

int core_encrypt(const unsigned char *plaintext, size_t plaintext_len,
                 const char *password,
                 unsigned char **ciphertext, size_t *ciphertext_len) {
    
    // Инициализация libsodium (безопасно вызывать многократно)
    if (sodium_init() < 0) return -1;

    // Вычисляем размер выходных данных: соль + nonce + сами данные + мак-адрес защиты (MAC)
    *ciphertext_len = crypto_pwhash_SALTBYTES + crypto_secretbox_NONCEBYTES + plaintext_len + crypto_secretbox_MACBYTES;
    *ciphertext = malloc(*ciphertext_len);
    if (*ciphertext == NULL) return -2;

    // Указатели на разные части зашифрованного буфера
    unsigned char *salt = *ciphertext;
    unsigned char *nonce = *ciphertext + crypto_pwhash_SALTBYTES;
    unsigned char *encrypted_data = *ciphertext + crypto_pwhash_SALTBYTES + crypto_secretbox_NONCEBYTES;

    // Generate random salt and nonce
    randombytes_buf(salt, crypto_pwhash_SALTBYTES);
    randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);

    // Деривация ключа из пароля (используем Argon2id)
    unsigned char key[crypto_secretbox_KEYBYTES];
    if (crypto_pwhash(key, sizeof key, password, strlen(password), salt,
                      crypto_pwhash_OPSLIMIT_INTERACTIVE,
                      crypto_pwhash_MEMLIMIT_INTERACTIVE,
                      crypto_pwhash_ALG_DEFAULT) != 0) {
        free(*ciphertext);
        return -3;
    }

    // Шифрование данных
    if (crypto_secretbox_easy(encrypted_data, plaintext, plaintext_len, nonce, key) != 0) {
        sodium_memzero(key, sizeof key);
        free(*ciphertext);
        return -4;
    }

    // Очищаем ключ в памяти ради безопасности
    sodium_memzero(key, sizeof key);
    return 0;
}

int core_decrypt(const unsigned char *ciphertext, size_t ciphertext_len,
                 const char *password,
                 unsigned char **plaintext, size_t *plaintext_len) {

    if (sodium_init() < 0) return -1;

    // Минимальный размер валидного зашифрованного файла
    size_t min_len = crypto_pwhash_SALTBYTES + crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES;
    if (ciphertext_len < min_len) return -2;

    // Размечаем входящий буфер
    const unsigned char *salt = ciphertext;
    const unsigned char *nonce = ciphertext + crypto_pwhash_SALTBYTES;
    const unsigned char *encrypted_data = ciphertext + crypto_pwhash_SALTBYTES + crypto_secretbox_NONCEBYTES;
    size_t encrypted_data_len = ciphertext_len - crypto_pwhash_SALTBYTES - crypto_secretbox_NONCEBYTES;

    *plaintext_len = encrypted_data_len - crypto_secretbox_MACBYTES;
    *plaintext = malloc(*plaintext_len + 1); // +1 для безопасного null-терминатора, если это текст
    if (*plaintext == NULL) return -3;

    // Восстанавливаем ключ по соли и паролю
    unsigned char key[crypto_secretbox_KEYBYTES];
    if (crypto_pwhash(key, sizeof key, password, strlen(password), salt,
                      crypto_pwhash_OPSLIMIT_INTERACTIVE,
                      crypto_pwhash_MEMLIMIT_INTERACTIVE,
                      crypto_pwhash_ALG_DEFAULT) != 0) {
        free(*plaintext);
        return -4;
    }

    // Расшифровываем данные
    if (crypto_secretbox_open_easy(*plaintext, encrypted_data, encrypted_data_len, nonce, key) != 0) {
        sodium_memzero(key, sizeof key);
        free(*plaintext);
        return -5; // Неверный пароль или поврежденные данные
    }

    (*plaintext)[*plaintext_len] = '\0'; // Гарантируем строку
    sodium_memzero(key, sizeof key);
    return 0;
}
