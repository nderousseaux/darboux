INPUT="../../partage/input"
MAIN="../../partage/derousseaux/main"
FILES="$INPUT/*.mnt"
HOSTFILE="../../partage/derousseaux/hostfile"

make
cp main $MAIN
cp hostfile $HOSTFILE
if [ "$#" -eq 1 ]; then
    FILES="$INPUT/$1.mnt"
fi
echo "file;time;index;openmp;nbVms" > res.csv

for i in `seq 0 5`
do
    for f in $FILES
    do
        name=`basename $f`

        export OMP_NUM_THREADS=1
        echo "Processing $name file with 1 vm... (without openmp) for the $i th time"
        time=`mpirun -n 1 --hostfile $HOSTFILE --map-by node --bind-to none $MAIN $INPUT/$name out/$name`
        if [ `diff out/${name} samples/${name} | wc -l` -ne 0 ]; then
            echo "ERRRRREUUUUUUUUR !! (le résultat est différent de l'exemple)"
        fi
        echo "$name;$time;$i;0;1" >> res.csv

        export OMP_NUM_THREADS=4
        for nbMachine in 1 2 6
        do
            echo "Processing $name file with $nbMachine vm...(with openmp) $i th time"
            time=`mpirun -n $nbMachine --hostfile $HOSTFILE --map-by node --bind-to none $MAIN $INPUT/$name out/$name`
            if [ `diff out/${name} samples/${name} | wc -l` -ne 0 ]; then
                echo "ERRRRREUUUUUUUUR !! (le résultat est différent de l'exemple)"
            fi
            echo "$name;$time;$i;1;$nbMachine" >> res.csv
        done
    done
done