#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/FreeRTOSConfig.h>

#include <esp_vfs.h>
#include <esp_vfs_fat.h>
#include <esp_log.h>
#include <esp_flash_partitions.h>
#include <esp_partition.h>
#include <nvs_flash.h>

#include "prov.h"
//#include "ota.h"

static const char *TAG = "app_main";

/* from esp-idf fatfs examples: */
const char *base_path = "/spiflash";                /*!< Mount path for the partition */
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE; /*!< Handle of the wear levelling library instance */
FILE *f;

volatile int pulse_detected = 0;
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    (void)(gpio_num);
    pulse_detected = 4;
}

void app_main(void)
{
    /* NVS */
    esp_err_t ret = nvs_flash_init();  // Initialize NVS partition
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase()); // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_init());  // Retry nvs_flash_init
    }
    ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle");  
    nvs_handle_t my_handle;
    size_t str_len = 0;
    char* str_val = NULL;
    ret = nvs_open_from_partition("nvs", "app", NVS_READONLY, &my_handle);  // Open the pre-filled NVS partition called "nvs", namespace = "app"
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "The NVS handle successfully opened");
        ESP_LOGI(TAG, "Reading values from NVS");
        ret = nvs_get_str(my_handle, "ota_url", NULL, &str_len);  // get the length of the string
        ESP_ERROR_CHECK(ret);
        str_val = (char*) malloc(str_len);
        ret = nvs_get_str(my_handle, "ota_url", str_val, &str_len);
        ESP_ERROR_CHECK(ret);
        printf("Custom ota_url: %s\n", str_val);
        free(str_val);
    }

    /* GPIO */
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<< GPIO_NUM_22);
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    gpio_config(&io_conf);
    gpio_install_isr_service(0); // e.g. ESP_INTR_FLAG_NMI
    gpio_isr_handler_add(GPIO_NUM_22, gpio_isr_handler, (void*)GPIO_NUM_22);

    int cnt = 0, tab = 0;
    uint8_t write_buf[2] = { 0x42, 0x33 };

    do_provisioning();
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

