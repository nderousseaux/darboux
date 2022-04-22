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
for f in $FILES
do
    name=`basename $f`

    for nbMachine in `seq 1 16`
    do
        echo "Processing $name file with $nbMachine vm..."
        mpirun -n $nbMachine --hostfile $HOSTFILE $MAIN $INPUT/$name out/$name
        if [ `diff out/${name} samples/${name} | wc -l` -ne 0 ]; then
            echo "ERRRRREUUUUUUUUR !! (le résultat et différent de l'exemple)"
        fi
    done
done