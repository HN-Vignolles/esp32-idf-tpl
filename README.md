# Get Started (ESP-IDF)

## Prerequisites
- Build tools, CMake and Ninja to build a full Application for ESP32
  - `sudo apt-get install git python3 python3-pip`
  - `sudo apt-get install wget flex bison gperf python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0`

## ESP-IDF
**First time:**
```bash
mkdir ~/esp_git
cd ~/esp_git
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout vX.Y.Z  # e.g. v5.2.3
git submodule update --init --recursive
#conda deactivate  # if you are using some conda environment
./install.sh [all|esp32]
. ./export.sh
```

**Normal workflow**
```bash
cd ~/esp_git/esp-idf
. ./export.sh
cd /path/to/esp32-idf-tpl
idf.py build
```

&nbsp;

# Misc
## Server Certificates
```bash
openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout server_certs/server_key.pem -out server_certs/server_cert.pem  # Common Name = server IP
```
