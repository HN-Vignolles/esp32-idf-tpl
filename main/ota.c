// source: https://github.com/espressif/esp-idf/blob/release/v5.2/examples/system/ota/native_ota_example

#include <esp_ota_ops.h>
#include <esp_http_client.h>
#include <esp_tls.h>
#include <esp_log.h>
#include <esp_app_format.h>
#include <nvs_flash.h>
#include <string.h>

#define BUFFSIZE 1024
#define HASH_LEN 32 /* SHA-256 digest length */
#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

static const char *TAG = "OTA";

static char ota_write_data[BUFFSIZE + 1] = { 0 };
//extern const uint8_t server_cert_pem_start[] asm("_binary_server_cert_pem_start");
//extern const uint8_t server_cert_pem_end[] asm("_binary_server_cert_pem_end");

static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

esp_err_t perform_ota(void) {
    esp_err_t err;
    esp_ota_handle_t update_handle = 0;
    int retries = 10;

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);

    ESP_LOGI(TAG, "Configured OTA boot partition \'%s\' at offset 0x%08"PRIx32,
        configured->label, configured->address);
    ESP_LOGI(TAG, "Running partition \'%s\' type %d subtype %d (offset 0x%08"PRIx32")",
        running->label, running->type, running->subtype, running->address);
    ESP_LOGI(TAG, "Next partition for OTA: \'%s\'", update_partition->label);

    ESP_LOGI(TAG, "OTA_STR: \'%s\'", CONFIG_OTA_STR);
    
    esp_err_t ret;
    nvs_handle_t nvs_handle;
    size_t str_len = 0;
    char* str_val = NULL;
    ret = nvs_open_from_partition("nvs", "app", NVS_READONLY, &nvs_handle);  // Open the pre-filled NVS partition called "nvs", namespace = "app"
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
    } else {
        ret = nvs_get_str(nvs_handle, "ota_url", NULL, &str_len);  // get the length of the string
        ESP_ERROR_CHECK(ret);
        str_val = (char*) malloc(str_len);
        ret = nvs_get_str(nvs_handle, "ota_url", str_val, &str_len);
        ESP_ERROR_CHECK(ret);
        printf(">>>>> ota_url: %s\n", str_val);
        free(str_val);
    }

    esp_http_client_config_t config = {
        //.url = CONFIG_FIRMWARE_UPG_URL,
        .host = "192.168.1.74", /* FIXME: --> nvs */
        .port = 8088,
        .path = "/ota-1.bin",
        //.cert_pem = (char *)server_cert_pem_start,
        .skip_cert_common_name_check = true,
        .timeout_ms = 10 * CONFIG_OTA_RECV_TIMEOUT,
        .keep_alive_enable = true,
        //.event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client;
    ESP_LOGI(TAG, "trying %s:%d%s", config.host, config.port, config.path);

    while (retries--) {
        client = esp_http_client_init(&config);
        if (client == NULL) {
            ESP_LOGE(TAG, "Failed to initialise HTTP connection");
            continue;
        }
        err = esp_http_client_open(client, 0);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
            esp_http_client_cleanup(client);
            continue;
        } else {
            break;
        }
    }

    if (retries <= 0)
        return ESP_FAIL;
    esp_http_client_fetch_headers(client);
    int binary_file_length = 0;
    bool image_header_was_checked = false; //!< deal with all received packets

    while (1) {
        int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
        if (data_read < 0) {
            ESP_LOGE(TAG, "Error: SSL data read error");
            free(config[1].url);
            http_cleanup(client);
            return ESP_FAIL;
        } else if (data_read > 0) {
            if (image_header_was_checked == false) {
                esp_app_desc_t new_app_info;
                // version: set(PROJECT_VER "0.1.0") in root makefile
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
                    // check current version with downloading
                    memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                    ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

                    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
                    }

                    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
                    esp_app_desc_t invalid_app_info;
                    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
                    }

                    // check current version with last invalid partition
                    if (last_invalid_app != NULL) {
                        if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
                            ESP_LOGW(TAG, "New version is the same as invalid version.");
                            ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
                            ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
                            free(config[1].url);
                            http_cleanup(client);
                            return ESP_OK;
                        }
                    }

                    if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
                        ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
                        free(config[1].url);
                        http_cleanup(client);
                        return ESP_OK;
                    }

                    image_header_was_checked = true;

                    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        free(config[1].url);
                        http_cleanup(client);
                        esp_ota_abort(update_handle);
                        return ESP_FAIL;
                    }
                    ESP_LOGI(TAG, "esp_ota_begin succeeded");
                } else {
                    ESP_LOGE(TAG, "received package is not fit len");
                    free(config[1].url);
                    http_cleanup(client);
                    esp_ota_abort(update_handle);
                    return ESP_FAIL;
                }
            } /* (image_header_was_checked == false) */
            err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_write failure");
                free(config[1].url);
                http_cleanup(client);
                esp_ota_abort(update_handle);
                return ESP_FAIL;
            }
            binary_file_length += data_read;
            ESP_LOGD(TAG, "Written image length %d", binary_file_length);
        } else if (data_read == 0) {
           /*
            * As esp_http_client_read never returns negative error code, we rely on
            * `errno` to check for underlying transport connectivity closure if any
            */
            if (errno == ECONNRESET || errno == ENOTCONN) {
                ESP_LOGE(TAG, "Connection closed, errno = %d", errno);
                break;
            }
            if (esp_http_client_is_complete_data_received(client) == true) {
                ESP_LOGI(TAG, "Connection closed");
                break;
            }
        }
    }
    ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);
    if (esp_http_client_is_complete_data_received(client) != true) {
        ESP_LOGE(TAG, "Error in receiving complete file");
        free(config[1].url);
        http_cleanup(client);
        esp_ota_abort(update_handle);
        return ESP_FAIL;
    }

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        } else {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
        free(config[1].url);
        http_cleanup(client);
        return ESP_FAIL;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        free(config[1].url);
        http_cleanup(client);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Prepare to restart system!");
    free(config[1].url);
    esp_restart();
}
