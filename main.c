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
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  printf("%d\n", size);

  mnt *m, *d;  
  double time_start, time_end, duration;

  if(argc < 2)
  {
    fprintf(stderr, "Usage: %s <input filename> [<output filename>]\n", argv[0]);
    exit(1);
  }

  // READ INPUT
  m = mnt_read(argv[1]);

  // COMPUTE
  time_start = MPI_Wtime();
  d = darboux(m);
  time_end = MPI_Wtime();
  duration = time_end-time_start;
  MPI_Allreduce(&duration, &duration, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  duration /=size;

  //Merge matrices
  d = merge_result(d);

  if(rank == 0){
    printf("%f\n", duration);
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
