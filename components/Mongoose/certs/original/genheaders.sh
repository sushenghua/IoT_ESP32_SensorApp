#!/bin/sh
cp ../../../../tools/f2sh ./
./f2sh -i ca.crt -n mqttCrt -o ../mqtt_crt.h
./f2sh -i ca.key -n mqttKey -o ../mqtt_key.h
