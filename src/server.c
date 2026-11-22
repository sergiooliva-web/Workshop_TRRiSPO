// Реализация HTTP-сервера на базе libmicrohttpd для шифрования и расшифровки данных

#define _POSIX_C_SOURCE 200809L
#include "server.h"
#include "core.h"
#include <microhttpd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef APP_VERSION
#define APP_VERSION "0.0.0"
#endif

// Глобальное состояние сервера и настройки из окружения/конфига
static volatile sig_atomic_t keep_running = 1;
static const char *g_api_token = NULL;
static const char *g_default_password = NULL;

// Буфер для накопления входящих данных одного HTTP-запроса
struct ConnectionInfo {
    unsigned char *data;
    size_t size;
};

// Обработчик сигналов SIGINT/SIGTERM для корректной остановки сервера
static void handle_signal(int sig) {
    (void)sig;
    keep_running = 0;
}

// Формирует и ставит в очередь простой текстовый или JSON-ответ
static enum MHD_Result send_text_response(struct MHD_Connection *connection,
                                          unsigned int status_code,
                                          const char *text,
                                          const char *content_type) {
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(text), (void *)text, MHD_RESPMEM_MUST_COPY);
    if (!response) return MHD_NO;

    MHD_add_response_header(response, "Content-Type", content_type);
    enum MHD_Result ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

// Колбэк очистки памяти после завершения обработки запроса
static void request_completed(void *cls, struct MHD_Connection *connection,
                              void **con_cls, enum MHD_RequestTerminationCode toe) {
    (void)cls;
    (void)connection;
    (void)toe;

    struct ConnectionInfo *con_info = *con_cls;
    if (con_info) {
        free(con_info->data);
        free(con_info);
        *con_cls = NULL;
    }
}

// Основной маршрутизатор и обработчик всех входящих HTTP-запросов
static enum MHD_Result answer_to_connection(void *cls,
                                            struct MHD_Connection *connection,
                                            const char *url,
                                            const char *method,
                                            const char *version,
                                            const char *upload_data,
                                            size_t *upload_data_size,
                                            void **con_cls) {
    (void)cls;
    (void)version;

    // При первом вызове для нового соединения выделяем структуру состояния
    if (*con_cls == NULL) {
        struct ConnectionInfo *con_info = calloc(1, sizeof(struct ConnectionInfo));
        if (!con_info) return MHD_NO;
        *con_cls = con_info;
        return MHD_YES;
    }

    struct ConnectionInfo *con_info = *con_cls;

    // 1. Обработка GET-запросов (проверка состояния и версии сервиса)
    if (strcmp(method, "GET") == 0) {
        if (strcmp(url, "/health") == 0 || strcmp(url, "/ready") == 0) {
            return send_text_response(connection, MHD_HTTP_OK, "{\"status\":\"ok\"}\n", "application/json");
        }
        if (strcmp(url, "/version") == 0) {
            char hostname[128] = "unknown";
            gethostname(hostname, sizeof(hostname) - 1);
            char buf[256];
            snprintf(buf, sizeof(buf), "{\"version\":\"%s\",\"host\":\"%s\"}\n", APP_VERSION, hostname);
            return send_text_response(connection, MHD_HTTP_OK, buf, "application/json");
        }
        return send_text_response(connection, MHD_HTTP_NOT_FOUND, "Not Found\n", "text/plain");
    }

    // 2. Обработка POST-запросов (/api/encrypt и /api/decrypt)
    if (strcmp(method, "POST") == 0) {
        int is_encrypt = (strcmp(url, "/api/encrypt") == 0);
        int is_decrypt = (strcmp(url, "/api/decrypt") == 0);

        if (!is_encrypt && !is_decrypt) {
            return send_text_response(connection, MHD_HTTP_NOT_FOUND, "Not Found\n", "text/plain");
        }

        // Накопление тела запроса (libmicrohttpd может передавать его по частям)
        if (*upload_data_size != 0) {
            unsigned char *new_data = realloc(con_info->data, con_info->size + *upload_data_size);
            if (!new_data) return MHD_NO;
            con_info->data = new_data;
            memcpy(con_info->data + con_info->size, upload_data, *upload_data_size);
            con_info->size += *upload_data_size;
            *upload_data_size = 0;
            return MHD_YES;
        }

        // Проверка токена авторизации (если задан в переменной окружения APP_API_TOKEN)
        if (g_api_token && strlen(g_api_token) > 0) {
            const char *req_token = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Api-Token");
            if (!req_token || strcmp(req_token, g_api_token) != 0) {
                return send_text_response(connection, MHD_HTTP_UNAUTHORIZED, "Unauthorized\n", "text/plain");
            }
        }

        // Получение пароля из заголовка X-Password или конфига по умолчанию
        const char *password = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Password");
        if (!password || strlen(password) == 0) {
            password = g_default_password;
        }
        if (!password || strlen(password) == 0) {
            return send_text_response(connection, MHD_HTTP_BAD_REQUEST, "Password required (X-Password header or config)\n", "text/plain");
        }

        // Защита от пустого тела запроса
        if (con_info->size == 0 || con_info->data == NULL) {
            return send_text_response(connection, MHD_HTTP_BAD_REQUEST, "Empty request body\n", "text/plain");
        }

        // Вызов криптографических функций из модуля core
        unsigned char *out_buf = NULL;
        size_t out_len = 0;
        int res = is_encrypt
            ? core_encrypt(con_info->data, con_info->size, password, &out_buf, &out_len)
            : core_decrypt(con_info->data, con_info->size, password, &out_buf, &out_len);

        if (res != 0) {
            return send_text_response(connection, MHD_HTTP_BAD_REQUEST, "Crypto operation failed\n", "text/plain");
        }

        // Отправка бинарного результата; MHD_RESPMEM_MUST_FREE освободит out_buf автоматически
        struct MHD_Response *response = MHD_create_response_from_buffer(
            out_len, out_buf, MHD_RESPMEM_MUST_FREE);
        if (!response) {
            free(out_buf);
            return MHD_NO;
        }

        MHD_add_response_header(response, "Content-Type", "application/octet-stream");
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    // Все остальные HTTP-методы запрещены
    return send_text_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED, "Method Not Allowed\n", "text/plain");
}

// Инициализирует настройки, запускает демон сервера и ожидает сигнала завершения
int run_server(const char *default_password) {
    g_default_password = default_password;
    g_api_token = getenv("APP_API_TOKEN");

    // Чтение порта из переменной окружения APP_PORT (по умолчанию 8080)
    unsigned int port = 8080;
    const char *port_env = getenv("APP_PORT");
    if (port_env && strlen(port_env) > 0) {
        int parsed = atoi(port_env);
        if (parsed > 0 && parsed <= 65535) {
            port = (unsigned int)parsed;
        }
    }

    // Настройка перехвата сигналов остановки процесса
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    // Запуск HTTP-демона со встроенным потоком опроса сокетов
    struct MHD_Daemon *daemon = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD,
        (uint16_t)port,
        NULL, NULL,
        &answer_to_connection, NULL,
        MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL,
        MHD_OPTION_END);

    if (!daemon) {
        fprintf(stderr, "Ошибка: Не удалось запустить HTTP-сервер на порту %u\n", port);
        return 1;
    }

    printf("HTTP-сервер запущен на порту %u (версия %s)\n", port, APP_VERSION);
    fflush(stdout);

    // Ожидание сигнала завершения (SIGINT / SIGTERM)
    while (keep_running) {
        pause();
    }

    printf("Завершение работы HTTP-сервера...\n");
    MHD_stop_daemon(daemon);
    return 0;
}
