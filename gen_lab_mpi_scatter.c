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


	if(MPI_Init(&argc,&argv))
	{
		perror("MPI_Init failed");
		return EXIT_FAILURE;
	}

	int rank;
	int size;

	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&size);


	//TODO ceci doit etre seulement fait par rank == 0 ?

	int i = 0, j = 0, nbilots = NBILOTS/size;


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

	int (*l)[M] = malloc(sizeof(int[N][M]));

	srand( time(0) );


	int taille_chunk = N / size;
	int taille_dernier_chunk = N - (taille_chunk * (size - 1));

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
	/*	int i;
		for(i=1;i<size-1;i++)
		{
			if(MPI_Send(&l[i*taille_chunk],taille_chunk*M,MPI_INT,i,i,MPI_COMM_WORLD) != MPI_SUCCESS)
			{
				perror("Erreur ssend");
				MPI_Finalize();
				if(rank == 0)
					free(l);

				exit(EXIT_FAILURE);
			}
		}

		if(MPI_Send(&l[i*taille_chunk],taille_dernier_chunk*M,MPI_INT,i,i,MPI_COMM_WORLD) != MPI_SUCCESS)
		{
			perror("Erreur ssend");
			MPI_Finalize();
			if(rank == 0)
				free(l);

			exit(EXIT_FAILURE);
		}


	}*/

	int taille_chunk_courant;
	if(rank == size-1)
	{
		taille_chunk_courant = taille_dernier_chunk;
	}
	else
	{
		taille_chunk_courant = taille_chunk;
	}

        int (*chunk)[M] = malloc(sizeof(int[taille_chunk_courant][M]));
	int *scount = malloc(sizeof(int) * size);
	int *displs = malloc(sizeof(int) * size);

	for(i=0;i<size-1;i++)
	{
		scount[i] = taille_chunk * M;
		printf("%d taille %d : %d\n",rank,i,scount[i]);
		displs[i] = i * taille_chunk * M;
		printf("%d dis %d : %d\n",rank,i,displs[i]);
	}

	scount[i] = taille_dernier_chunk * M;
		printf("%d taille %d : %d\n",rank,i,scount[i]);
	displs[i] = i * taille_chunk * M;
		printf("%d dis %d : %d\n",rank,i,displs[i]);

	if(MPI_Scatterv(l,scount,displs,MPI_INT,chunk,taille_chunk_courant,MPI_INT,0,MPI_COMM_WORLD) != MPI_SUCCESS)
	{
			perror("Erreur recv");
			MPI_Finalize();
			if(rank == 0)
				free(l);

			free(chunk);

			exit(EXIT_FAILURE);
	}

/*
	if(rank == 0)
	{
		memcpy(chunk,l,taille_chunk_courant*M*sizeof(int));	
	}
	else
	{
		if(MPI_Recv(chunk,taille_chunk_courant*M,MPI_INT,0,rank,MPI_COMM_WORLD,&status) != MPI_SUCCESS)
		{
			perror("Erreur recv");
			MPI_Finalize();
			if(rank == 0)
				free(l);

			free(chunk);

			exit(EXIT_FAILURE);
		}
	}
*/

	/* place <nbilots> ilots aleatoirement a l'interieur du laby */
	for( ; nbilots ; nbilots-- )
	{
		i = rand()%(taille_chunk_courant-4) + 2;
		j = rand()%(M-4) + 2;
		chunk[i][j] = 0;
	}


	//TODO affichage que par le 0

#ifdef AFFICHE
	initgraph(M*(CARRE+INTER), taille_chunk_courant*(CARRE+INTER));
	for( int i=0 ; i<taille_chunk_courant ; i++ )
		for( int j=0 ; j<M ; j++ )
			if( chunk[i][j]==0 )
				affichecarre(i,j);
	refresh();
#endif /* AFFICHE */


	generation(taille_chunk_courant,M,chunk);

/*
	if(rank != 0)
	{
		if(MPI_Send(chunk,taille_chunk_courant*M,MPI_INT,0,rank,MPI_COMM_WORLD) != MPI_SUCCESS)
		{
			perror("Erreur ssend");
			MPI_Finalize();
			if(rank == 0)
				free(l);

			free(chunk);

			exit(EXIT_FAILURE);
		}

	}
	else
	{
		memcpy(l,chunk,taille_chunk_courant*M*sizeof(int));

		for(i=1;i<size-1;i++)
		{
			if(MPI_Recv(&l[i*taille_chunk],taille_chunk*M,MPI_INT,i,i,MPI_COMM_WORLD,&status) != MPI_SUCCESS)
			{
				perror("Erreur recv");
				MPI_Finalize();
				if(rank == 0)
					free(l);

				free(chunk);

				exit(EXIT_FAILURE);
			}
		}

		if(MPI_Recv(&l[i*taille_chunk],taille_dernier_chunk*M,MPI_INT,i,i,MPI_COMM_WORLD,&status) != MPI_SUCCESS)
		{
			perror("Erreur recv");
			MPI_Finalize();
			if(rank == 0)
				free(l);

			free(chunk);

			exit(EXIT_FAILURE);
		}

*/
	if(MPI_Gatherv(chunk,taille_chunk_courant,MPI_INT,l,scount,displs,MPI_INT,0,MPI_COMM_WORLD) != MPI_SUCCESS)
	{
			perror("Erreur recv");
			MPI_Finalize();
			if(rank == 0)
				free(l);

			free(chunk);

			exit(EXIT_FAILURE);
	}



	if(rank == 0)
	{
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
			if(rank == 0)
				free(l);

			free(chunk);
			exit(EXIT_FAILURE);
		}
		x = M;
		if(write( f, &x, sizeof(int) ) == -1)
		{
			perror("Erreur write");
			MPI_Finalize();
			if(rank == 0)
				free(l);

			free(chunk);

			exit(EXIT_FAILURE);
		}
		if(write( f, l, N*M*sizeof(int) ) == -1)
		{
			perror("Erreur write");
			MPI_Finalize();
			if(rank == 0)
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

	if(rank == 0)
		free(l);

	free(chunk);

	if(rank == 0)
		printf("%f\n",MPI_Wtime() - temps);

	return EXIT_SUCCESS;
}

