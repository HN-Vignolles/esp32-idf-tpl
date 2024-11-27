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
