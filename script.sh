#! /bin/bash

if [ -f res_100 ]
then
	rm res_*
fi

if [ -f graph.plot ]
then
	rm graph.plot
fi

nbilot=20

iteration=20
coeur=4

if [ $# -eq 1 ]
then
	nbilot=$3
fi

j=100
while [ $j -le 1000 ]
do
	i=0
	while [ $i -lt $iteration ]
	do
		mpirun ./gen_lab $nbilot $j $j >> res_$j
		mpirun -n $coeur ./gen_lab_mpi $nbilot $j $j >> res_$j
		i=$(($i + 1))
	done

awk -v iteration=$iteration -v taille=$j -f calcul.awk res_$j >> graph.plot
echo $j

j=$(($j + 100))
done
