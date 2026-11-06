#!/bin/sh
set -eu

NAME=crypto-tool
VERSION=$(cat VERSION)
ARCH=$(dpkg --print-architecture)
ROOT=build/pkg/${NAME}_${VERSION}_${ARCH}

# Флаги усиления защиты, принятые в дистрибутиве
export CFLAGS="$(dpkg-buildflags --get CFLAGS)"
export LDFLAGS="$(dpkg-buildflags --get LDFLAGS)"
export CPPFLAGS="$(dpkg-buildflags --get CPPFLAGS)"

# Конфигурируем и собираем выпускную версию (Release)
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=OFF
cmake --build build/release

# Очищаем старую папку сборки пакета, если она была
rm -rf "$ROOT"

# Имитируем установку в корень будущего пакета (программа падает в usr/bin)
DESTDIR="$PWD/$ROOT" cmake --install build/release --strip

# Добавляем файлы конфигурации по правилам файловой иерархии
mkdir -p "$ROOT/etc/crypto_tool"
cp packaging/crypto_tool.conf "$ROOT/etc/crypto_tool/crypto_tool.conf"

# Автоматическое определение зависимостей
mkdir -p debian && touch debian/control
DEPENDS=$(dpkg-shlibdeps -O -e"$ROOT/usr/bin/crypto_tool" | sed 's/^shlibs:Depends=//')
rm -rf debian

# Создаем структуру метаданных внутри пакета
mkdir -p "$ROOT/DEBIAN" dist

# Подставляем реальные VERSION, ARCH и DEPENDS в шаблон control
sed -e "s/@VERSION@/$VERSION/" -e "s/@ARCH@/$ARCH/" -e "s/@DEPENDS@/$DEPENDS/" \
 packaging/DEBIAN/control > "$ROOT/DEBIAN/control"

# Копируем служебный conffiles
cp packaging/DEBIAN/conffiles "$ROOT/DEBIAN/conffiles"

# --- РЕШЕНИЕ ПРОБЛЕМЫ С ПРАВАМИ В WSL НА ДИСКЕ NTFS ---
# Копируем структуру пакета во временную директорию внутри чистой файловой системы Linux
TMP_BUILD="/tmp/deb_build_${NAME}"
rm -rf "$TMP_BUILD"
cp -r "$ROOT" "$TMP_BUILD"

# Принудительно выставляем правильные права внутри /tmp (на NTFS это не работало)
chmod -R 755 "$TMP_BUILD"
chmod 755 "$TMP_BUILD/DEBIAN"

# Собираем готовый .deb пакет из папки /tmp напрямую в вашу папку dist/
dpkg-deb --root-owner-group --build "$TMP_BUILD" dist/

# Очищаем временную папку
rm -rf "$TMP_BUILD"
