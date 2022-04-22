// fonctions d'entrée/sortie
#include <stdio.h>

#include "check.h"
#include "type.h"
#include "io.h"

mnt *read_file(char *fname){
  mnt *m;
  FILE *f;
  CHECK((f = fopen(fname, "r")) != NULL);
  CHECK((m = malloc(sizeof(*m))) != NULL);
  CHECK(fscanf(f, "%d", &m->ncols) == 1);
  CHECK(fscanf(f, "%d", &m->nrows) == 1);
  CHECK(fscanf(f, "%f", &m->xllcorner) == 1);
  CHECK(fscanf(f, "%f", &m->yllcorner) == 1);
  CHECK(fscanf(f, "%f", &m->cellsize) == 1);
  CHECK(fscanf(f, "%f", &m->no_data) == 1);

  CHECK((m->terrain = malloc(m->ncols * m->nrows * sizeof(float))) != NULL);

  for(int i = 0 ; i < m->ncols * m->nrows ; i++)
  {
    CHECK(fscanf(f, "%f", &m->terrain[i]) == 1);
  }
  CHECK(fclose(f) == 0); 
  return m;
}

//Détermine le nombre de ligne de la matrice d'index spécifié
int get_nrows(int nrows, int index){
  int result = nrows/size;
  result+=2; //On ajoute deux lignes pour les swaps

  //Si c'est le premier processus
  if(index == 0){
    result--; //On enlève une ligne (La première ne sera pas envoyée)
  }
  //Si c'est le dernier processus
  if(index == size-1){
    result--; //On enlève une ligne (La dernière ne sera pas envoyée)
    result+=nrows%size; //On ajoute le reste
  }
  return result;
}

mnt *create_m(mnt *m_source, int nrows){
  mnt *m;
  CHECK((m = malloc(sizeof(*m))) != NULL);
  m->ncols = m_source->ncols;
  m->nrows = nrows;
  m->xllcorner = m_source->xllcorner;
  m->yllcorner = m_source->yllcorner;
  m->cellsize = m_source->cellsize;
  m->no_data  = m_source->no_data;

  CHECK((m->terrain = malloc(m->ncols * m->nrows * sizeof(float))) != NULL);
  return m;
}

mnt *mnt_read(char *fname){
  mnt *m;
  CHECK((m = malloc(sizeof(*m))) != NULL);

  //Le premier processus envoie une partie de la matrice à tout le monde
  if(rank == 0){
    mnt *m_principal = read_file(fname);

    //On envoie une partie de la matrice à chaque sous processus
    for(int i = 1; i<size; i++){
      //Taille de la matrice
      int size_m[2] = {m_principal->ncols, get_nrows(m_principal->nrows, i)};
      MPI_Ssend(&size_m, 2, MPI_INT, i, 0, MPI_COMM_WORLD);

      //Autres données
      MPI_Ssend(&m_principal->xllcorner, 4, MPI_FLOAT, i, 0, MPI_COMM_WORLD);

      //Terrain
      int index_start = (m_principal->ncols) * (m_principal->nrows/size) * i;
      index_start -= m_principal->ncols; //On commence une ligne avant (ligne de swap)
      MPI_Ssend(&(m_principal->terrain[index_start]), size_m[0] * size_m[1],MPI_FLOAT, i, 0, MPI_COMM_WORLD);
    }

    //On crée la matrice de rang 0
    m = create_m(m_principal, get_nrows(m_principal->nrows, 0));
    for(int i=0; i<m->ncols*m->nrows; i++){
      m->terrain[i] = m_principal->terrain[i];
    }

    free(m_principal->terrain);
    free(m_principal);
  }

  //Les autres processus recoivent une partie de la matrice
  else{
      //On reçoit la taille
      MPI_Recv(&m->ncols, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      m = create_m(m, m->nrows);

      //On recoit les autres données
      MPI_Recv(&m->xllcorner, 4, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
     
      //On recoit le terrain
      MPI_Recv(&(m->terrain[0]), m->ncols * m->nrows, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }
  return(m);
}

void mnt_write(mnt *m, FILE *f){
  CHECK(f != NULL);

  fprintf(f, "%d\n", m->ncols);
  fprintf(f, "%d\n", m->nrows);
  fprintf(f, "%.2f\n", m->xllcorner);
  fprintf(f, "%.2f\n", m->yllcorner);
  fprintf(f, "%.2f\n", m->cellsize);
  fprintf(f, "%.2f\n", m->no_data);

  for(int i = 0 ; i < m->nrows ; i++)
  {
    for(int j = 0 ; j < m->ncols ; j++)
    {
      fprintf(f, "%.2f ", TERRAIN(m,i,j));
    }
    fprintf(f, "\n");
  }
}

void mnt_write_lakes(mnt *m, mnt *d, FILE *f){
  CHECK(f != NULL);

  for(int i = 0 ; i < m->nrows ; i++)
  {
    for(int j = 0 ; j < m->ncols ; j++)
    {
      const float dif = TERRAIN(d,i,j)-TERRAIN(m,i,j);
      fprintf(f, "%c", (dif>1.)?'#':(dif>0.)?'+':'.' );
    }
    fprintf(f, "\n");
  }
}

mnt *merge_result(mnt *d){
  //On calcule la taille de la matrice initiale
  int nrows;
  MPI_Allreduce(&d->nrows, &nrows, 1, MPI_INT, MPI_SUM,MPI_COMM_WORLD);
  nrows -= (size-1)*2;

  //Si c'est le processus 1, on reçoit les autres matrices
  if(rank == 0){
    //On crée la matrice
    mnt * d_final = create_m(d, nrows);
    for(int i=0; i<d->ncols*d->nrows; i++){
      d_final->terrain[i] = d->terrain[i];
    }

    //On reçoit les autres matrices
    for(int i = 1; i<size; i++){
      int first_line = i*(nrows/size); //La première l
      int nb_lines = get_nrows(nrows, i); 
      nb_lines -= i==size-1?1:2; //On prend deux lignes de moins (sauf si on est le dernier processus)
      MPI_Recv(&(d_final->terrain[first_line*d_final->ncols]), d->ncols * nb_lines, MPI_FLOAT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    d = d_final;
  }
  else{
    int nb_lines = d->nrows;
    nb_lines -= rank==size-1?1:2; //On envoie deux lignes de moins (sauf si on est le dernier processus)
    
    //On envoie sa matrice
    MPI_Ssend(&(d->terrain[d->ncols]), d->ncols * nb_lines,MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
  }
  return d;
}