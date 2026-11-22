// Заголовочный файл модуля HTTP-сервера
#ifndef SERVER_H
#define SERVER_H

// Запускает HTTP-сервер с заданным паролем по умолчанию.
// Возвращает 0 при успешном завершении или код ошибки при сбое.
int run_server(const char *default_password);

#endif // SERVER_H
