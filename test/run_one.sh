#!/bin/bash
set -o errexit

outdir="m5out_$1_$2"
../build/X86/gem5.opt -d $outdir ../configs/pred/pred.py test_prog --mult $2 --pred $1 &> /dev/null
storage=`grep "storage=" ${outdir}/config.ini | grep -v "storage=0" | cut -d'=' -f 2`
lookups=`grep "cpu.branchPred.lookups" ${outdir}/stats.txt | awk '{print $2}'`
incorrect=`grep "cpu.branchPred.condIncorrect" ${outdir}/stats.txt | awk '{print $2}'`

rm -rf $outdir
echo "$1,$2,$storage,$lookups,$incorrect" >> results.csv
