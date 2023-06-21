#!/usr/bin/env python3
# Small script to install main script dependencies

# Author:   Renan Lavarec
# Nickname: Ti-R
# Website:  www.ti-r.com
# License MIT

import subprocess
import sys

subprocess.call([sys.executable, "-m", "pip", "install", "--user", "--upgrade", "pip"])

def install(package):
    subprocess.call([sys.executable, "-m", "pip", "install", "--user", package])

install('pick')