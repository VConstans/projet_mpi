BEGIN	{somme=0; carre=0; nb=0;}

	{
		somme+=$1;
		carre+=$1*$1;
		nb++;
	}

END	{
		printf("Moyenne : %f\n",somme/nb);
		printf("Ecart type : %f\n", (sqrt(carre*nb - somme * somme)/nb));
	}
