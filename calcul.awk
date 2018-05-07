BEGIN	{somme=0; carre=0; nb=0;somme_mpi=0;carre_mpi=0;}

	{
		if(nb<iteration)
		{
			somme+=$1;
			carre+=$1*$1;
			nb++;
		}
		else
		{
			somme_mpi+=$1;
			carre_mpi+=$1*$1;
			nb++;
		}

	}

END	{
		printf("%s ",taille);
		printf("%f ",somme/iteration);
		printf("%f ", (sqrt(carre*iteration - somme * somme)/iteration));
		printf("%f ",somme_mpi/iteration);
		printf("%f\n", (sqrt(carre_mpi*iteration_mpi - somme_mpi * somme_mpi)/iteration));
	}
