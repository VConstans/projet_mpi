BEGIN	{somme=0; carre=0; nb=0;somme_mpi=0;carre_mpi=0;}

	{
		if(nb%2 == 0)
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
		printf("%f ", 3 * (sqrt(carre*iteration - somme * somme)/iteration));
		printf("%f ",somme_mpi/iteration);
		printf("%f\n", 3 * (sqrt(carre_mpi*iteration - somme_mpi * somme_mpi)/iteration));
	}
