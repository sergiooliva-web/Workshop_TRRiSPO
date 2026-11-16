#!/bin/sh
set -eu

NAME=crypto-tool
VERSION=$(cat VERSION)

# Проверка совпадения версии в файле VERSION и тега Git (если сборка запущена по тегу)
if [ "${GITHUB_REF_TYPE:-}" = "tag" ]; then
    EXPECTED_TAG="v${VERSION}"
    if [ "$GITHUB_REF_NAME" != "$EXPECTED_TAG" ]; then
        echo "Ошибка: Тег '$GITHUB_REF_NAME' не совпадает с версией '$EXPECTED_TAG' из файла VERSION!" >&2
        exit 1
    fi
fi

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

# Добавляем обязательную документацию пакета для прохождения lintian
mkdir -p "$ROOT/usr/share/doc/$NAME"
cat << EOF > "$ROOT/usr/share/doc/$NAME/copyright"
Format: https://debian.org
Upstream-Name: $NAME

Files: *
Copyright: 2026 Developer
License: MIT
EOF

cat << EOF | gzip -9n > "$ROOT/usr/share/doc/$NAME/changelog.gz"
$NAME ($VERSION) unstable; urgency=low

  * Release version $VERSION.

 -- Developer <dev@example.com>  Mon, 16 Nov 2026 18:00:00 +0300
EOF

# --- РЕШЕНИЕ ПРОБЛЕМЫ С ПРАВАМИ В WSL НА ДИСКЕ NTFS ---
TMP_BUILD="/tmp/${NAME}_${VERSION}_${ARCH}"
rm -rf "$TMP_BUILD"
cp -r "$ROOT" "$TMP_BUILD"

chmod -R 755 "$TMP_BUILD"
chmod 755 "$TMP_BUILD/DEBIAN"
chmod 644 "$TMP_BUILD/etc/crypto_tool/crypto_tool.conf" "$TMP_BUILD/DEBIAN/control" "$TMP_BUILD/DEBIAN/conffiles" "$TMP_BUILD/usr/share/doc/$NAME/copyright" "$TMP_BUILD/usr/share/doc/$NAME/changelog.gz"

dpkg-deb --root-owner-group --build "$TMP_BUILD" dist/
rm -rf "$TMP_BUILD"

