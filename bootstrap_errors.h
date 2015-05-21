/*! \file
    \brief Public interfaces for function(s) dealing with estimating
    parameter errors via bootstrap resampling.

 */

#ifndef _BOOTSTRAP_ERRORS_H_
#define _BOOTSTRAP_ERRORS_H_

#include <string>

#include "param_struct.h"   // for mp_par structure
#include "model_object.h"


/*! \brief Primary wrapper function: runs bootstrap resampling and prints
           summary statistics; allows optional saving of all parameters to file

    If saving of all best-fit parameters to file is requested, then outputFile_ptr
    should be non-NULL (i.e., should point to a file object opened for writing, possibly
    with header information already written).
*/
int BootstrapErrors( double *bestfitParams, mp_par *parameterLimits, bool paramLimitsExist, 
					ModelObject *theModel, double ftol, int nIterations, int nFreeParams,
					int whichStatistic, FILE *outputFile_ptr );

/*! \brief Alternate wrapper: returns array of best-fit parameters in outputParamArray;
           doesn't print any summary statistics (e.g., sigmas, confidence intervals). 
           
    Note that outputParamArray will be allocated here; it should be de-allocated by 
    whatever function is calling this function. */
int BootstrapErrorsArrayOnly( double *bestfitParams, mp_par *parameterLimits, bool paramLimitsExist, 
					ModelObject *theModel, double ftol, int nIterations, int nFreeParams,
					int whichStatistic, double **outputParamArray );


#endif  // _BOOTSTRAP_ERRORS_H_
