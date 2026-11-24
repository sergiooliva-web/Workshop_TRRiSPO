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
dpkg-deb --info dist/crypto-tool_1.0.1_amd64.deb
```
(Проверяли, что поле `Depends` заполнилось автоматически и содержит системные библиотеки с нужными версиями).

2. Тестирование установки:
```bash
sudo apt install ./dist/crypto-tool_1.0.1_amd64.deb
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

* **Актуальная версия:** `1.0.1`
* **Образ в Docker Hub:** `sergeishikov/crypto_tool`
* **Размер образа:** `127 МБ` (распакованное содержимое — `32.6 МБ`)

### Параметры окружения
* `APP_PORT` — порт, на котором сервер принимает подключения внутри контейнера (по умолчанию `8080`).
* `APP_API_TOKEN` — секретный токен авторизации для доступа к защищенным методам `/api/*` (передается клиентом в заголовке `X-Api-Token`).

### Сборка и запуск контейнера
1. Сборка `.deb`-пакета и Docker-образа:
```bash
./build_deb.sh
docker build -t crypto_tool:1.0.1 .
```

2. Запуск контейнера (с пробросом на внешний порт `8081`):
```bash
docker run -d --name crypto_tool -p 8081:8080 -e APP_API_TOKEN=secret crypto_tool:1.0.1
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

## Эксплуатация в Kubernetes (ПР 4)

Манифесты развертывания расположены в каталоге `k8s/` (`namespace.yaml`, `configmap.yaml`, `deployment.yaml`, `service.yaml`).

### Развертывание сервиса
1. Создание пространства имен и секрета с токеном авторизации (не хранится в Git):
```bash
kubectl apply -f k8s/namespace.yaml
kubectl create secret generic crypto-tool-secret \
  --namespace crypto-tool \
  --from-literal=APP_API_TOKEN=supersecret
```

2. Применение манифестов и проверка готовности подов:
```bash
kubectl apply -f k8s/
kubectl rollout status deployment/crypto-tool -n crypto-tool
kubectl get all -n crypto-tool
```

3. Проверка маршрутизации и балансировки нагрузки:
```bash
kubectl run curl-test --rm -i --restart=Never -n crypto-tool --image=curlimages/curl -- sh -c '
  for i in 1 2 3 4; do curl -s http://crypto-tool/version; done
  curl -s -o /dev/null -w "Без токена: %{http_code}\n" -X POST --data-binary "test" http://crypto-tool/api/encrypt
  curl -s -o /dev/null -w "С токеном: %{http_code}\n" -X POST -H "X-Api-Token: supersecret" -H "X-Password: pass" --data-binary "test" http://crypto-tool/api/encrypt
'
```

### Основные операции сопровождения
* **Обновление версии без простоя (`RollingUpdate`):**
  ```bash
  kubectl set image deployment/crypto-tool -n crypto-tool app=sergeishikov/crypto_tool:1.0.1
  kubectl rollout status deployment/crypto-tool -n crypto-tool
  ```
* **Горизонтальное масштабирование:**
  ```bash
  kubectl scale deployment/crypto-tool -n crypto-tool --replicas=4
  ```
* **Откат на предыдущую версию при сбое:**
  ```bash
  kubectl rollout undo deployment/crypto-tool -n crypto-tool
  kubectl rollout status deployment/crypto-tool -n crypto-tool
  ```
