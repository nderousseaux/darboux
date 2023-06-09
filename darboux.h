#ifndef __DARBOUX_H__
#define __DARBOUX_H__

#include "type.h"

#define EPSILON .01

mnt *darboux(const mnt *restrict m);
void send_lines(float* w, int ncols, int nrows);

#endif
