#!/bin/bash
set -e

cd ~/esp/blinds_matter

# Активировать окружение
. ~/esp/esp-idf/export.sh > /dev/null 2>&1
. ~/esp/esp-matter/export.sh > /dev/null 2>&1

# Запустить cmake конфигурацию (без компиляции)
idf.py reconfigure 2>/dev/null || true

# Исправить неправильный путь со ; в build.ninja
BROKEN='cd "/Users/dangerwind/esp/esp-matter/connectedhomeip/connectedhomeip/config/esp32/components/chip;/Users/dangerwind/esp/blinds_matter/build/esp-idf/chip"'
FIXED='cd "/Users/dangerwind/esp/blinds_matter/build/esp-idf/chip"'
sed -i '' "s|${BROKEN}|${FIXED}|g" ~/esp/blinds_matter/build/build.ninja

# Создать stamp файл чтобы ninja не пытался пересобрать chip
touch ~/esp/blinds_matter/build/esp-idf/chip/chip_gn-prefix/src/chip_gn-stamp/chip_gn-build

# Запустить финальную сборку напрямую через ninja (минуя idf.py который регенерирует cmake)
cd ~/esp/blinds_matter/build
ninja all
