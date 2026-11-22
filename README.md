# Crypto Tool (Вариант 3)

Практическая работа по дисциплине «Технологии распространения, развертывания и сопровождения программного обеспечения».

## Описание
Утилита и HTTP-сервис для шифрования и расшифрования данных по паролю с использованием библиотек `libsodium` и `libmicrohttpd`.

## Требования для сборки
Пакеты для сборки описаны в файле `build-deps.txt`.

## Как собрать
```bash
cmake -S . -B build
cmake --build build
```

## Как запустить (CLI)
```bash
./build/crypto_tool --help
```

## Как проверяли работоспособность пакета (ПР 1)

1. Просмотр состава и метаданных созданного пакета:
```bash
dpkg-deb --info dist/crypto-tool_0.3.0_amd64.deb
```
(Проверяли, что поле `Depends` заполнилось автоматически и содержит системные библиотеки с нужными версиями).

2. Тестирование установки:
```bash
sudo apt install ./dist/crypto-tool_0.3.0_amd64.deb
```
(Пакет устанавливается в систему, бинарник попадает в `/usr/bin/crypto_tool`, а дефолтный конфиг — в `/etc/crypto_tool/`).

3. Проверка работы флагов:
```bash
crypto_tool --version  # Выдает версию из файла VERSION
crypto_tool --help     # Выводит справку по командам
```

4. Проверка реального шифрования:
```bash
echo "DevOps Secret" > test.txt
crypto_tool encrypt test.txt encrypted.dat  # Запрашивает пароль и шифрует
crypto_tool decrypt encrypted.dat result.txt # Расшифровывает обратно
cat result.txt # Выводит исходный текст "DevOps Secret"
```

## Запуск в Docker-контейнере (ПР 3)

Образ собирается на базе `ubuntu:24.04` из готового `.deb`-пакета и запускает HTTP-сервер от непривилегированного пользователя (`UID 10001`).

* **Актуальная версия:** `0.3.0`
* **Размер образа:** `127 МБ` (распакованное содержимое — `32.6 МБ`)

### Параметры окружения
* `APP_PORT` — порт, на котором сервер принимает подключения внутри контейнера (по умолчанию `8080`).
* `APP_API_TOKEN` — секретный токен авторизации для доступа к защищенным методам `/api/*` (передается клиентом в заголовке `X-Api-Token`).

### Сборка и запуск контейнера
1. Сборка `.deb`-пакета и Docker-образа:
```bash
./build_deb.sh
docker build -t crypto_tool:0.3.0 .
```

2. Запуск контейнера (с пробросом на внешний порт `8081`):
```bash
docker run -d --name crypto_tool -p 8081:8080 -e APP_API_TOKEN=secret crypto_tool:0.3.0
```

3. Проверка работы API и состояния здоровья:
```bash
# Проверка статуса и версии
curl -s http://localhost:8081/health
curl -s http://localhost:8081/version

# Шифрование и расшифрование данных по HTTP
curl -s -X POST -H "X-Api-Token: secret" -H "X-Password: mypass" \
     --data-binary "DevOps Secret" http://localhost:8081/api/encrypt -o enc.bin

curl -s -X POST -H "X-Api-Token: secret" -H "X-Password: mypass" \
     --data-binary @enc.bin http://localhost:8081/api/decrypt
```
