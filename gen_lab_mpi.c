/**
 * CONSTANS Victor
 * Code source de la génération de labyrinthe parallélisé avec MPI
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <mpi.h>
#include <string.h>

/* à commenter pour désactiver l'affichage */
#define AFFICHE

/* nombre de cases constructibles minimal */
#define CONSMIN 10
/* probabilité qu'une case du bord ne soit pas constructible */
#define PROBPASCONS 10
/* nombre d'ilots par defaut */
#define NBILOTS 20

#ifdef AFFICHE
#include "graph.h"

/* taille du mur (pixels) */
#define CARRE 2
/* espace entre les pixels */
#define INTER 0
/* nombre de couleurs */
#define NBCOL 120
/* taux de rafraichissement de l'affichage */
#define REFRESH 20

#ifdef AFFICHE
	int ref=REFRESH;
#endif /* AFFICHE */



/* fonction qui affiche un carre de cote CARRE dans la case (i,j) */
static
void affichecarre(int i, int j)
{
	for( int k=0 ; k<CARRE ; ++k )
		line(j*(CARRE+INTER)+k,i*(CARRE+INTER),
		     j*(CARRE+INTER)+k,i*(CARRE+INTER)+CARRE-1);
}
#endif /* AFFICHE */

/* fonction estconstructible : renvoie vrai si la case (i,j) est constructible */
static
int estconstructible( size_t N, size_t M, int l[N][M], int i, int j)
{
	if( l[i][j]==0 )
		return 0;
	else if( (l[i-1][j]==0 && l[i][j+1] && l[i][j-1] && l[i+1][j-1] && l[i+1][j] && l[i+1][j+1] )
		|| (l[i+1][j]==0 && l[i][j+1] && l[i][j-1] && l[i-1][j-1] && l[i-1][j] && l[i-1][j+1] )
		|| (l[i][j-1]==0 && l[i+1][j] && l[i-1][j] && l[i-1][j+1] && l[i][j+1] && l[i+1][j+1] )
		|| (l[i][j+1]==0 && l[i+1][j] && l[i-1][j] && l[i-1][j-1] && l[i][j-1] && l[i+1][j-1] )
		)
		return 1;
	else
		return 0;

}


/**
 * Fonction de génération du labyrinthe dans le chunk de taille taille_chunk_courant * M
 */
void generation(int taille_chunk_courant, int M, int chunk[taille_chunk_courant][M])
{
	/* initialise les cases constructibles */
	int nbcons = 0, i, j;
	for(i=1 ; i<taille_chunk_courant-1 ; i++ )
		for( int j=1 ; j<M-1 ; j++ )
			if( estconstructible( taille_chunk_courant, M, chunk, i, j ) )
			{
				chunk[i][j] = -1;
				nbcons++;
			}
	/* supprime quelques cases constructibles sur les bords */
	for(i=1 ; i<taille_chunk_courant-1 ; i++ )
	{
		if( chunk[i][1] == -1 && (rand()%PROBPASCONS) && nbcons>(CONSMIN*2) )
		{
			chunk[i][1] = 1;
			nbcons--;
		}
		if( chunk[i][M-2] == -1 && (rand()%PROBPASCONS) && nbcons>(CONSMIN*2) )
		{
			chunk[i][M-2] = 1;
			nbcons--;
		}
	}
  	for(j=1 ; j<M-1 ; j++ )
  	{
		if( chunk[1][j] == -1 && (rand()%PROBPASCONS) && nbcons>CONSMIN )
		{
			chunk[1][j] = 1;
			nbcons--;
		}
		if( chunk[taille_chunk_courant-2][j] == -1 && (rand()%PROBPASCONS) && nbcons>CONSMIN )
		{
			chunk[taille_chunk_courant-2][j] = 1;
			nbcons--;
		}
	}


	while( nbcons )
	{
		int r = 1 + rand() % nbcons;
		for( i=1 ; i<taille_chunk_courant-1 ; i++ )
		{
			for( j=1 ; j<M-1 ; j++ )
				if( chunk[i][j] == -1 )
					if( ! --r )
						break;
			if( ! r )
				break;
		}
		/* on construit en (i,j) */
		chunk[i][j] = 0;

#ifdef AFFICHE
		affichecarre(i,j);
		if( ! --ref )
		{
			refresh();
			ref = REFRESH;
		}
#endif /* AFFICHE */

		nbcons --;
		/* met a jour les 8 voisins */
		for( int ii=i-1 ; ii<=i+1 ; ++ii )
			for( int jj=j-1 ; jj<=j+1 ; ++jj )
				if( chunk[ii][jj]==1 && estconstructible(taille_chunk_courant, M, chunk, ii,jj) )
				{
					nbcons ++;
					chunk[ii][jj] = -1;
				}
				else if( chunk[ii][jj]==-1 && ! estconstructible(taille_chunk_courant, M, chunk, ii,jj) )
				{
					nbcons --;
					chunk[ii][jj] = 1;
				}
	}	/* fin while */

}



int main(int argc, char* argv[argc+1])
{
	double temps = MPI_Wtime();


	/* Initialisation de MPI et récupération des informations telle
	 * que le nombre de processus utilisés et le rang du
	 * processus courant
	 */
	if(MPI_Init(&argc,&argv))
	{
		perror("MPI_Init failed");
		return EXIT_FAILURE;
	}

	int rank;
	int size;

	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&size);


	int i = 0, j = 0, nbilots = NBILOTS/size;


	/* Récupération du nombre d'ilots total a placer dans le
	 * labyrinthe. On divise ce nombre par le nombre de porcessus
	 * pour obtenir le nombre d'ilot que chaque processus va devoir
	 * placer individuellement sur son morceau de labyrinthe pour
	 * pouvoir lancer la génération
	 */
	if( argc > 1 )
		nbilots = strtoull(argv[1], 0, 0)/size;

	/* taille du labyrinthe : */
	/* hauteur : */
	size_t N = 400;
	if( argc > 2 )
		N = strtoull(argv[2], 0, 0);
	/* largeur : */
        size_t M = 600;
	if( argc > 3 )
		M = strtoull(argv[3], 0, 0);

	/* Calcul de la hauteur des chunk sauf le dernier qui aura
	 * une taille égal ou plus petite que les autres, pour pouvoir
	 * gérer les cas où la hauteur du labyrinthe n'est pas un
	 * multiple du nombre de processus
	 */
	int taille_chunk = N / size;

	/*On arrondi la taille des chunk (sauf le dernier) à l'entier
	 * supérieur
	 */
	if(N % size != 0)
	{
		taille_chunk ++;
	}

	//Calcul de la hauteur du dernier chunk (le plus petit)
	int taille_dernier_chunk = N - (taille_chunk * (size - 1));

	int (*l)[M] = malloc(sizeof(int[taille_chunk * size][M]));

	srand( time(0) );



	// Initialisation du labyrinthe uniquement faite par le processus 0 (root)
	if(rank == 0)
	{

		/* initialise l : murs autour, vide a l'interieur */
		for( i=0 ; i<N ; i++ )
			for( j=0 ; j<M ; j++ )
				if( i==0 || i==N-1 ||j==0 || j==M-1 )
				{
					l[i][j] = 0; /* mur */
				}
				else
					l[i][j] = 1; /* vide */
	}

	/* Assignation à chaque processus de la hauteur du chunk 
	 * auquel il va devoir générer la parie de labyrinthe
	 */
	int taille_chunk_courant;
	if(rank == size-1)
	{
		taille_chunk_courant = taille_dernier_chunk;
	}
	else
	{
		taille_chunk_courant = taille_chunk;
	}

	// Allocation du chunk qui va servir à stocker le morceau de labyrinthe
        int (*chunk)[M] = malloc(sizeof(int[taille_chunk][M]));


	/**
	 * Distribution des morceaux de labyrinthe au différents processus
	 */
	if(MPI_Scatter(l,taille_chunk * M,MPI_INT,chunk,taille_chunk * M,MPI_INT,0,MPI_COMM_WORLD) != MPI_SUCCESS)
	{
			perror("Erreur MPI_Scatter");
			MPI_Finalize();
			free(l);
			free(chunk);

			exit(EXIT_FAILURE);
	}


	/* place <nbilots> ilots aleatoirement a l'interieur du laby */
	for( ; nbilots ; nbilots-- )
	{
		i = rand()%(taille_chunk_courant-4) + 2;
		j = rand()%(M-4) + 2;
		chunk[i][j] = 0;
	}



#ifdef AFFICHE
	initgraph(M*(CARRE+INTER), taille_chunk_courant*(CARRE+INTER));
	for( int i=0 ; i<taille_chunk_courant ; i++ )
		for( int j=0 ; j<M ; j++ )
			if( chunk[i][j]==0 )
				affichecarre(i,j);
	refresh();
#endif /* AFFICHE */


	// Génération du labyrinthe
	generation(taille_chunk_courant,M,chunk);


	/**
	 * Rassemblement des différents morceaux du labyrinthe pour que le processus 0 ai une version complete
	 * du labyrinthe
	 */
	if(MPI_Gather(chunk,taille_chunk * M,MPI_INT,l,taille_chunk * M,MPI_INT,0,MPI_COMM_WORLD) != MPI_SUCCESS)
	{
			perror("Erreur MPI_Gather");
			MPI_Finalize();
			free(l);
			free(chunk);

			exit(EXIT_FAILURE);
	}



	if(rank == 0)
	{
		/**
		 * Passe itérative du processus 0 sur les jonctions de morceaux
		 * pour combler le vide entre les morceaux
		 */
		int (*tmp)[M] = malloc(sizeof(int[8][M]));
		for(i=1;i<size;i++)
		{
			memcpy(tmp,&l[taille_chunk*i-4][0],8*M*sizeof(int));
			generation(8,M,tmp);
			memcpy(&l[taille_chunk*i-4][0],tmp,8*M*sizeof(int));
		}

		free(tmp);




		/* ENREGISTRE UN FICHIER. Format : LARGEUR(int), HAUTEUR(int), tableau brut (N*M (int))*/
		int f = open( "laby.lab", O_WRONLY|O_CREAT, 0644 );
		int x = N;
		if(write( f, &x, sizeof(int) ) == -1)
		{
			perror("Erreur write");
			MPI_Finalize();
			free(l);
			free(chunk);

			exit(EXIT_FAILURE);
		}
		x = M;
		if(write( f, &x, sizeof(int) ) == -1)
		{
			perror("Erreur write");
			MPI_Finalize();
			free(l);
			free(chunk);

			exit(EXIT_FAILURE);
		}
		if(write( f, l, N*M*sizeof(int) ) == -1)
		{
			perror("Erreur write");
			MPI_Finalize();
			free(l);
			free(chunk);

			exit(EXIT_FAILURE);
		}
		close( f );


	}
	#ifdef AFFICHE
		refresh();
		waitgraph(); /* attend que l'utilisateur tape une touche */
		closegraph();
	#endif /* AFFICHE */


	MPI_Finalize();

	free(l);

	free(chunk);

	if(rank == 0)
		printf("%f\n",MPI_Wtime() - temps);

	return EXIT_SUCCESS;
}

