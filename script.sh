#! /bin/bash

if [ -f res ]
then
	rm res
fi

nbilot=20
N=400
M=600

iteration=$1
coeur=$2

if [ $# -eq 3 ]
then
	nbilot=$3
fi

if [ $# -eq 4 ]
then
	N=$4
fi

if [ $# -eq 5 ]
then
	M=$5
fi

i=0
while [ $i -lt $iteration ]
do
	mpirun -n $2 ./gen_lab_mpi >> res
	i=$(($i + 1))
done


awk -f calcul.awk res
