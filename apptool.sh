#!/bin/bash

# color
ColorE='\033[0m'
PinkColorB='\033[1;35m'
RedColorB='\033[1;31m'
GreenColorB='\033[1;32m'
CyanColorB='\033[1;36m'

# vaiables
EncryptioinKey=encryptedFlash/key/flash_encryption_key.bin

BootloaderBin=build/bootloader/bootloader.bin
BootloaderBinEncrypted=build/bootloader/bootloaderEncrypted.bin

AppBin=build/SensorApp.bin
AppBinEncrypted=build/SensorAppEncrypted.bin

OTABin=build/partitions_two_ota.bin
OTABinEncrypted=build/partitions_two_ota_encrypted.bin

BootloadAddr=0x1000
AppAddr=0x10000
OTAAddr=0x8000

FlashSpeed=921600
Port=/dev/ttyUSB0


# format prompt
if [[ ( $# -eq 0) ]]; then
  printf "${PinkColorB}use this script as: apptool [m][e][f][k] \n${ColorE}"
  exit
fi


# promp beginning
  printf "${CyanColorB}------------------------------------------------------------------\n${ColorE}"
  printf "${CyanColorB}--------- apptool m/e/f/k (make/encrypt/flash/key) begin ---------\n${ColorE}"

# make binary when 'm' specified
if [[ ( $# -gt 0 && $1 == *"m"* ) ]]; then
  # printf "\n"
  printf "${PinkColorB}---------------------->>> build binary <<<------------------------\n${ColorE}"
  make
fi


# encrypt when 'e' specified
if [[ ( $# -gt 0 && $1 == *"e"* ) ]]; then
  # printf "\n"
  printf "${PinkColorB}--------------------->>> encrypt binary <<<-----------------------\n${ColorE}"
  if [ -f $BootloaderBin ]; then
    printf "${GreenColorB}------> encrypt bootloader bin ...\n${ColorE}"
    espsecure.py encrypt_flash_data --keyfile $EncryptioinKey --address $BootloadAddr -o  $BootloaderBinEncrypted $BootloaderBin
    printf "${ColorE}done encryption bootloader binary to: ${GreenColorB}$BootloaderBinEncrypted \n"
  else
    printf "${RedColorB}--->>> bootloader binary $BootloaderBin not exists \n${ColorE}"
  fi

  if [ -f $AppBin ]; then
    printf "${GreenColorB}------> encrypt app bin ...\n${ColorE}"
    espsecure.py encrypt_flash_data --keyfile $EncryptioinKey --address $AppAddr -o $AppBinEncrypted $AppBin
    printf "${ColorE}done encryption app binary to: ${GreenColorB}$AppBinEncrypted \n${ColorE}"
  else
    printf "${RedColorB}--->>> app binary $AppBin not exists \n${ColorE}"
  fi

  if [ -f $OTABin ]; then
    printf "${GreenColorB}------> encrypt OTA bin ...\n${ColorE}"
    espsecure.py encrypt_flash_data --keyfile $EncryptioinKey --address $OTAAddr -o $OTABinEncrypted $OTABin
    printf "${ColorE}done encryption OTA binary to: ${GreenColorB}$OTABinEncrypted \n${ColorE}"
  else
    printf "${RedColorB}--->>> app binary $OTABin not exists \n${ColorE}"
  fi

fi


# flash when 'f' specified
if [[ ( $# -gt 0 && $1 == *"f"* ) ]]; then
  # printf "\n"
  printf "${PinkColorB}---------------------->>> flash binary <<<------------------------\n${ColorE}"
  if [ ! -f $BootloaderBinEncrypted ]; then
    printf "${RedColorB}--->>> bootloader binary $BootloaderBinEncrypted not exists \n${ColorE}"
  elif [ ! -f $AppBinEncrypted ]; then
    printf "${RedColorB}--->>> app binary $AppBinEncrypted not exists \n${ColorE}"
  else
    printf "${GreenColorB}------> flash bootloader bin at $BootloadAddr, app bin at $AppAddr ...\n${ColorE}"
    esptool.py --chip esp32 --port $Port --baud $FlashSpeed --before default_reset --after hard_reset \
    write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect \
    $AppAddr $AppBinEncrypted

    # esptool.py --chip esp32 --port $Port --baud $FlashSpeed --before default_reset --after hard_reset \
    # write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect \
    # $BootloadAddr $BootloaderBinEncrypted $AppAddr $AppBinEncrypted

    # esptool.py --chip esp32 --port $Port --baud $FlashSpeed --before default_reset --after hard_reset \
    # write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect \
    # $BootloadAddr $BootloaderBinEncrypted $AppAddr $AppBinEncrypted $OTAAddr $OTABinEncrypted
  fi

  # if [ -f $BootloaderBinEncrypted ]; then
  #   printf "${GreenColorB}------> flash encrypted bootloader bin at 0x1000 ...\n${ColorE}"
  #   esptool.py --port $Port --baud $FlashSpeed write_flash 0x1000 $BootloaderBinEncrypted
  #   printf "${ColorE}done bootloader flash \n"
  # else
  #   printf "${RedColorB}--->>> bootloader binary $BootloaderBinEncrypted not exists \n${ColorE}"
  # fi

  # if [ -f $AppBinEncrypted ]; then
  #   printf "${GreenColorB}------> flash encrypted app bin at 0x10000 ...\n${ColorE}"
  #   esptool.py --port $Port --baud $FlashSpeed write_flash 0x10000 $AppBinEncrypted
  #   printf "${ColorE}done app flash \n${ColorE}"
  # else
  #   printf "${RedColorB}--->>> app binary $AppBinEncrypted not exists \n${ColorE}"
  # fi
fi

# burn flash encryption key when 'k' specified
if [[ ( $# -gt 0 && $1 == *"k"* ) ]]; then
  # printf "\n"
  printf "${PinkColorB}------------------>>> burn encryption key <<<---------------------\n${ColorE}"
  espefuse.py --port $Port burn_key flash_encryption $EncryptioinKey
fi



# done
  printf "${CyanColorB}--------- apptool m/e/f/k (make/encrypt/flash/key) done ----------\n${ColorE}"
  printf "${CyanColorB}------------------------------------------------------------------\n${ColorE}"
