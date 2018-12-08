#!/bin/bash
set -o errexit

make test_prog

rm -rf m5out*
mkdir -p work

#preds=(isltage5 isltage8 isltage13 isltage16)
#mults=(16 17 18 19 20)
preds=(tagelsc5)
mults=(16 17 18 19 20)

all_p=()
all_m=()
for pred in "${preds[@]}"; do
  for mult in "${mults[@]}"; do
    all_p+=($pred)
    all_m+=($mult)
  done
done

echo "pred,mult,storage,lookups,incorrect" > results.csv

parallel=8
length=${#all_p[@]}
for ((i=0; i<$length; )) {
    for ((j=0; j<$parallel && i<$length; j++)) {
        sh run_one.sh ${all_p[$i]} ${all_m[$i]} &
        i=$((i+1))
    }
    wait
    echo "$i/$length"
}
