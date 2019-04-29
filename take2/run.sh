#!/bin/sh
set -e -x

. ~/pydev/dev/bin/activate

python scan.py --ipaddr 192.168.1.40 --prefix DMM 19 22
