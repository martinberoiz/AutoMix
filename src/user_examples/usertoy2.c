/* User functions for a second toy example not included in Ph.D. thesis.
   The example has 5 equally weighted models.
   Model k has dimension k, (k=1,...,5) and is
   a mixture of two normals, the first component having weight 0.3, mean
   (5,...,5) and cov matrix I_k, the second component having weight 0.7,
   mean (-5,...,-5) and cov matrix 4*I_k. */

#include <math.h>
#include <stdio.h>

#define tpi 6.283185307179586477

/* Function to return number of models */
int get_nmodels(void) { return 5; }

/* Function to return the dimension of each model */
void load_model_dims(int nmodels, int *model_dims) {
  for (int k = 0; k < nmodels; k++) {
    model_dims[k] = k + 1;
  }
  return;
}

/* Function to return initial conditions for RWM runs */
void get_rwm_init(int model_k, int mdim, double *rwm) {
  for (int j = 0; j < mdim; j++) {
    rwm[j] = 0;
  }
}

/* Function to return log target distribution up to additive const at (k,theta)
   value also returned in llh */

void logpost(int k, int nkk, double *theta, double *lp, double *llh) {
  int i;
  int model_dims[] = {1, 2, 3, 4, 5};
  nkk = model_dims[k];
  double work[nkk], mu1[nkk], mu2[nkk], sig1, sig2, w1, w2;
  double lptemp;
  double modw = 0.0;

  sig1 = 1.0;
  sig2 = 2.0;
  w1 = 0.3;
  w2 = 0.7;
  if (k < 4) {
    modw = 1.0 / pow(2.0, (k + 1));
  }
  if (k == 4) {
    modw = 0.0625;
  }

  for (i = 0; i < nkk; i++) {
    mu1[i] = 5.0;
    mu2[i] = -5.0;
  }

  lptemp = 0.0;
  for (i = 0; i < nkk; i++) {
    work[i] = (theta[i] - mu1[i]);
    lptemp -= pow(work[i], 2.0) / (2.0 * pow(sig1, 2));
  }
  lptemp = exp(lptemp);
  lptemp /= pow(tpi, (nkk / 2.0));
  lptemp /= pow(sig1, nkk);
  *lp = w1 * lptemp;
  lptemp = 0.0;
  for (i = 0; i < nkk; i++) {
    work[i] = (theta[i] - mu2[i]);
    lptemp -= pow(work[i], 2.0) / (2.0 * pow(sig2, 2));
  }
  lptemp = exp(lptemp);
  lptemp /= pow(tpi, (nkk / 2.0));
  lptemp /= pow(sig2, nkk);
  *lp += w2 * lptemp;
  *lp = log(modw * (*lp));

  *llh = *lp;
  return;
}
