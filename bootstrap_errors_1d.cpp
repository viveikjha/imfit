/* FILE: bootstrap_errors_1d.cpp --------------------------------------- */
/* VERSION 0.01
 *
 * Code for estimating errors on fitted parameters (for a 1D profile fit via
 * profilefit) via bootstrap resampling.
 *
 *     [v0.01]: 3 Mar 2011: Created; initial development.
 *
 */


/* ------------------------ Include Files (Header Files )--------------- */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "definitions.h"
#include "model_object_1d.h"
#include "mpfit_cpp.h"
#include "mersenne_twister.h"
#include "bootstrap_errors_1d.h"
#include "print_results.h"


/* ------------------- Function Prototypes ----------------------------- */
/* External Functions: */
int myfunc( int nDataVals, int nParams, double *params, double *deviates,
           double **derivatives, ModelObject *aModel );




void BootstrapErrors( double *bestfitParams, mp_par *parameterLimits, 
									bool paramLimitsExist, ModelObject *theModel, int nIterations,
									int nFreeParams )
{
  mp_config  mpConfig;
  mp_result  mpfitResult;
  mp_par  *mpfitParameterConstraints;
  double  *paramsVect;
  double  **paramArray;
  int  i, status, nIter;
  int  nParams = theModel->GetNParams();
  int  nStoredDataVals = theModel->GetNDataValues();
  
  /* seed random number generators with current time */
  init_genrand( (unsigned long)time(NULL) );

  paramsVect = (double *) malloc(nParams * sizeof(double));
  // Allocate 2D array to hold bootstrap results for each parameter
  paramArray = (double **)calloc( (size_t)nParams, sizeof(double *) );
  for (i = 0; i < nParams; i++)
    paramArray[i] = (double *)calloc( (size_t)nIterations, sizeof(double) );


  theModel->UseBootstrap();


  // Set up things for L-M minimization
  if (paramLimitsExist)
    mpfitParameterConstraints = parameterLimits;
  else
    mpfitParameterConstraints = NULL;
  bzero(&mpfitResult, sizeof(mpfitResult));       /* Zero the results structure */
  bzero(&mpConfig, sizeof(mpConfig));
  mpConfig.maxiter = 1000;

  // Do minimization
  printf("\nStarting bootstrap iterations...\n");

  for (nIter = 0; nIter < nIterations; nIter++) {
    
    theModel->MakeBootstrapSample();
    for (i = 0; i < nParams; i++)
      paramsVect[i] = bestfitParams[i];
    status = mpfit(myfunc, nStoredDataVals, nParams, paramsVect, mpfitParameterConstraints,
                      &mpConfig, theModel, &mpfitResult);
    for (i = 0; i < nParams; i++) {
      paramArray[i][nIter] = paramsVect[i];
    }
//    PrintResults(paramsVect, 0, &mpfitResult, theModel, nFreeParams, parameterLimits, status);
  }


  free(paramArray);

}


/* Needed by mpfit: */
// int myfunc( int nDataVals, int nParams, double *params, double *deviates,
//            double **derivatives, ModelObject *theModel )
// {
// 
//   theModel->ComputeDeviates(deviates, params);
//   return 0;
// }


/* END OF FILE: bootstrap_errors_1d.cpp -------------------------------- */
