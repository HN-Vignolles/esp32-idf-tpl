idf_component_register(SRCS "app_main.c" "prov.c"
#           ota.c
#           utils.c
#           EMBED_TXTFILES ../server_certs/server_cert.pem
            INCLUDE_DIRS ".")

nvs_create_partition_image(nvs ../nvs_data.csv FLASH_IN_PROJECT)

# set(image ../fatfs_image)
# fatfs_create_spiflash_image(storage ${image} FLASH_IN_PROJECT)
