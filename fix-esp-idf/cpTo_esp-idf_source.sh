find . -name *.h -type f | xargs chmod u-x
#find . -name *.cpp -type f | xargs chmod u-x
# find . -name *.c -type f | xargs chmod u-x
chmod u-x *.c

cp spi_master.h ~/Projects/source/esp32/esp-idf/components/driver/include/driver/
cp spi_master.c ~/Projects/source/esp32/esp-idf/components/driver/
cp i2c.c        ~/Projects/source/esp32/esp-idf/components/driver/
