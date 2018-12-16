#!/bin/bash

# color
ColorE='\033[0m'
PinkColorB='\033[1;35m'
RedColorB='\033[1;31m'
GreenColorB='\033[1;32m'
CyanColorB='\033[1;36m'

# vaiables
EncryptionKey=encryptedFlash/key/flash_encryption_key.bin

BootloaderBin=build/bootloader/bootloader.bin
BootloaderBinEncrypted=build/bootloader/bootloaderEncrypted.bin

AppBin=build/SensorApp.bin
AppBinEncrypted=build/SensorAppEncrypted.bin

BootloadFlashAddress=0x1000
AppFlashAddress=0x10000

FlashSpeed=921600
Port=/dev/ttyUSB0


# Ref: https://docs.espressif.com/projects/esp-idf/en/latest/security/flash-encryption.html#pregenerating-a-flash-encryption-key
#
# permanently disable plaintext serial-flashing: espefuse.py --port PORT write_protect_efuse FLASH_CRYPT_CNT
#
# increase FLASH_CRYPT_CNT efuse: espefuse.py burn_efuse FLASH_CRYPT_CNT
# Note: can be called only for 4 times, then encryption is permanently enabled
#       you cannot serial-flash with new plaintext data;
#       you can serial-flash with new encrypted data if you know the encryption key and make bin with that key

# format prompt
if [[ ( $# -eq 0) ]]; then
  printf "${PinkColorB}use this script as: apptool [k][m][e][f bootloader|app]  \n${ColorE}"
  exit
fi


# promp beginning
  printf "${CyanColorB}------------------------------------------------------------------\n${ColorE}"
  printf "${CyanColorB}--------- apptool k/m/e/f (key/make/encrypt/flash) begin ---------\n${ColorE}"


# burn flash encryption key when 'k' specified
if [[ ( $# -gt 0 && $1 == *"k"* ) ]]; then
  # printf "\n"
  printf "${PinkColorB}------------------>>> burn encryption key <<<---------------------\n${ColorE}"
  espefuse.py --port $Port burn_key flash_encryption $EncryptionKey
fi


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
    printf "${GreenColorB}------> encrypt bootloader binary ...\n${ColorE}"
    espsecure.py encrypt_flash_data --keyfile $EncryptionKey --address $BootloadFlashAddress \
    -o $BootloaderBinEncrypted $BootloaderBin
    printf "${ColorE}done encryption bootloader binary to: ${GreenColorB}$BootloaderBinEncrypted \n"
  else
    printf "${RedColorB}--->>> bootloader binary $BootloaderBin does not exist \n${ColorE}"
  fi

  if [ -f $AppBin ]; then
    printf "${GreenColorB}------> encrypt app binary ...\n${ColorE}"
    espsecure.py encrypt_flash_data --keyfile $EncryptionKey --address $AppFlashAddress \
    -o $AppBinEncrypted $AppBin
    printf "${ColorE}done encryption app binary to: ${GreenColorB}$AppBinEncrypted \n${ColorE}"
  else
    printf "${RedColorB}--->>> app binary $AppBin does not exist \n${ColorE}"
  fi

fi


# flash when 'f' specified
if [[ ( $# -eq 2 && $1 == *"f"* ) ]]; then
  # printf "\n"
  printf "${PinkColorB}---------------------->>> flash binary <<<------------------------\n${ColorE}"
  if [[ ( $2 == "bootloader" ) ]]; then
    if [ ! -f $BootloaderBinEncrypted ]; then
      printf "${RedColorB}--->>> bootloader binary $BootloaderBinEncrypted does not exist \n${ColorE}"
    else
      printf "${GreenColorB}------> flash encrypted bootloader at $BootloadFlashAddress ...\n${ColorE}"
      esptool.py --chip esp32 --port $Port --baud $FlashSpeed --before default_reset --after hard_reset \
      write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect \
      $BootloadFlashAddress $BootloaderBinEncrypted
    fi
  elif [[ ( $2 == "app" ) ]]; then
    if [ ! -f $AppBinEncrypted ]; then
      printf "${RedColorB}--->>> app binary $AppBinEncrypted does not exist \n${ColorE}"
    else
      printf "${GreenColorB}------> flash encrypted app at $AppFlashAddress ...\n${ColorE}"
      esptool.py --chip esp32 --port $Port --baud $FlashSpeed --before default_reset --after hard_reset \
      write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect \
      $AppFlashAddress $AppBinEncrypted
      # esptool.py --port $Port --baud $FlashSpeed write_flash $AppFlashAddress $AppBinEncrypted
    fi
  fi
fi


# done
  printf "${CyanColorB}--------- apptool k/m/e/f (key/make/encrypt/flash) done ----------\n${ColorE}"
  printf "${CyanColorB}------------------------------------------------------------------\n${ColorE}"
