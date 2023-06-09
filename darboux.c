// fonction de calcul principale : algorithme de Darboux
// (remplissage des cuvettes d'un MNT)
#include <string.h>
#include <omp.h>
#include <mpi.h>

#include "check.h"
#include "type.h"
#include "darboux.h"


// si ce define n'est pas commenté, l'exécution affiche sur stderr la hauteur
// courante en train d'être calculée (doit augmenter) et l'itération du calcul
// #define DARBOUX_PPRINT

#define PRECISION_FLOTTANT 1.e-5

// pour accéder à un tableau de flotant linéarisé (ncols doit être défini) :
#define WTERRAIN(w,i,j) (w[(i)*ncols+(j)])

// calcule la valeur max de hauteur sur un terrain
float max_terrain(const mnt *restrict m){
  float max = m->terrain[0];
  #pragma omp parallel for schedule(static) reduction(max:max)
  for(int i = 0 ; i < m->ncols * m->nrows ; i++)
    if(m->terrain[i] > max)
      max = m->terrain[i];
  return(max);
}

float *init_W(const mnt *restrict m){
  const int ncols = m->ncols, nrows = m->nrows;
  float *restrict W;
  CHECK((W = malloc(ncols * nrows * sizeof(float))) != NULL);

  // initialisation W
  float max = max_terrain(m) + 10.;

  MPI_Allreduce(&max, &max, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);
  
  #pragma omp parallel for schedule(static, 2)
  for(int i = 0 ; i < nrows ; i++)
  { 
    for(int j = 0 ; j < ncols ; j++)
    {
      if(i==0 || i==nrows-1 || j==0 || j==ncols-1 || TERRAIN(m,i,j) == m->no_data)
        WTERRAIN(W,i,j) = TERRAIN(m,i,j);
      else
        WTERRAIN(W,i,j) = max;
    }
  }

  return(W);
}

void send_lines(float* w, int ncols, int nrows){
  MPI_Request request = MPI_REQUEST_NULL;

  //Envoi
  if(rank != 0){
    //Envoi première ligne au processus -rank
    MPI_Issend(&(w[ncols]), ncols, MPI_FLOAT, rank-1, 0, MPI_COMM_WORLD, &request);
  }
  if(rank != size-1){
    //Envoi dernière ligne au processus +rank
    MPI_Issend(&(w[ncols*(nrows-2)]), ncols, MPI_FLOAT, rank+1, 0, MPI_COMM_WORLD, &request);
  }


  //Réception
  if(rank != 0){
    //Reception de la part de -rank de la première ligne
    MPI_Recv(&(w[0]), ncols, MPI_FLOAT, rank-1, 0, MPI_COMM_WORLD,  MPI_STATUS_IGNORE);
  }
  if(rank != size-1){  
    //Reception de la part de +rank de la dernière ligne
    MPI_Recv(&(w[ncols*(nrows-1)]), ncols, MPI_FLOAT, rank+1, 0, MPI_COMM_WORLD,  MPI_STATUS_IGNORE);
  }
}

// variables globales pour l'affichage de la progression
#ifdef DARBOUX_PPRINT
  float min_darboux=9999.; // ça ira bien, c'est juste de l'affichage
  int iter_darboux=0;
  // fonction d'affichage de la progression
  void dpprint()
  {
    if(min_darboux != 9999.)
    {
      fprintf(stderr, "%.3f %d\r", min_darboux, iter_darboux++);
      fflush(stderr);
      min_darboux = 9999.;
    }
    else
      fprintf(stderr, "\n");
  }
#endif

// pour parcourir les 8 voisins :
const int VOISINS[8][2] = {{-1,-1}, {-1,0}, {-1,1}, {0,-1}, {0,1}, {1,-1}, {1,0}, {1,1}};

// cette fonction calcule le nouveau W[i,j] en utilisant Wprec[i,j]
// et ses 8 cases voisines : Wprec[i +/- 1 , j +/- 1],
// ainsi que le MNT initial m en position [i,j]
// inutile de modifier cette fonction (elle est sensible...):
int calcul_Wij(float *restrict W, const float *restrict Wprec, const mnt *m, const int i, const int j){
  const int nrows = m->nrows, ncols = m->ncols;
  int modif = 0;

  // on prend la valeur précédente...
  WTERRAIN(W,i,j) = WTERRAIN(Wprec,i,j);
  // ... sauf si :
  if(WTERRAIN(Wprec,i,j) > TERRAIN(m,i,j))
  {
    // parcourir les 8 voisins haut/bas + gauche/droite
    for(int v=0; v<8; v++)
    {
      const int n1 = i + VOISINS[v][0];
      const int n2 = j + VOISINS[v][1];

      // vérifie qu'on ne sort pas de la grille.
      // ceci est théoriquement impossible, si les bords de la matrice Wprec
      // sont bien initialisés avec les valeurs des bords du mnt
      CHECK(n1>=0 && n1<nrows && n2>=0 && n2<ncols);

      // si le voisin est inconnu, on l'ignore et passe au suivant
      if(WTERRAIN(Wprec,n1,n2) == m->no_data)
        continue;

      CHECK(TERRAIN(m,i,j)>m->no_data);
      CHECK(WTERRAIN(Wprec,i,j)>m->no_data);
      CHECK(WTERRAIN(Wprec,n1,n2)>m->no_data);

      // il est important de mettre cette valeur dans un temporaire, sinon le
      // compilo fait des arrondis flotants divergents dans les tests ci-dessous
      const float Wn = WTERRAIN(Wprec,n1,n2) + EPSILON;
      if(TERRAIN(m,i,j) >= Wn)
      {
        WTERRAIN(W,i,j) = TERRAIN(m,i,j);
        modif = 1;
        #ifdef DARBOUX_PPRINT
        if(WTERRAIN(W,i,j)<min_darboux)
          min_darboux = WTERRAIN(W,i,j);
        #endif
      }
      else if(WTERRAIN(Wprec,i,j) > Wn)
      {
        WTERRAIN(W,i,j) = Wn;
        modif = 1;
        #ifdef DARBOUX_PPRINT
        if(WTERRAIN(W,i,j)<min_darboux)
          min_darboux = WTERRAIN(W,i,j);
        #endif
      }
    }
  }
  return(modif);
}

// applique l'algorithme de Darboux sur le MNT m, pour calculer un nouveau MNT
mnt *darboux(const mnt *restrict m){
  const int ncols = m->ncols, nrows = m->nrows;

  // initialisation
  float *restrict W, *restrict Wprec;
  CHECK((W = malloc(ncols * nrows * sizeof(float))) != NULL);
  Wprec = init_W(m);

  int first_line = rank==0?0:1; //On commence à la deuxième ligne (sauf si c'est c'est le premier processus)
  int last_line = rank==size-1?nrows:nrows-1; //On fini à l'avant dernière ligne (sauf si c'est le dernier processus)

  // calcul : boucle principale
  int modif;
  while(modif){
    send_lines(Wprec, ncols, nrows);
    modif = 0; // sera mis à 1 s'il y a une modification

    #pragma omp parallel for schedule(static, 2) reduction(|:modif)
    for(int i=first_line; i<last_line; i++){
      for(int j=0; j<ncols; j++){
        // calcule la nouvelle valeur de W[i,j]
        // en utilisant les 8 voisins de la position [i,j] du tableau Wprec
        modif |= calcul_Wij(W, Wprec, m, i, j);
      }
    }
    MPI_Allreduce(&modif, &modif, 1, MPI_INT, MPI_SUM,MPI_COMM_WORLD);
    #ifdef DARBOUX_PPRINT
    dpprint();
    #endif

    // échange W et Wprec
    // sans faire de copie mémoire : échange les pointeurs sur les deux tableaux
    float *tmp = W;
    W = Wprec;
    Wprec = tmp;
  }
  // fin du while principal
  send_lines(W, ncols, nrows);

  // fin du calcul, le résultat se trouve dans W
  free(Wprec);
  // crée la structure résultat et la renvoie
  mnt *res;
  CHECK((res=malloc(sizeof(*res))) != NULL);
  memcpy(res, m, sizeof(*res));
  res->terrain = W;

  return(res);
}
