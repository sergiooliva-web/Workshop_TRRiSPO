# Crypto Tool (Вариант 3)

Практическая работа по дисциплине «Технологии распространения, развертывания и сопровождения программного обеспечения».

## Описание
Консольная утилита для шифрования и расшифрования файлов по паролю с использованием библиотеки `libsodium`.

## Требования для сборки
Пакеты для сборки описаны в файле `build-deps.txt`.

## Как собрать
```bash
cmake -S . -B build
cmake --build build
```

## Как запустить
```bash
./build/crypto_tool --help
```
## Как проверяли работоспособность пакета

1. Просмотр состава и метаданных созданного пакета:bash
dpkg-deb --info dist/crypto-tool_0.1.0_amd64.deb

(Проверяли, что поле Depends заполнилось автоматически и содержит системную библиотеку с нужной версией).

2. Тестирование установки:bash
sudo apt install ./dist/crypto-tool_0.1.0_amd64.deb

(Пакет устанавливается в систему, бинарник падает в /usr/bin/crypto_tool, а дефолтный конфиг — в /etc/crypto_tool/).

3. Проверка работы флагов:bash
crypto_tool --version  # Выдает версию 0.1.0 из файла VERSION
crypto_tool --help     # Выводит красивую справку по командам

4. Проверка реального шифрования:bash
echo "DevOps Secret" > test.txt
crypto_tool encrypt test.txt encrypted.dat  # Запрашивает пароль и шифрует
crypto_tool decrypt encrypted.dat result.txt # Расшифровывает обратно
cat result.txt # Выводит исходный текст "DevOps Secret"
