find . -name *.h -type f | xargs chmod u-x
find . -name *.cpp -type f | xargs chmod u-x
find . -name *.c -type f | xargs chmod u-x
find . -name *.mk -type f | xargs chmod u-x
find . -name *.txt -type f | xargs chmod a-x

