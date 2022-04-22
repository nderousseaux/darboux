// fonctions d'entrée/sortie

#include <stdio.h>

#include "check.h"
#include "type.h"
#include "io.h"


int share_rows(int n_rows, int index){
  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  if(index == size-1 && index == 0){
    return n_rows;
  }

  int result = n_rows/size;
  int reste=n_rows%size;
  if(index == size-1){
    return result+reste+1;
  }
  else{
    if(index == 0){
      return result+1;
    }
    else{
      return result+2;
    }
  }
}

void display_information(mnt * m,int rank){
  int index_2 = m->ncols;
  int index_dernier = (m->ncols*m->nrows)-1;
  int index_avant_dernier = index_dernier - m->ncols;
   printf("num_proc: %d, ncold: %d, nrows: %d, xcorner:  %f, ycorner: %f, cellsize: %f, \n terrain[0]: %f, terrain[1], %f, terrain[-2]: %f, terrain[-1], %f, \n",
      rank, m->ncols, m->nrows, m->xllcorner, m->yllcorner, m->cellsize, m->terrain[0], m->terrain[index_2], m->terrain[index_avant_dernier], m->terrain[index_dernier]
    );
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
  for(int i = 0; i<m->ncols*m->nrows; i++){ //TODO: SUpprimer si sert à rien
    m->terrain[i] = 0.;
  }
  return m;
}

mnt *mnt_read(char *fname)
{
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  mnt *m;
  CHECK((m = malloc(sizeof(*m))) != NULL);
  if(rank == 0){
    mnt *m_principal = read_file(fname);

    //On envoie une partie de la matrice à chaque sous processus
    for(int i = 1; i<size; i++){
      int size_m[2] = {m_principal->ncols, share_rows(m_principal->nrows, i)};
      int offset = (m_principal->ncols * (m_principal->nrows/size) * i)-m_principal->ncols;
      MPI_Ssend(&size_m, 2, MPI_INT, i, 0, MPI_COMM_WORLD);
      MPI_Ssend(&m_principal->xllcorner, 4, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
      MPI_Ssend(&(m_principal->terrain[offset]), size_m[0] * size_m[1],MPI_FLOAT, i, 0, MPI_COMM_WORLD);
    }

    //On crée la matrice pour rank = 0
    // prenons l'exemple de min.mnt  qui est de taille de 10*10
    // le premier processus  qui va recupérer les premiéres lignes va prévoire tjr  de la place pour une derniére ligne
    // du coup le nombre de ligne totale =nrows+1
    m = create_m(m_principal, share_rows(m_principal->nrows, 0));

    // ncols-1 car la derniére ligne sera réservé
    for(int i=0; i<m->ncols*m->nrows; i++){
      m->terrain[i] = m_principal->terrain[i];
    }
  }
  //Processus autre que 0
  //pour tous les processus différents du premier et du dernier, vont prévoir deux lignes supplémentaires
  //pour stocker les lignes qu'ils vont recevoir
  else{
      MPI_Status s;
      MPI_Recv(&m->ncols, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &s);
      MPI_Recv(&m->xllcorner, 4, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &s);
      m = create_m(m, m->nrows);
     
     // vus que la première ligne sera réservée on va commencer à écrire à partir de la deuxième ligne
      MPI_Recv(&(m->terrain[0]), m->ncols * m->nrows, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &s);

  }
  // display_information(m,rank);
  return(m);
}

void mnt_write(mnt *m, FILE *f)
{
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

void mnt_write_lakes(mnt *m, mnt *d, FILE *f)
{
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

mnt *merge_result(mnt *d){
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  //On calcule la taille de la matrice initiale
  int nrows;
  MPI_Allreduce(&d->nrows, &nrows, 1, MPI_INT, MPI_SUM,MPI_COMM_WORLD);
  nrows -= (size-1)*2;
  if(rank == 0){
    //On crée la matrice
    mnt * d_final = create_m(d, nrows);

    //On rempli la matrice
    for(int i=0; i<d->ncols*d->nrows; i++){
      d_final->terrain[i] = d->terrain[i];
    }
    for(int i = 1; i<size; i++){
      int ligne_depart = i*(nrows/size);
      int nbLigne = 0;
      if(i == size-1){
        nbLigne = share_rows(nrows, i)-1;
      }
      else{
        nbLigne = share_rows(nrows, i)-2;
      }
      MPI_Recv(&(d_final->terrain[ligne_depart*d_final->ncols]), d->ncols * nbLigne, MPI_FLOAT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
      
    d = d_final;

    // for(int i=0; i<d->ncols*d->nrows;i++){
    //   printf("%d : %f\n",i, d->terrain[i]);
    // }
  }
  else{
    //On envoie sa matrice
    int minus = 2;
    if(rank == size-1){
      minus = 1;
    }
    
    MPI_Ssend(&(d->terrain[d->ncols]), d->ncols * (d->nrows-minus),MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
  }
  return d;
}