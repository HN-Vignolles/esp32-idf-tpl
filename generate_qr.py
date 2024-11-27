#!/usr/bin/env python3

import qrcode
import subprocess
import re
import sys

# FIXME: random passwd?
user = "wifiprov"
password = "abcd1234"

def main():
    args = ['esptool.py', 'read_mac']
    result = subprocess.run(args, text=True, capture_output=True)
    print(result.stdout)

    if result.returncode != 0:
        sys.exit(1)

    #result = 'esptool.py v4.6.2\nFound 2 serial ports\nSerial port /dev/ttyUSB0\nConnecting....\nDetecting chip type... Unsupported detection protocol, switching and trying again...\nConnecting.......\nDetecting chip type... ESP32\nChip is ESP32-D0WDQ6 (revision v1.0)\nFeatures: WiFi, BT, Dual Core, 240MHz, VRef calibration in efuse, Coding Scheme None\nCrystal is 26MHz\nMAC: ec:62:60:a1:b2:5c\nUploading stub...\nRunning stub...\nStub running...\nMAC: 7c:9e:bd:5b:2d:80\nHard resetting via RTS pin...\n'
    m = re.search(r"MAC: (..:){3}(..):(..):(..)", result.stdout)
    qr_str = f'{{"ver":"v1","name":"PROV_{(m[2] + m[3] + m[4]).upper()}","username":"{user}","pop":"{password}","transport":"softap"}}'
    print(qr_str)
    qr = qrcode.QRCode(version=5, error_correction=qrcode.constants.ERROR_CORRECT_L, border=10)
    qr.add_data(qr_str)
    qr.make(fit=True)
    img = qr.make_image(fill_color="black", back_color="white")
    img.save('qrcode.png')

if __name__ == '__main__':
    main()
