#!/bin/sh
# MAX_THREAD=20
# NB_ESSAI=149

# #On note les entêtes
# echo -e 'TYPE;NBTHREAD;INDEXESSAI;TIME' > ./ex1.result

# #On essaye plusieurs fois pour avoir une moyenne
# for i in `seq 0 $NB_ESSAI`
# do
#     #Avec un nombre de threads croissant
#     for j in `seq 1 $MAX_THREAD`
#     do
#         ti=`/tmp/tp2_1_matmat.o 1 |head -n 2|tail -n 1|cut -d" " -f5`
#         echo -e "0;$j;$i;$ti" >> ./ex1.result

#         env OMP_SCHEDULE="static"
#         ti=`/tmp/tp2_1_matmat.o $j |head -n 2|tail -n 1|cut -d" " -f5`
#         echo -e "1;$j;$i;$ti" >> ./ex1.result

#         env OMP_SCHEDULE="dynamic"
#         ti=`/tmp/tp2_1_matmat.o $j |head -n 2|tail -n 1|cut -d" " -f5`
#         echo -e "2;$j;$i;$ti" >> ./ex1.result
    
#     done

# done

FILES="./input/*.mnt"

if [ "$#" -eq 1 ]; then
    FILES="./input/$1.mnt"
fi
make
for f in $FILES
do
    name=`basename $f`
    echo "Processing $name file..."
    nbThread=1
    time1="$(sh -c "OMP_NUM_THREADS=${nbThread} time -f "%e" ./main input/${name} out/${name}> /dev/null" 2>&1)"
    time1=`echo $time1 | awk '{print $NF}'`
    echo "Execution time with $nbThread thread(s) was $time1 seconds"
    if [ `diff out/${name} samples/${name} | wc -l` -ne 0 ]; then
        echo "ERRRRREUUUUUUUUR !! (le résultat et différent de l'exemple)"
    fi
    echo "  "

    nbThread=4
    time2="$(sh -c "OMP_NUM_THREADS=${nbThread} time -f "%e" ./main input/${name} out/${name}> /dev/null" 2>&1)"
    time2=`echo $time2 | awk '{print $NF}'`
    echo "Execution time with $nbThread thread(s) was $time2 seconds"

    if [ `diff out/${name} samples/${name} | wc -l` -ne 0 ]; then
        echo "ERRRRREUUUUUUUUR !! (le résultat et différent de l'exemple)"
    fi

done