FILES="./input/*.mnt"

if [ "$#" -eq 1 ]; then
    FILES="./input/$1.mnt"
fi
make
for f in $FILES
do
    name=`basename $f`


    nbThread=1
    echo "Processing $name file with $nbThread thead..."
    mpirun -n $nbThread main input/$name out/$name
    if [ `diff out/${name} samples/${name} | wc -l` -ne 0 ]; then
        echo "ERRRRREUUUUUUUUR !! (le résultat et différent de l'exemple)"
    fi
    nbThread=4
    echo "Processing $name file with $nbThread theads..."
    mpirun -n $nbThread main input/$name out/$name
    if [ `diff out/${name} samples/${name} | wc -l` -ne 0 ]; then
        echo "ERRRRREUUUUUUUUR !! (le résultat et différent de l'exemple)"
    fi

done