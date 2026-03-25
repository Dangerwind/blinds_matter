# ESP32 Blinds Controller with Matter

Контроллер жалюзи на ESP32-WROOM-32 с интеграцией в умный дом через Matter (Home Assistant, Apple Home, Google Home).

---

## Схема включения

### Питание

Питание 5В подаётся через стабилизатор **ASM1117-3.3** который даёт 3.3В для ESP32 и DRV8833.

```
5V ──┬── ASM1117-3.3 ──┬── 3.3V (ESP32, DRV8833 logic)
     |   [IN] [OUT]    |
   100мкФ            100мкФ
   танталов.         танталов.
     |                 |
    GND               GND
```

> Конденсаторы танталовые 100мкФ на входе и выходе стабилизатора.  
> 5В также идёт на VM пин DRV8833 (питание моторов).

---

### ESP32-WROOM-32 — обязательные подтяжки

Без этих резисторов ESP32 не запустится или не будет прошиваться:

| Пин ESP32 | Резистор | Куда |
|-----------|----------|------|
| EN | 10кОм | → 3.3В |
| GPIO0 | 10кОм | → 3.3В |
| GPIO15 | не нужен | внутренний pull-up включён программно |

```
3.3В ──10кОм──── EN
3.3В ──10кОм──── GPIO0
```

---

### Светодиоды

Светодиоды подключаются через резистор **2кОм**:

```
GPIO25 ──2кОм──── LED1 ──── GND
GPIO32 ──2кОм──── LED2 ──── GND
```

---

### DRV8833 (плата модуль)

Модуль DRV8833 — двухканальный драйвер моторов. Управляется напрямую от GPIO ESP32.

**Подключение питания:**

| Пин DRV8833 | Подключение |
|-------------|------------|
| VM | +5В (питание моторов) |
| VCC / 3V3 | 3.3В (если есть на плате) |
| GND | GND |
| STBY / nSLEEP | 3.3В через резистор 47кОм |

**Управление:**

| Пин DRV8833 | GPIO ESP32 |
|-------------|-----------|
| AIN1 | GPIO 18 |
| AIN2 | GPIO 19 |
| BIN1 | GPIO 23 |
| BIN2 | GPIO 13 |

**Выходы на моторы:**

| Пин DRV8833 | Назначение |
|-------------|-----------|
| AOUT1 | Мотор шторы 1 |
| AOUT2 | Мотор шторы 1 |
| BOUT1 | Мотор шторы 2 |
| BOUT2 | Мотор шторы 2 |

> Конденсатор 100мкФ между VM и GND как можно ближе к модулю — обязательно для защиты ESP32 от помех мотора.

---

### Герконы (концевые выключатели)

Active LOW — один провод на GPIO, второй на GND. Внутренняя подтяжка включена программно.

| GPIO | Назначение |
|------|-----------|
| GPIO 4 | Геркон 1 — шторы внизу |
| GPIO 16 | Геркон 1 — шторы сверху |
| GPIO 15 | Геркон 2 — шторы внизу |
| GPIO 27 | Геркон 2 — шторы сверху |

---

### Кнопки управления

Active LOW — один провод на GPIO, второй на GND. Внутренняя подтяжка включена программно.

| GPIO | Кнопка |
|------|--------|
| GPIO 26 | Шторы 1 — вверх |
| GPIO 17 | Шторы 1 — вниз |
| GPIO 33 | Шторы 2 — вверх |
| GPIO 22 | Шторы 2 — вниз |
| GPIO 21 | СТОП |

- **Короткое нажатие СТОП** — аварийная остановка всех моторов
- **Удержание СТОП 15 сек** — сброс привязки и режим комиссионирования (оба LED мигают)

---

### Разъём подключения проводов (сверху вниз)

| Пин | Назначение |
|-----|-----------|
| 1 (верхний) | GPIO 27 — геркон «вверх» шторы 2 |
| 2 | GPIO 15 — геркон «вниз» шторы 2 |
| 3 | BOUT1 — мотор шторы 2 |
| 4 | BOUT2 — мотор шторы 2 |
| 5 | AOUT2 — мотор шторы 1 |
| 6 | AOUT1 — мотор шторы 1 |
| 7 | GPIO 16 — геркон «вверх» шторы 1 |
| 8 | GPIO 4 — геркон «вниз» шторы 1 |
| 9 (нижний) | GND — общий провод |

---

### Разъём программатора (USB-UART)

Вывести на разъём следующие пины для прошивки:

| Пин | Назначение |
|-----|-----------|
| 3.3В | Питание |
| GND | Земля |
| TX | GPIO 1 (UART TX) |
| RX | GPIO 3 (UART RX) |
| EN | Сброс ESP32 |
| RST | GPIO 0 (режим прошивки) |

Подключение программатора:

| Программатор | ESP32 |
|-------------|-------|
| TX | RX (GPIO3) |
| RX | TX (GPIO1) |
| EN | GPIO0 |
| RST | EN |
| VCC | 3.3В |
| GND | GND |

---

## Matter комиссионирование

Параметры (задаются в исходнике CHIPDeviceConfig.h):
- discriminator: **4001**
- passcode: **20202021**

Получить QR код:
```bash
python3 ~/esp/esp-matter/connectedhomeip/connectedhomeip/src/setup_payload/python/generate_setup_payload.py \
    -d 4001 -p 20202021 -vid 65521 -pid 32768 -cf 0 -dm 2
```

Добавление в Home Assistant: Настройки → Устройства и службы → Matter (BETA) → Добавить устройство.

---

## Установка и сборка с нуля

### 1. Зависимости

```bash
# ESP-IDF v5.1.4
git clone --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf
cd ~/esp/esp-idf && git checkout v5.1.4 && ./install.sh
. ~/esp/esp-idf/export.sh

# ESP-Matter v1.3
git clone --recursive https://github.com/espressif/esp-matter.git ~/esp/esp-matter
cd ~/esp/esp-matter && git checkout v1.3 && ./install.sh
. ~/esp/esp-matter/export.sh
```

### 2. Изменить discriminator в исходнике

```bash
# Найти строку CHIP_DEVICE_CONFIG_USE_TEST_SETUP_DISCRIMINATOR
# и поменять значение на нужное (в hex)
nano ~/esp/esp-matter/connectedhomeip/connectedhomeip/src/include/platform/CHIPDeviceConfig.h
```

### 3. Клонировать и собрать проект

```bash
git clone https://github.com/Dangerwind/blinds_matter.git ~/esp/blinds_matter
cd ~/esp/blinds_matter
chmod +x build_fixed.sh
./build_fixed.sh
```

> ⚠️ На macOS используйте `build_fixed.sh` вместо `idf.py build` из-за бага chip_gn.

### 4. Найти порт программатора

```bash
ls /dev/cu.*          # macOS
ls /dev/ttyUSB*       # Linux
```

### 5. Прошивка

```bash
esptool.py -p /dev/cu.usbserial-110 -b 460800 \
    --before default_reset --after hard_reset \
    write_flash \
    0x1000  build/bootloader/bootloader.bin \
    0x8000  build/partition_table/partition-table.bin \
    0x20000 build/blinds_matter.bin
```

### 6. Мониторинг

```bash
idf.py -p /dev/cu.usbserial-110 monitor
```

### 7. Полный сброс (при проблемах)

```bash
idf.py -p /dev/cu.usbserial-110 erase-flash
# затем повторить шаг 5
```

---

## Структура проекта

```
blinds_matter/
├── main/
│   ├── config.h              # GPIO пины и константы
│   ├── motor_control.cpp/h   # Управление моторами, watchdog
│   ├── button_handler.cpp/h  # Кнопки, режим перекомиссионирования
│   └── matter_handler.cpp/h  # Matter endpoints
├── sdkconfig.defaults
├── build_fixed.sh            # Скрипт сборки для macOS
└── README.md
```
