#!/bin/bash



if [ ! -e generafile ];
then
    echo "Compilare generafile, eseguibile mancante!";
    exit 1
fi
if [ ! -e farm ];
then
    echo "Compilare farm, eseguibile mancante!"
    exit 1
fi

OFF='\033[0m'       # Text Reset

# Regular Colors
RED='\033[0;31m'          # Red
GREEN='\033[0;32m'        # Green
YELLOW='\033[0;33m'       # Yellow
BLUE='\033[0;34m'         # Blue
PURPLE='\033[0;35m'       # Purple
CYAN='\033[0;36m'         # Cyan

echo ""
echo -e "${PURPLE}Esecuzione normale, risultati su stdout${OFF}"
echo ""


j=1
for i in 100 150 19 116 2 1 117 3 5 17 4 16 19 8 10 111 12 13 14 15 18 20; do
    ./generafile file$i.dat $(($i*11 + $j*117)) > /dev/null
    j=$(($j+3))
done

mkdir -p testdir
mv file19.dat file8.dat testdir
mkdir -p testdir/testdir2
mv file111.dat file150.dat testdir/testdir2

# delaied execution by 200 millisec, with 8 threads and a queue 8 buckets long  
# valgrind is used to check for memory leak
valgrind --leak-check=full ./farm -n 4 -q 8 -t 200 -d testdir file* 

echo ""
echo -e "${GREEN}normalexe FINISH${OFF}"
echo ""


make cleanall 