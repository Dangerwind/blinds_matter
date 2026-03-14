# ESP32 Blinds Controller with Matter
## Полная инструкция по установке, сборке и прошивке (macOS)

---

## Содержание
1. [Схема подключения](#1-схема-подключения)
2. [Требования к системе](#2-требования-к-системе)
3. [Установка зависимостей](#3-установка-зависимостей)
4. [Клонирование ESP-IDF и ESP-Matter](#4-клонирование-esp-idf-и-esp-matter)
5. [Настройка проекта](#5-настройка-проекта)
6. [Сборка прошивки](#6-сборка-прошивки)
7. [Прошивка ESP32](#7-прошивка-esp32)
8. [QR-код для Matter](#8-qr-код-для-matter)
9. [Подключение к умному дому](#9-подключение-к-умному-дому)
10. [Диагностика и отладка](#10-диагностика-и-отладка)

---

## 1. Схема подключения

```
ESP32 DEVKIT V1
┌────────────────────────────────────────┐
│  GPIO12 → MX1508 IN1 (Motor 1)        │
│  GPIO13 → MX1508 IN2 (Motor 1)        │
│  GPIO14 → MX1508 IN3 (Motor 2)        │
│  GPIO15 → MX1508 IN4 (Motor 2)        │
│                                        │
│  GPIO4  → Геркон 1 ОТКРЫТ (на GND)   │
│  GPIO5  → Геркон 1 ЗАКРЫТ (на GND)   │
│  GPIO16 → Геркон 2 ОТКРЫТ (на GND)   │
│  GPIO17 → Геркон 2 ЗАКРЫТ (на GND)   │
│                                        │
│  GPIO18 → Кнопка Открыть 1 (на GND)  │
│  GPIO19 → Кнопка Закрыть 1 (на GND)  │
│  GPIO23 → Кнопка Открыть 2 (на GND)  │
│  GPIO22 → Кнопка Закрыть 2 (на GND)  │
│  GPIO21 → Аварийная стоп  (на GND)   │
│                                        │
│  GPIO25 → LED1 → 330Ω → GND          │
│  GPIO26 → LED2 → 330Ω → GND          │
└────────────────────────────────────────┘

Питание MX1508:
  VCC → 5V (от ESP32 или внешний источник)
  GND → общая земля с ESP32

Герконы и кнопки подключаются одним контактом к GPIO,
другим к GND. Внутренние подтяжки (pull-up) включены в коде.
```

---

## 2. Требования к системе

- macOS 11 (Big Sur) или новее
- Не менее 10 ГБ свободного места на диске
- Python 3.8–3.11 (НЕ 3.12 — несовместим с некоторыми IDF-скриптами)
- Xcode Command Line Tools
- Интернет-соединение для git clone

---

## 3. Установка зависимостей

### 3.1 Установить Homebrew (если не установлен)
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

После установки добавьте Homebrew в PATH (для Apple Silicon):
```bash
echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
eval "$(/opt/homebrew/bin/brew shellenv)"
```

### 3.2 Установить Xcode Command Line Tools
```bash
xcode-select --install
```
Нажмите «Установить» в появившемся диалоге.

### 3.3 Установить системные зависимости через Homebrew
```bash
brew install cmake ninja dfu-util ccache python@3.11
brew install wget curl git
```

### 3.4 Установить Python 3.11 по умолчанию (если не он)
```bash
# Проверить версию
python3 --version

# Если нужно — явно указать путь к python3.11
which python3.11
```

### 3.5 Установить pyserial (нужен для прошивки)
```bash
python3 -m pip install pyserial
```

---

## 4. Клонирование ESP-IDF и ESP-Matter

> ⚠️ Выполняйте все команды из одной директории. Рекомендуется `~/esp/`.

### 4.1 Создать рабочую директорию
```bash
mkdir -p ~/esp
cd ~/esp
```

### 4.2 Клонировать ESP-IDF v5.1.4
```bash
git clone --recursive --depth=1 --branch v5.1.4 \
    https://github.com/espressif/esp-idf.git esp-idf
```

> Используем v5.1.4 — это последняя стабильная ветка, официально поддерживаемая esp-matter на момент создания проекта.

### 4.3 Установить инструменты ESP-IDF
```bash
cd ~/esp/esp-idf
./install.sh esp32
```

Скрипт скачает и установит:
- xtensa-esp32 GCC toolchain
- OpenOCD
- esptool.py
- idf-component-manager

Это займёт 5–15 минут в зависимости от скорости интернета.

### 4.4 Добавить ESP-IDF в переменные окружения
```bash
# Добавить в .zprofile для автоматической загрузки в новых сессиях
echo 'export IDF_PATH=~/esp/esp-idf' >> ~/.zprofile
echo 'alias get_idf=". $IDF_PATH/export.sh"' >> ~/.zprofile

# Активировать в ТЕКУЩЕМ терминале
export IDF_PATH=~/esp/esp-idf
. $IDF_PATH/export.sh
```

### 4.5 Клонировать ESP-Matter v1.3
```bash
cd ~/esp

git clone --recursive --depth=1 --branch v1.3 \
    https://github.com/espressif/esp-matter.git esp-matter
```

> esp-matter v1.3 совместим с esp-idf v5.1.x и поддерживает Matter SDK 1.2 (ConnectedHomeIP). Эта комбинация является рекомендуемой на mid-2024 и протестирована с Apple Home, Google Home, Amazon Alexa.

### 4.6 Установить зависимости ESP-Matter
```bash
cd ~/esp/esp-matter
./install.sh
```

Скрипт установит Python-пакеты и скомпилирует необходимые хост-инструменты (chip-cert, spake2p и др.). Это займёт 10–20 минут.

### 4.7 Добавить ESP-Matter в окружение
```bash
echo 'export ESP_MATTER_PATH=~/esp/esp-matter' >> ~/.zprofile
echo 'alias get_matter=". $ESP_MATTER_PATH/export.sh"' >> ~/.zprofile

export ESP_MATTER_PATH=~/esp/esp-matter
. $ESP_MATTER_PATH/export.sh
```

---

## 5. Настройка проекта

### 5.1 Скопировать проект в рабочую директорию
```bash
# Распакуйте архив проекта, например:
unzip blinds_matter.zip -d ~/esp/
# или
cp -r /путь/к/blinds_matter ~/esp/blinds_matter
```

### 5.2 Перейти в директорию проекта
```bash
cd ~/esp/blinds_matter
```

### 5.3 Активировать окружение ESP-IDF + ESP-Matter
> ⚠️ Это нужно делать в КАЖДОМ новом терминале перед сборкой!

```bash
. ~/esp/esp-idf/export.sh
. ~/esp/esp-matter/export.sh
```

### 5.4 Установить таргет ESP32
```bash
idf.py set-target esp32
```

### 5.5 Сгенерировать commissioning credentials (паспорт устройства)
Matter-устройство должно иметь уникальный PASSCODE (PIN-код) и DISCRIMINATOR (позволяет отличить устройство при поиске).

Сгенерировать и записать в NVS-фабричный раздел:
```bash
# Установить утилиту генерации данных (входит в esp-matter)
pip3 install esp-matter-mfg-tool --break-system-packages 2>/dev/null || \
pip3 install esp-matter-mfg-tool

# Сгенерировать commissioning данные
# PASSCODE — 8-значный PIN (20202021 — тестовый, замените на свой)
# DISCRIMINATOR — 12-битное число (0–4095)
python3 $ESP_MATTER_PATH/tools/mfg_tool/mfg_tool.py \
    -n 1 \
    --passcode 20202021 \
    --discriminator 3840 \
    --vendor-id 0xFFF1 \
    --product-id 0x8001 \
    --output ./mfg_output

echo "Commissioning data generated in ./mfg_output"
```

> **Примечание:** Для домашнего использования тестовые значения (passcode=20202021, discriminator=3840) полностью корректны. Для коммерческого продукта необходима регистрация в CSA (Connectivity Standards Alliance) и получение официальных VID/PID.

---

## 6. Сборка прошивки

### 6.1 Запустить сборку
```bash
cd ~/esp/blinds_matter
idf.py build
```

Первая сборка занимает 10–30 минут (компилируется весь Matter SDK ~900 файлов). Последующие — значительно быстрее благодаря ccache.

### 6.2 Успешная сборка

В конце вывода должно быть:
```
[100%] Linking CXX executable blinds_matter.elf
[100%] Built target blinds_matter.elf
...
Project build complete. To flash, run:
  idf.py flash
```

---

## 7. Прошивка ESP32

### 7.1 Подключить ESP32 по USB

Использовать оригинальный кабель с поддержкой данных (не только зарядный).

### 7.2 Найти порт
```bash
ls /dev/cu.usbserial-* /dev/cu.SLAB_* /dev/cu.wchusbserial* 2>/dev/null
```

Обычно порт выглядит как `/dev/cu.usbserial-XXXX` или `/dev/cu.SLAB_USBtoUART`.

### 7.3 Прошить основную прошивку
```bash
idf.py -p /dev/cu.usbserial-XXXX flash
```

Замените `/dev/cu.usbserial-XXXX` на ваш порт.

### 7.4 Прошить commissioning данные (Matter credentials)
```bash
# Найти bin-файл с commissioning данными
ls ./mfg_output/*/

# Прошить его в раздел 'fctry' (factory NVS)
esptool.py -p /dev/cu.usbserial-XXXX write_flash \
    0x321000 ./mfg_output/1/1-manufacturing.bin
```

### 7.5 Полная прошивка (всё за один раз)
```bash
idf.py -p /dev/cu.usbserial-XXXX flash monitor
```

Флаг `monitor` автоматически открывает консоль после прошивки.

---

## 8. QR-код для Matter

После загрузки устройства в консоли (monitor) вы увидите:

```
I (1234) chip[DL]: Device Configuration:
I (1234) chip[DL]:   Serial Number: TEST_SN
I (1234) chip[DL]:   Vendor Id: 65521 (0xFFF1)
I (1234) chip[DL]:   Product Id: 32769 (0x8001)
I (1234) chip[DL]:   Setup Pin Code (0 for UNKNOWN/ERROR): 20202021
I (1234) chip[DL]:   Setup Discriminator (0xFFFF for UNKNOWN/ERROR): 3840 (0xF00)
I (1234) chip[DL]:   Pairing Hint: 33, Pairing Instruction: (none)
I (1234) chip[SVR]: SetupQRCode: [MT:Y3.13OTB00KA0648G00]
I (1234) chip[SVR]: Copy/paste the below URL in a browser to see the QR Code:
I (1234) chip[SVR]: https://project-chip.github.io/connectedhomeip/qrcode.html?data=MT%3AY3.13OTB00KA0648G00
```

**Открыть QR-код:**
1. Скопируйте ссылку из консоли
2. Откройте в браузере — появится QR-код
3. Используйте этот QR-код в приложении умного дома

**Ручной код для сопряжения:** `2020 2021` (passcode)

---

## 9. Подключение к умному дому

### Apple Home
1. Откройте приложение **Дом** на iPhone/iPad
2. Нажмите **«+»** → **«Добавить устройство»**
3. Нажмите **«Отсканируйте QR-код»**
4. Отсканируйте QR-код из браузера (или введите 8-значный код вручную)
5. Следуйте инструкциям — устройство появится как 4 выключателя: «Blinds1-Open», «Blinds1-Close», «Blinds2-Open», «Blinds2-Close»

### Google Home
1. Откройте приложение **Google Home**
2. Нажмите **«+»** → **«Настройка устройства»** → **«Новое устройство»**
3. Выберите дом, нажмите **«Устройство Matter»**
4. Отсканируйте QR-код

### Amazon Alexa
1. Откройте приложение **Amazon Alexa**
2. Нажмите **«+»** → **«Добавить устройство»**
3. Выберите **«Другой»** → **«Matter»**
4. Следуйте инструкциям

---

## 10. Диагностика и отладка

### Открыть монитор (без перепрошивки)
```bash
idf.py -p /dev/cu.usbserial-XXXX monitor
```
Закрыть: `Ctrl+]`

### Уровни логов

В консоли увидите метки:
- `I` — Info (нормальная работа)
- `W` — Warning (предупреждение, напр. сработал сторожевой таймер)
- `E` — Error (ошибка)

Основные теги:
- `motor` — события моторов
- `buttons` — нажатия кнопок
- `matter` — Matter-события
- `main` — загрузка

### Сброс commissioning данных (если нужно подключить к другому дому)
```bash
# Стереть NVS с commissioning данными и перезагрузить
idf.py -p /dev/cu.usbserial-XXXX erase-flash
# Затем прошить заново:
idf.py -p /dev/cu.usbserial-XXXX flash
esptool.py -p /dev/cu.usbserial-XXXX write_flash \
    0x321000 ./mfg_output/1/1-manufacturing.bin
```

### Сборка с очисткой кэша
```bash
idf.py fullclean
idf.py build
```

### Изменить сторожевой таймер
Откройте `main/config.h`, измените константу:
```c
#define MOTOR_WATCHDOG_MS   15000   // 15 секунд → замените на нужное значение
```
Пересоберите: `idf.py build flash`

---

## Структура проекта

```
blinds_matter/
├── CMakeLists.txt          — корневой файл сборки
├── sdkconfig.defaults      — настройки SDK (BT, Wi-Fi, Matter)
├── partitions.csv          — таблица разделов flash (4 МБ)
├── README.md               — эта инструкция
└── main/
    ├── CMakeLists.txt      — регистрация компонента
    ├── config.h            — GPIO пины и константы
    ├── main.cpp            — точка входа, задачи FreeRTOS
    ├── motor_control.h/.cpp — управление моторами MX1508
    ├── button_handler.h/.cpp — обработка кнопок с дебаунсом
    └── matter_handler.h/.cpp — интеграция с Matter SDK
```

---

## Версии компонентов (протестировано)

| Компонент | Версия | Примечание |
|-----------|--------|------------|
| ESP-IDF | v5.1.4 | LTS ветка, стабильная |
| ESP-Matter | v1.3 | Поддерживает Matter 1.2 |
| ConnectedHomeIP | 1.2.0.0 | Включён в esp-matter |
| Toolchain | xtensa-esp32-elf-12.2.0 | Устанавливается автоматически |
| Python | 3.11.x | 3.12 не поддерживается IDF |

---

## Часто задаваемые вопросы

**Q: Устройство не появляется при поиске в приложении**
A: Убедитесь, что телефон и ESP32 находятся в одной Wi-Fi сети 2.4 ГГц. Matter требует 2.4 ГГц при commissioning.

**Q: Ошибка «Failed to create Matter node»**
A: Проверьте, что раздел `fctry` прошит корректно и sdkconfig.defaults применился. Выполните `idf.py fullclean && idf.py build flash`.

**Q: Мотор не останавливается на концевом выключателе**
A: Проверьте подключение геркона: другой контакт должен идти на GND, не на VCC. Также проверьте номер GPIO в config.h.

**Q: QR-код не отображается в консоли**
A: Commissioning данные могут не совпадать с прошитыми. Убедитесь, что прошили mfg bin-файл командой из раздела 7.4.
