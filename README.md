Была задумка сделать устройство которое само поднимало/опускало бы рулонные шторы и интегрировалось в умный дом.

# ESP32 Blinds Controller with Matter

Контроллер рулонных штор на ESP32-WROOM-32 с интеграцией в умный дом через Matter (Home Assistant, Apple Home, Google Home).

---

[![Как поднимаются шторы](https://github.com/Dangerwind/blinds_matter/raw/main/img/show.gif)](https://github.com/Dangerwind/blinds_matter/blob/main/img/show.gif)

Устройство можно собрать на базе отладочной платы ESP32 (проще, но не компактно) или на модуле ESP32-WROOM-32 (как на фото).

[![Внешний вид, передняя часть](https://github.com/Dangerwind/blinds_matter/raw/main/img/pcb-up.jpg)](https://github.com/Dangerwind/blinds_matter/blob/main/img/pcb-up.jpg)
[![Внешний вид, задняя часть](https://github.com/Dangerwind/blinds_matter/raw/main/img/pcb-back.jpg)](https://github.com/Dangerwind/blinds_matter/blob/main/img/pcb-back.jpg)

Производство печатной платы дорого для разового проекта, поэтому всё собрано на макетке.

[![Макетка для ESP32](https://github.com/Dangerwind/blinds_matter/raw/main/img/maketka-esp32.jpg)](https://github.com/Dangerwind/blinds_matter/blob/main/img/maketka-esp32.jpg)

---

## Список компонентов

| Компонент | Кол-во |
|-----------|--------|
| Модуль ESP32-WROOM-32 | 1 |
| Тактовые кнопки IT-1102W (или любые другие) | 5 |
| Макетная плата для ESP32 5×7 см | 1 |
| Клеммник винтовой DG127-5.08-03P | 3 |
| Светодиоды SMD (любые, я использовал зелёные) | 2 |
| Резистор SMD 0805 2–3 кОм (для светодиодов) | 2 |
| Резистор SMD 0805 10 кОм (подтяжка ESP32) | 2 |
| Резистор SMD 0805 47 кОм (для DRV8833 nSLEEP) | 1 |
| Разъём PLS-6 (для программатора) | 1 |
| Плата DRV8833 | 1 |
| Конденсатор танталовый 100 мкФ 16В | 2 |
| Мотор-редуктор 6В 50 об/мин | 2 |
| Геркон нормально разомкнутый | 4 |
| Разъём Type-C на плате | 1 |
| Провод КСПВ 8×0.4 | по длине |
| Магниты (крепятся на нижний край шторы) | 2 |

[![Мотор-редуктор](https://github.com/Dangerwind/blinds_matter/raw/main/img/ga12-n20-geared-dc-motor-1.jpg)](https://github.com/Dangerwind/blinds_matter/blob/main/img/ga12-n20-geared-dc-motor-1.jpg)

> Если собираете на отладочной плате ESP32 — программатор не нужен, USB уже встроен.

---

## Схема включения

### Питание

Питание 5В подаётся через стабилизатор **ASM1117-3.3**, который даёт 3.3В для ESP32 и логики DRV8833.

```
5В ──┬── [ASM1117-3.3] ──┬── 3.3В → ESP32, DRV8833 (логика)
     |                   |
   100мкФ             100мкФ
  (танталовый)       (танталовый)
     |                   |
    GND                 GND

5В также → VM пин DRV8833 (питание моторов)
```

---

### ESP32-WROOM-32 — обязательные подтяжки

> Если собираете на отладочной плате — эти резисторы уже установлены, ничего делать не нужно.

Для модуля ESP32-WROOM-32 без этих резисторов ESP32 не запустится или не будет прошиваться:

```
3.3В ──10кОм──── EN      (включение модуля)
3.3В ──10кОм──── GPIO0   (режим работы при старте)
```

---

### Светодиоды

```
GPIO25 ──2кОм──── LED1 (+) ──── LED1 (-) ──── GND
GPIO32 ──2кОм──── LED2 (+) ──── LED2 (-) ──── GND
```

---

### DRV8833 — драйвер моторов

**Питание:**

| Пин DRV8833 | Подключение |
|-------------|------------|
| VM | +5В (питание моторов) |
| GND | GND |
| nSLEEP | 3.3В через резистор 47 кОм |

> Конденсатор 100 мкФ между VM и GND как можно ближе к модулю — защищает ESP32 от помех мотора.

**Управление от ESP32:**

| DRV8833 | GPIO ESP32 |
|---------|-----------|
| AIN1 | GPIO 18 |
| AIN2 | GPIO 19 |
| BIN1 | GPIO 23 |
| BIN2 | GPIO 13 |

**Выходы на моторы:**

| DRV8833 | Назначение |
|---------|-----------|
| AOUT1, AOUT2 | Мотор шторы 1 |
| BOUT1, BOUT2 | Мотор шторы 2 |

---

### Герконы

Нормально разомкнутые. Один провод на GPIO, второй на GND. Внутренняя подтяжка включается программно.

| GPIO | Назначение |
|------|-----------|
| GPIO 4 | Геркон шторы 1 — положение «вниз» |
| GPIO 16 | Геркон шторы 1 — положение «вверх» |
| GPIO 15 | Геркон шторы 2 — положение «вниз» |
| GPIO 27 | Геркон шторы 2 — положение «вверх» |

---

### Кнопки управления

Один провод на GPIO, второй на GND.

| GPIO | Кнопка |
|------|--------|
| GPIO 26 | Шторы 1 — вверх |
| GPIO 17 | Шторы 1 — вниз |
| GPIO 33 | Шторы 2 — вверх |
| GPIO 22 | Шторы 2 — вниз |
| GPIO 21 | СТОП |

- **Короткое нажатие СТОП** — аварийная остановка всех моторов
- **Удержание СТОП 15 сек** — сброс привязки и переход в режим комиссионирования (оба LED мигают 4 раза в секунду до подключения к умному дому)

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

Герконы и моторы соединяются одним 8-жильным проводом КСПВ 8×0.4. Земля у всех герконов общая — один провод на все четыре. Хватает 8 жил, но можно взять КСПВ 10×0.4 в зависимости от прокладки кабеля.

---

### Разъём программатора

Вывести на разъём PLS-6:

| Пин | Назначение |
|-----|-----------|
| 1 | 3.3В |
| 2 | GND |
| 3 | TX → RX (GPIO3) ESP32 |
| 4 | RX → TX (GPIO1) ESP32 |
| 5 | RST → GPIO0 ESP32 (режим прошивки) |
| 6 | EN → EN ESP32 (сброс) |

---

## Matter комиссионирование

Параметры устройства (задаются в `CHIPDeviceConfig.h`):
- discriminator: **4001**
- passcode: **20202021**

Получить QR код:
```bash
python3 ~/esp/esp-matter/connectedhomeip/connectedhomeip/src/setup_payload/python/generate_setup_payload.py \
    -d 4001 -p 20202021 -vid 65521 -pid 32768 -cf 0 -dm 2
```

Или сгенерировать QR по ссылке: `https://project-chip.github.io/connectedhomeip/qrcode.html` — вставить код вида `MT:Y.K90.......`

**Добавление в Home Assistant:** Настройки → Устройства и службы → Matter (BETA) → Добавить устройство → QR код или ручной код.

---

## Установка и сборка

### 1. Установить зависимости

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

### 2. Изменить discriminator (опционально)

Каждое устройство должно иметь уникальный discriminator:

```bash
# Найти строку CHIP_DEVICE_CONFIG_USE_TEST_SETUP_DISCRIMINATOR
# поменять значение на нужное в hex (например 4001 = 0xFA1)
nano ~/esp/esp-matter/connectedhomeip/connectedhomeip/src/include/platform/CHIPDeviceConfig.h
```

### 3. Клонировать и собрать проект

```bash
git clone https://github.com/Dangerwind/blinds_matter.git ~/esp/blinds_matter
cd ~/esp/blinds_matter
chmod +x build_fixed.sh
./build_fixed.sh
```

>  На macOS используйте `build_fixed.sh` вместо `idf.py build` из-за бага chip_gn.

### 4. Найти порт программатора

```bash
ls /dev/cu.*       # macOS → например /dev/cu.usbserial-110
ls /dev/ttyUSB*    # Linux
```

### 5. Прошить

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

## Монтаж

Герконы крепятся сверху и снизу окна. Когда штора доходит краем до геркона (на нижнем крае шторы закреплён магнит), мотор автоматически останавливается.

[![Общий вид](https://github.com/Dangerwind/blinds_matter/raw/main/img/all.jpg)](https://github.com/Dangerwind/blinds_matter/blob/main/img/all.jpg)
[![Крепление геркона](https://github.com/Dangerwind/blinds_matter/raw/main/img/gerkon.jpg)](https://github.com/Dangerwind/blinds_matter/blob/main/img/gerkon.jpg)
[![Крепление геркона 2](https://github.com/Dangerwind/blinds_matter/raw/main/img/gerkon-2.jpg)](https://github.com/Dangerwind/blinds_matter/blob/main/img/gerkon-2.jpg)
[![Крепление двигателя](https://github.com/Dangerwind/blinds_matter/raw/main/img/motor.jpg)](https://github.com/Dangerwind/blinds_matter/blob/main/img/motor.jpg)

Корпус устройства и все пластиковые детали напечатаны из PETG на 3D принтере. Все 3D модели в формате STL находятся в папке `STL`. В шторах нужно заменить механизм подъёма на напечатанный механизм с мотором.

---

*Весь код написан полностью с помощью ИИ.*
