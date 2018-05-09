#! /bin/bash

if [ -f res_100 ]
then
	rm cpu_res_*
fi

if [ -f graph.plot ]
then
	rm cpu_graph.plot
fi

nbilot=20

iteration=20
taille=600

if [ $# -eq 1 ]
then
	nbilot=$3
fi

coeur=1
while [ $coeur -le 4 ]
do
	i=0
	while [ $i -lt $iteration ]
	do
		mpirun ./gen_lab $nbilot $taille $taille >> cpu_res_$coeur
		mpirun -n $coeur ./gen_lab_mpi $nbilot $taille $taille >> cpu_res_$coeur
		i=$(($i + 1))
	done

awk -v iteration=$iteration -v coeur=$coeur -f calcul_cpu.awk cpu_res_$coeur >> cpu_graph.plot
echo $coeur

coeur=$(($coeur + 1))
done
