#!/bin/sh
# cp ../../../../tools/f2sh ./
g++ -std=c++11 -o f2sh ../../../../tools/file2StrHeader.cpp -lssl -lcrypto

# mqtt user and password encryption
./f2sh -i mqtt_user   -n mqttUser   -o ../mqtt_user.h   -c aes_128_cbc -key CkECr+xGO4YKbicw -iv cBh++uagobC8s1bK -a false -note false
./f2sh -i mqtt_passwd -n mqttPasswd -o ../mqtt_passwd.h -c aes_128_cbc -key CkECr+xGO4YKbicw -iv cBh++uagobC8s1bK -a false -note false

# ca.crt encryption
./f2sh -i ca.crt -n mqttCrt -o ../mqtt_crt.h -c aes_128_cbc -key k4pR/+0gEhOHXucV -iv 22n+vbspOAsq8ECP
mv ski.h ../../../Common/

# ca.key export
./f2sh -i ca.key -n mqttKey -o ../mqtt_key.h
