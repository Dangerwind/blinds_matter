#include "matter_handler.h"
#include "motor_control.h"
#include "config.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_matter.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <app/server/Server.h>

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

static const char *TAG = "matter";
static uint16_t s_ep_id[4] = {};

// ── LED мигание при ожидании комиссионирования ────────────────────────────────
static volatile bool s_commissioning_mode = false;
static TaskHandle_t  s_blink_task = nullptr;

static void _blink_task(void *arg)
{
    ESP_LOGI(TAG, "Blink task started — waiting for commissioning");
    while (s_commissioning_mode) {
        gpio_set_level(LED1_STATUS, 1);
        gpio_set_level(LED2_STATUS, 1);
        vTaskDelay(pdMS_TO_TICKS(125));
        gpio_set_level(LED1_STATUS, 0);
        gpio_set_level(LED2_STATUS, 0);
        vTaskDelay(pdMS_TO_TICKS(125));
    }
    // Выключить LED когда комиссионирование завершено
    gpio_set_level(LED1_STATUS, 0);
    gpio_set_level(LED2_STATUS, 0);
    ESP_LOGI(TAG, "Blink task stopped — commissioned");
    s_blink_task = nullptr;
    vTaskDelete(nullptr);
}

static void _start_blink(void)
{
    s_commissioning_mode = true;
    if (s_blink_task == nullptr) {
        xTaskCreate(_blink_task, "blink_task", 2048, nullptr, 3, &s_blink_task);
    }
}

static void _stop_blink(void)
{
    s_commissioning_mode = false;
    // Задача завершится сама на следующей итерации
}

// ── Matter callbacks ──────────────────────────────────────────────────────────

static void _set_endpoint_off(int slot)
{
    esp_matter_attr_val_t val = esp_matter_bool(false);
    attribute::update(s_ep_id[slot], OnOff::Id, OnOff::Attributes::OnOff::Id, &val);
}

static esp_err_t _attr_update_cb(attribute::callback_type_t type,
                                  uint16_t endpoint_id, uint32_t cluster_id,
                                  uint32_t attribute_id, esp_matter_attr_val_t *val,
                                  void *priv_data)
{
    if (type != PRE_UPDATE) return ESP_OK;
    if (cluster_id   != OnOff::Id)                    return ESP_OK;
    if (attribute_id != OnOff::Attributes::OnOff::Id) return ESP_OK;

    bool on = val->val.b;

    int slot = -1;
    for (int i = 0; i < 4; i++) {
        if (s_ep_id[i] == endpoint_id) { slot = i; break; }
    }
    if (slot < 0) return ESP_OK;

    ESP_LOGI(TAG, "Matter endpoint %u (slot %d) OnOff=%s", endpoint_id, slot, on ? "ON" : "OFF");

    if (on) {
        int opposite = (slot % 2 == 0) ? slot + 1 : slot - 1;
        _set_endpoint_off(opposite);

        motor_channel_t ch  = (slot < 2) ? MOTOR_CHANNEL_1 : MOTOR_CHANNEL_2;
        motor_dir_t     dir = (slot % 2 == 0) ? MOTOR_DIR_OPEN : MOTOR_DIR_CLOSE;
        motor_stop(ch);
        motor_start(ch, dir);

        if (!motor_get_state(ch)->is_running) {
            val->val.b = false;
        }
    } else {
        motor_channel_t ch = (slot < 2) ? MOTOR_CHANNEL_1 : MOTOR_CHANNEL_2;
        motor_stop(ch);
    }

    return ESP_OK;
}

static esp_err_t _identify_cb(identification::callback_type_t type,
                               uint16_t endpoint_id, uint8_t effect_id,
                               uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Matter identify endpoint %u", endpoint_id);
    return ESP_OK;
}

static void _event_cb(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg)
{
    if (event->Type == chip::DeviceLayer::DeviceEventType::kCommissioningComplete) {
        ESP_LOGI(TAG, "Commissioning complete — stopping blink");
        _stop_blink();
    }
}

static endpoint_t *_create_endpoint(node_t *node, int slot, const char *label)
{
    on_off_plugin_unit::config_t cfg;
    endpoint_t *ep = on_off_plugin_unit::create(node, &cfg, ENDPOINT_FLAG_NONE, nullptr);
    if (!ep) { ESP_LOGE(TAG, "Failed to create endpoint '%s'", label); return nullptr; }
    s_ep_id[slot] = endpoint::get_id(ep);
    ESP_LOGI(TAG, "Endpoint '%s' ID=%u", label, s_ep_id[slot]);
    return ep;
}

void matter_handler_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    node::config_t node_cfg;
    node_t *node = node::create(&node_cfg, _attr_update_cb, _identify_cb);
    if (!node) { ESP_LOGE(TAG, "Failed to create Matter node"); abort(); }

    _create_endpoint(node, 0, "Blinds1-Open");
    _create_endpoint(node, 1, "Blinds1-Close");
    _create_endpoint(node, 2, "Blinds2-Open");
    _create_endpoint(node, 3, "Blinds2-Close");

    ESP_LOGI(TAG, "Matter node ready with 4 endpoints");
}

void matter_handler_start(void)
{
    esp_err_t err = esp_matter::start(_event_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_matter::start failed: %s", esp_err_to_name(err));
        abort();
    }

    // Проверить — есть ли уже fabric (устройство уже закомиссионировано)?
    uint8_t fabric_count = chip::Server::GetInstance().GetFabricTable().FabricCount();
    ESP_LOGI(TAG, "Fabric count: %u", fabric_count);
    if (fabric_count == 0) {
        ESP_LOGW(TAG, "No fabric — starting commissioning blink");
        _start_blink();
    }

    ESP_LOGI(TAG, "Matter running");
}

void matter_report_motor_stopped(int channel)
{
    int base = (channel == 1) ? 0 : 2;
    for (int i = base; i < base + 2; i++) {
        esp_matter_attr_val_t val = esp_matter_bool(false);
        attribute::update(s_ep_id[i], OnOff::Id, OnOff::Attributes::OnOff::Id, &val);
    }
    ESP_LOGI(TAG, "Reported motor channel %d stopped to Matter", channel);
}
