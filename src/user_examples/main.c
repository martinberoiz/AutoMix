// Driver main function for the AutoMix RJMCMC sampler
#include "automix.h"
#include "logwrite.h"
#include "user.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void usage(char *invocation);
void parse_cmdline_args(int argc, char *argv[], char **fname, int *nsweep,
                        int *nsweep2, unsigned long *seed, int *doperm,
                        int *adapt, int *mode, int *dof, int *nburn);
// wrapper function to fit with AutoMix
double logposterior(int model_k, double *x) {
  double lpost = 0;
  double likelihood;
  logpost(model_k, 0, x, &lpost, &likelihood);
  return lpost;
}
extern void sdrni(unsigned long *seed);

int main(int argc, char *argv[]) {

  clock_t starttime = clock();

  // Default values for command line arguments
  // mode ~ m ~ 0 if mixture fitting, 1 if user supplied mixture params, 2 if
  // AutoRJ
  int mode = 0;
  // nsweep ~ N ~ no. of reversible jump sweeps in stage 3
  int nsweep = 1E5;
  // nsweep2 ~ n ~ max(n,10000*mdim,100000) sweeps in within-model RWM in stage
  // 1
  int nsweep2 = 1E5;
  // adapt ~ a ~ 1 if RJ adaptation done in stage 3, 0 otherwise
  int adapt = 1;
  // doperm ~ p ~ 1 if random permutation done in stage 3 RJ move, 0 otherwise
  int doperm = 1;
  // seed ~ s ~ random no. seed, 0 uses clock seed
  unsigned long seed = 0;
  // dof ~ t ~ 0 if Normal random variables used in RWM and RJ moves,
  // otherwise specify integer degrees of freedom of student t variables
  int dof = 0;
  // fname ~ f ~ filename base (default = "output")
  char *const fname_default = "output";
  char *fname = fname_default;
  // -1 is a flag for undefined
  int nburn = -1;

  // Override defaults if user supplies command line options
  parse_cmdline_args(argc, argv, &fname, &nsweep, &nsweep2, &seed, &doperm,
                     &adapt, &mode, &dof, &nburn);
  sdrni(&seed);
  if (nburn == -1) {
    nburn = nsweep / 10;
    if (nburn < 10000) {
      nburn = 10000;
    }
  }

  // Initialize the AutoMix sampler
  int nmodels = get_nmodels();
  int *model_dims = (int *)malloc(nmodels * sizeof(int));
  load_model_dims(nmodels, model_dims);
  int mdim_len = 0;
  for (int i = 0; i < nmodels; i++) {
    mdim_len += model_dims[i];
  }
  amSampler am;
  double *initRWM = (double *)malloc(mdim_len * sizeof(double *));
  int offset = 0;
  for (int i = 0; i < nmodels; i++) {
    get_rwm_init(i, model_dims[i], initRWM + offset);
    offset += model_dims[i];
  }
  initAMSampler(&am, nmodels, model_dims, logposterior, initRWM);
  free(initRWM);

  // --- Section 5.1 - Read in mixture parameters if mode 1 (m=1) ---
  if (mode == 1) {
    // Read AutoMix parameters from file if mode = 1
    printf("Reading parameters from mix file.\n");
    int ok = read_mixture_params(fname, &am);
    if (ok == EXIT_FAILURE) {
      return EXIT_FAILURE;
    }
  } else {
    printf("Using %d samples for RWM Mixture Fitting Model.\n", nsweep2);
    estimate_conditional_probs(&am, nsweep2);
    report_cond_prob_estimation(fname, am);
  }

  // -----Start of main loop ----------------
  // Burn some samples first
  printf("Burning in %d samples.\n", nburn);
  burn_samples(&am, nburn);
  // Collect nsweep RJMCMC samples
  printf("Start of main sample: ");
  rjmcmc_samples(&am, nsweep);
  printf("%d samples generated.\n", nsweep);
  // --- Section 10 - Write statistics to files ---------
  report_rjmcmc_run(fname, am, mode, nsweep2, nsweep);

  freeAMSampler(&am);

  clock_t endtime = clock();
  double timesecs = (endtime - starttime) / ((double)CLOCKS_PER_SEC);
  printf("Total time elapsed: %f sec.\n", timesecs);

  return EXIT_SUCCESS;
}

void usage(char *invocation) {
  char *name = strrchr(invocation, '/');
  if (name == NULL) {
    name = invocation;
  } else {
    name += 1;
  }
  printf("%s\n", name);
  printf("Usage: %s [-m int] [-N int] [-n int] [-a bool] [-p bool] [-s int] "
         "[-t int] [-f string] [-h, --help]\n",
         name);
  printf("-m int: Specify mode. 0 if mixture fitting, 1 if user supplied "
         "mixture params, 2 if AutoRJ.\n");
  printf("-N int: Number of reversible jump sweeps in stage 3.\n");
  printf("-n int: max(n, 10000 * mdim, 100000) sweeps in within-model RWM in "
         "stage 1.\n");
  printf("-a bool: 1 if RJ adaptation done in stage 3, 0 otherwise.\n");
  printf("-p bool: 1 if random permutation done in stage 3 RJ move, 0 "
         "otherwise.\n");
  printf("-s int: random no. seed. 0 uses clock seed.\n");
  printf(
      "-t int: 0 if Normal random variables used in RWM and RJ moves, "
      "otherwise specify integer degrees of freedom of student t variables.\n");
  printf("-b int: Number of samples to burn. If not specified it defaults to "
         "max between 10,000 and N/10.\n");
  printf("-f string: filename base.\n");
  printf("-h, --help: Print this help and exit.");
  printf(
      "\n(c) Original code by David Hastie. Modifications by Martin Beroiz.\n");
}

void parse_cmdline_args(int argc, char *argv[], char **fname, int *nsweep,
                        int *nsweep2, unsigned long *seed, int *doperm,
                        int *adapt, int *mode, int *dof, int *nburn) {
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-f")) {
      *fname = argv[++i];
      continue;
    } else if (!strcmp(argv[i], "-N")) {
      *nsweep = atoi(argv[++i]);
      continue;
    } else if (!strcmp(argv[i], "-n")) {
      *nsweep2 = atoi(argv[++i]);
      if (*nsweep2 < 100000) {
        *nsweep2 = 100000;
      }
      continue;
    } else if (!strcmp(argv[i], "-s")) {
      *seed = atoi(argv[++i]);
      continue;
    } else if (!strcmp(argv[i], "-p")) {
      *doperm = atoi(argv[++i]);
      continue;
    } else if (!strcmp(argv[i], "-m")) {
      *mode = atoi(argv[++i]);
      if (*mode > 2 || *mode < 0) {
        printf("Error: Invalid mode entered. Mode must be {0, 1, 2}.\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
      }
      continue;
    } else if (!strcmp(argv[i], "-a")) {
      *adapt = atoi(argv[++i]);
      continue;
    } else if (!strcmp(argv[i], "-t")) {
      *dof = atoi(argv[++i]);
      continue;
    } else if (!strcmp(argv[i], "-b")) {
      *nburn = atoi(argv[++i]);
      if (*nburn < 0) {
        printf("Error: Negative value for burn samples.\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
      }
      continue;
    } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
      usage(argv[0]);
      exit(EXIT_SUCCESS);
    } else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
      printf("AutoMix Version %s\n", AUTOMIX_VERSION);
      exit(EXIT_SUCCESS);
    } else {
      printf("Unrecognized argument: %s\n", argv[i]);
      usage(argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  return;
}
