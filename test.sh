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

    sleep 2
    echo "  "

    nbThread=4
    time2="$(sh -c "OMP_NUM_THREADS=${nbThread} time -f "%e" ./main input/${name} out/${name}> /dev/null" 2>&1)"
    time2=`echo $time2 | awk '{print $NF}'`
    echo "Execution time with $nbThread thread(s) was $time2 seconds"

    if [ `diff out/${name} samples/${name} | wc -l` -ne 0 ]; then
        echo "ERRRRREUUUUUUUUR !! (le résultat et différent de l'exemple)"
    fi

    echo "  "

done