#include <stdio.h>
#include <time.h>
#include <stdlib.h>


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <mpi.h>

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


int main(int argc, char* argv[argc+1])
{
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

	int i = 0, j = 0, nbilots = NBILOTS, nbcons;
#ifdef AFFICHE
	int ref=REFRESH;
#endif /* AFFICHE */

	if( argc > 1 )
		nbilots = strtoull(argv[1], 0, 0);

	/* taille du labyrinthe : */
	/* hauteur : */
	size_t N = 400;
	if( argc > 2 )
		N = strtoull(argv[2], 0, 0);
	/* largeur : */
        size_t M = 600;
	if( argc > 3 )
		N = strtoull(argv[3], 0, 0);
        int (*l)[M] = malloc(sizeof(int[N][M]));

	srand( time(0) );

	/* initialise l : murs autour, vide a l'interieur */
	for( i=0 ; i<N ; i++ )
		for( j=0 ; j<M ; j++ )
			if( i==0 || i==N-1 ||j==0 || j==M-1 )
			{
				l[i][j] = 0; /* mur */
			}
			else
				l[i][j] = 1; /* vide */

	/* place <nbilots> ilots aleatoirement a l'interieur du laby */
	for( ; nbilots ; nbilots-- )
	{
		i = rand()%(N-4) + 2;
		j = rand()%(M-4) + 2;
		l[i][j] = 0;
	}

#ifdef AFFICHE
	initgraph(M*(CARRE+INTER), N*(CARRE+INTER));
	for( int i=0 ; i<N ; i++ )
		for( int j=0 ; j<M ; j++ )
			if( l[i][j]==0 )
				affichecarre(i,j);
	refresh();
#endif /* AFFICHE */



	/* initialise les cases constructibles */
	nbcons = 0;
	for( int i=1 ; i<N-1 ; i++ )
		for( int j=1 ; j<M-1 ; j++ )
			if( estconstructible( N, M, l, i, j ) )
			{
				l[i][j] = -1;
				nbcons++;
			}
	/* supprime quelques cases constructibles sur les bords */
	for( int i=1 ; i<N-1 ; i++ )
	{
		if( l[i][1] == -1 && (rand()%PROBPASCONS) && nbcons>(CONSMIN*2) )
		{
			l[i][1] = 1;
			nbcons--;
		}
		if( l[i][M-2] == -1 && (rand()%PROBPASCONS) && nbcons>(CONSMIN*2) )
		{
			l[i][M-2] = 1;
			nbcons--;
		}
	}
  	for( int j=1 ; j<M-1 ; j++ )
  	{
		if( l[1][j] == -1 && (rand()%PROBPASCONS) && nbcons>CONSMIN )
		{
			l[1][j] = 1;
			nbcons--;
		}
		if( l[N-2][j] == -1 && (rand()%PROBPASCONS) && nbcons>CONSMIN )
		{
			l[N-2][j] = 1;
			nbcons--;
		}
	}

	printf("Avant\n");



	int taille_chunk = N / size;
	int taille_dernier_chunk = N - (taille_chunk * (size - 1));

	/*
	MPI_Datatype chunk_type;
	MPI_Datatype dernier_chunk_type;
	if(MPI_Type_vector(N,taille_chunk,M,MPI_INT,&chunk_type) != MPI_SUCCESS)
	{
		errno("Erreur MPI_type_vector");
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}

	if(MPI_Type_vector(N,taille_dernier_chunk,M,MPI_INT,&dernier_chunk_type) != MPI_SUCCESS)
	{
		errno("Erreur MPI_type_vector");
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}*/

	if(rank == 0)
	{
		int i;
		for(i=0;i<size-1;i++)
		{
			if(MPI_Send(&l[i*taille_chunk],taille_chunk,MPI_INT,i,i,MPI_COMM_WORLD) != MPI_SUCCESS)
			{
				perror("Erreur ssend");
				MPI_Finalize();
				exit(EXIT_FAILURE);
			}
		}

		if(MPI_Send(&l[i*taille_chunk],taille_dernier_chunk,MPI_INT,i,i,MPI_COMM_WORLD) != MPI_SUCCESS)
		{
			perror("Erreur ssend");
			MPI_Finalize();
			exit(EXIT_FAILURE);
		}

		printf("Envoyé\n");

	}

	int taille_chunk_courant;
	if(rank == size-1)
	{
		taille_chunk_courant = taille_dernier_chunk;
	}
	else
	{
		taille_chunk_courant = taille_chunk;
	}

	int* chunk = (int*) malloc (taille_chunk_courant * sizeof(int));
	MPI_Status status;

	if(MPI_Recv(chunk,taille_chunk_courant,MPI_INT,0,rank,MPI_COMM_WORLD,&status) != MPI_SUCCESS)
	{
		perror("Erreur recv");
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}

	printf("Recu\n");
	
	/* boucle principale de génération */
	while( nbcons )
	{
		int r = 1 + rand() % nbcons;
		for( i=1 ; i<N-1 ; i++ )
		{
			for( j=1 ; j<M-1 ; j++ )
				if( l[i][j] == -1 )
					if( ! --r )
						break;
			if( ! r )
				break;
		}
		/* on construit en (i,j) */
		l[i][j] = 0;

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
				if( l[ii][jj]==1 && estconstructible(N, M, l, ii,jj) )
				{
					nbcons ++;
					l[ii][jj] = -1;
				}
				else if( l[ii][jj]==-1 && ! estconstructible(N, M, l, ii,jj) )
				{
					nbcons --;
					l[ii][jj] = 1;
				}
	}	/* fin while */

	if(MPI_Send(chunk,taille_chunk_courant,MPI_INT,0,rank,MPI_COMM_WORLD) != MPI_SUCCESS)
	{
		perror("Erreur ssend");
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}

	printf("Envoyé\n");


	if(rank == 0)
	{
		for(i=0;i<size-1;i++)
		{
			if(MPI_Recv(&l[i*taille_chunk],taille_chunk,MPI_INT,i,i,MPI_COMM_WORLD,&status) != MPI_SUCCESS)
			{
				perror("Erreur recv");
				MPI_Finalize();
				exit(EXIT_FAILURE);
			}
		}

		if(MPI_Recv(&l[i*taille_chunk],taille_dernier_chunk,MPI_INT,i,i,MPI_COMM_WORLD,&status) != MPI_SUCCESS)
		{
			perror("Erreur recv");
			MPI_Finalize();
			exit(EXIT_FAILURE);
		}

		printf("recu\n");



		printf("Ecriture\n");

		/* ENREGISTRE UN FICHIER. Format : LARGEUR(int), HAUTEUR(int), tableau brut (N*M (int))*/
		int f = open( "laby.lab", O_WRONLY|O_CREAT, 0644 );
		int x = N;
		if(write( f, &x, sizeof(int) ) == -1)
		{
			perror("Erreur write");
			exit(EXIT_FAILURE);
		}
		x = M;
		if(write( f, &x, sizeof(int) ) == -1)
		{
			perror("Erreur write");
			exit(EXIT_FAILURE);
		}
		if(write( f, l, N*M*sizeof(int) ) == -1)
		{
			perror("Erreur write");
			exit(EXIT_FAILURE);
		}
		close( f );

		printf("Attente\n");

	#ifdef AFFICHE
		refresh();
		waitgraph(); /* attend que l'utilisateur tape une touche */
		closegraph();
	#endif /* AFFICHE */

	}

	MPI_Finalize();

	free(l);
	return EXIT_SUCCESS;
}

