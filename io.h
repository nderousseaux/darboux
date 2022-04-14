#ifndef __IO_H__
#define __IO_H__

#include "type.h"
#include "mpi.h"


mnt *mnt_read(char *fname);
void mnt_write(mnt *m, FILE *f);
void mnt_write_lakes(mnt *m, mnt *d, FILE *f);
int share_rows(int n_rows, int index);
mnt *read_file(char *fname);
mnt *create_m(mnt *m, int index);

#endif
