// programme principal
#include <stdio.h>
#include <stdlib.h>


#include "type.h"
#include "io.h"
#include "darboux.h"
#include "mpi.h"

int main(int argc, char **argv)
{
  if (MPI_Init(&argc, &argv)){
		fprintf(stderr, "Erreur MPI_Init\n");
  }
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  mnt *m;
  mnt *d;  

  if(argc < 2)
  {
    fprintf(stderr, "Usage: %s <input filename> [<output filename>]\n", argv[0]);
    exit(1);
  }

  // READ INPUT
  m = mnt_read(argv[1]);

  // COMPUTE
  d = darboux(m);

  d = merge_result(d);

  if(rank == 0){
    // WRITE OUTPUT
    FILE *out;
    if(argc == 3)
      out = fopen(argv[2], "w");
    else
      out = stdout;
    mnt_write(d, out);
    if(argc == 3)
      fclose(out);
    else
      mnt_write_lakes(m, d, stdout);
  }

  
  free(m->terrain);
  free(m);
  free(d->terrain);
  free(d);
  MPI_Finalize();
  return(0);
}
