// fonctions d'entrée/sortie

#include <stdio.h>

#include "check.h"
#include "type.h"
#include "io.h"


int share_rows(int n_rows, int index){
  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int result = n_rows/size;
  int reste=n_rows%size;
  if(index == size-1){
    return result+reste;
  }
  else{
    return result;
  }
}

void display_information(mnt * m,int* rank){
   
   printf("**************************************\n");
   printf("nb_proc: %d\n",*rank);
   printf("**************************************\n");
   printf("informations : \n");
   printf("              m->ncols ->%d \n",m->ncols);
   printf("              m->nrows ->%d \n",m->nrows);
   printf("              m->xllcorner ->%f \n",  m->xllcorner);
   printf("              m->yllcorner ->%f \n",  m->yllcorner);
   printf("              m->cellsize  ->%f \n", m->cellsize );
   printf("              m->terrain[0]   ->%f \n", m->terrain[0]);
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
      int size_m[2] = {m_principal->nrows, share_rows(m_principal->ncols, i)};

      int offset = m_principal->ncols * (m_principal->nrows/size) * i;
      MPI_Ssend(&size_m, 2, MPI_INT, i, 0, MPI_COMM_WORLD);
      MPI_Ssend(&m_principal->xllcorner, 4, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
      MPI_Ssend(&(m_principal->terrain[offset]), size_m[0] * size_m[1],MPI_FLOAT, i, 0, MPI_COMM_WORLD);
    }

    //On crée la matrice pour rank = 0
    m = create_m(m_principal, share_rows(m_principal->ncols, 0));
    for(int i=0; i<m->ncols*m->nrows; i++){
      m->terrain[i] = m_principal->terrain[i];
    }
  }
  //Processus autre que 0
  else{
    MPI_Status s;
    MPI_Recv(&m->ncols, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &s);
    MPI_Recv(&m->xllcorner, 4, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &s);
    m = create_m(m, m->nrows);
    MPI_Recv(&(m->terrain[0]), m->ncols * m->nrows, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &s);
  }
  

   display_information(m,&rank);
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