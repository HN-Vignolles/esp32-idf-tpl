idf_component_register(SRCS "app_main.c" "prov.c"
#           ota.c
#           utils.c
#           EMBED_TXTFILES ../server_certs/server_cert.pem
            INCLUDE_DIRS "."
            REQUIRES lwip mbedtls driver
            PRIV_REQUIRES nvs_flash bootloader_support esp_timer esp_wifi fatfs
                app_update network_provisioning esp_http_client esp-tls)

nvs_create_partition_image(nvs ../nvs_data.csv FLASH_IN_PROJECT)

# set(image ../fatfs_image)
# fatfs_create_spiflash_image(storage ${image} FLASH_IN_PROJECT)
