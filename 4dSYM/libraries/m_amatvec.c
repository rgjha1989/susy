// -----------------------------------------------------------------
// Irrep matrix--vector operation with adjoint matrix
// c <-- adag * b
#include "../include/config.h"
#include "../include/complex.h"
#include "../include/su3.h"

#ifndef FAST
void mult_adj_su3_mat_vec(su3_matrix *a, su3_vector *b, su3_vector *c) {
  register int i,j;
  register complex x;
  for (i = 0; i < DIMF; i++) {
    x.real = 0.0;
    x.imag = 0.0;
    for (j = 0; j < DIMF; j++) {
#ifndef REALREP
      register complex y, z;
      CONJG(a->e[j][i], z);
      CMUL(z, b->c[j], y);
      CSUM(x, y);
#else
      x.real += a->e[j][i] * b->c[j].real;
      x.imag += a->e[j][i] * b->c[j].imag;
#endif
    }
    c->c[i] = x;
  }
}
#else   // FAST version for DIMF=3 only
void mult_adj_su3_mat_vec(su3_matrix *a, su3_vector *b, su3_vector *c) {
  int i;
  register Real t, ar, ai, br, bi, cr, ci;
  for (i = 0; i < 3; i++) {
    ar = a->e[0][i].real;
    ai = a->e[0][i].imag;
    br = b->c[0].real;
    bi = b->c[0].imag;
    cr = ar * br;
    t = ai * bi;
    cr += t;
    ci = ar * bi;
    t = ai * br;
    ci -= t;

    ar = a->e[1][i].real;
    ai = a->e[1][i].imag;
    br = b->c[1].real;
    bi = b->c[1].imag;
    t = ar * br;
    cr += t;
    t = ai * bi;
    cr += t;
    t = ar * bi;
    ci += t;
    t = ai * br;
    ci -= t;

    ar = a->e[2][i].real;
    ai = a->e[2][i].imag;
    br = b->c[2].real;
    bi = b->c[2].imag;
    t = ar * br;
    cr += t;
    t = ai * bi;
    cr += t;
    t = ar * bi;
    ci += t;
    t = ai * br;
    ci -= t;

    c->c[i].real = cr;
    c->c[i].imag = ci;
  }
}
#endif
// -----------------------------------------------------------------