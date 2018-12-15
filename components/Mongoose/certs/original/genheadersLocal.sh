#!/bin/sh
# cp ../../../../tools/f2sh ./
g++ -std=c++11 -o f2sh ../../../../tools/file2StrHeader.cpp -lssl -lcrypto
./f2sh -i ca.crt -n mqttCrt -o ../mqtt_crt.h -c aes_128_cbc -key k4pR/+0gEhOHXucV -iv 22n+vbspOAsq8ECP
mv ski.h ../
./f2sh -i ca.key -n mqttKey -o ../mqtt_key.h
