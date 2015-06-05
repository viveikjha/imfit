/* FILE: makeimage_main.cpp ---------------------------------------------- */
/*
 * Program for generating model images, using same code and approaches as
 * imfit does (but without the image-fitting parts).
 * 
 * The proper translations are:
 * NAXIS1 = naxes[0] = nColumns = sizeX;
 * NAXIS2 = naxes[1] = nRows = sizeY.
*/

// Copyright 2010--2015 by Peter Erwin.
// 
// This file is part of Imfit.
// 
// Imfit is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your
// option) any later version.
// 
// Imfit is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
// 
// You should have received a copy of the GNU General Public License along
// with Imfit.  If not, see <http://www.gnu.org/licenses/>.



/* ------------------------ Include Files (Header Files )--------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string>

#include "definitions.h"
#include "image_io.h"
#include "model_object.h"
#include "add_functions.h"
#include "commandline_parser.h"
#include "config_file_parser.h"
#include "utilities_pub.h"
#include "option_struct_makeimage.h"


/* ---------------- Definitions ---------------------------------------- */
#define EST_SIZE_HELP_STRING "     --estimation-size <int>  Size of square image to use for estimating fluxes [default = 5000]"


// Option names for use in config files
static string  kNCols1 = "NCOLS";
static string  kNCols2 = "NCOLUMNS";
static string  kNRows = "NROWS";


#ifdef USE_OPENMP
#define VERSION_STRING      "1.2 (OpenMP-enabled)"
#else
#define VERSION_STRING      "1.2"
#endif



/* ------------------- Function Prototypes ----------------------------- */
/* External functions: */

/* Local Functions: */
void ProcessInput( int argc, char *argv[], makeimageCommandOptions *theOptions );
void HandleConfigFileOptions( configOptions *configFileOptions, 
								makeimageCommandOptions *mainOptions );


/* ------------------------ Global Variables --------------------------- */

/* ------------------------ Module Variables --------------------------- */





/* ---------------- MAIN ----------------------------------------------- */

int main( int argc, char *argv[] )
{
  int  nColumns, nRows;
  int  nPixels_psf, nRows_psf, nColumns_psf;
  int  nParamsTot;
  int  status;
  double  *psfPixels;
  double  *psfOversampledPixels;
  int  nPixels_psf_oversampled, nColumns_psf_oversampled, nRows_psf_oversampled;
  int  x1_oversample = 0;
  int  x2_oversample = 0;
  int  y1_oversample = 0;
  int  y2_oversample = 0;
  double  *paramsVect;
  ModelObject  *theModel;
  vector<string>  functionList;
  vector<double>  parameterList;
  vector<int>  functionBlockIndices;
  vector<string>  imageCommentsList;
  makeimageCommandOptions  options;
  configOptions  userConfigOptions;
  bool  printFluxesOnly = false;
  
  
  /* Process command line and parse config file: */
  SetDefaultMakeimageOptions(&options);
  ProcessInput(argc, argv, &options);
  
  if ((options.printFluxes) && (! options.saveImage) && (! options.printImages))
    printFluxesOnly = true;

  /* Read configuration file */
  if (! FileExists(options.configFileName.c_str())) {
    fprintf(stderr, "\n*** ERROR: Unable to find configuration file \"%s\"!\n\n", 
           options.configFileName.c_str());
    return -1;
  }
  status = ReadConfigFile(options.configFileName, true, functionList, parameterList,
  							functionBlockIndices, userConfigOptions);
  if (status != 0) {
    fprintf(stderr, "\n*** ERROR: Failure reading configuration file \"%s\"!\n\n", 
    			options.configFileName.c_str());
    return -1;
  }

  // Parse and process user-supplied (non-function) values from config file, if any
  HandleConfigFileOptions(&userConfigOptions, &options);

  if (! options.saveImage) {
    printf("\nUser requested that no images be saved!\n\n");
  }
  
  if ((options.nColumns > 0) && (options.nRows > 0))
    options.noImageDimensions = false;
  if ( (options.noRefImage) && (options.noImageDimensions)) {
    if (options.saveImage) {
      fprintf(stderr, "\n*** ERROR: Insufficient image dimensions (or no reference image) supplied!\n\n");
      return -1;
    }
    else {
      // minimal image size for the case where we don't actually generate an image
      // (except for --print-fluxes purposes)
      options.nColumns = 2;
      options.nRows = 2;
    }
  }
  /* Get image size from reference image, if necessary */
  if ((! printFluxesOnly) && (options.noImageDimensions)) {
    status = GetImageSize(options.referenceImageName, &nColumns, &nRows);
    if (status != 0) {
      fprintf(stderr,  "\n*** ERROR: Failure determining size of image file \"%s\"!\n\n", 
      			options.referenceImageName.c_str());
      exit(-1);
    }
    // Reminder: nColumns = n_pixels_per_row
    // Reminder: nRows = n_pixels_per_column
    printf("Reference image read: naxis1 [# rows] = %d, naxis2 [# columns] = %d\n",
           nRows, nColumns);
  }
  else {
    nColumns = options.nColumns;
    nRows = options.nRows;
  }
  

  /* Read in PSF image, if supplied */
  if (options.psfImagePresent) {
    printf("Reading PSF image (\"%s\") ...\n", options.psfFileName.c_str());
    psfPixels = ReadImageAsVector(options.psfFileName, &nColumns_psf, &nRows_psf);
    if (psfPixels == NULL) {
      fprintf(stderr,  "\n*** ERROR: Unable to read PSF image file \"%s\"!\n\n", 
      			options.psfFileName.c_str());
      exit(-1);
    }
    nPixels_psf = nColumns_psf * nRows_psf;
    printf("naxis1 [# pixels/row] = %d, naxis2 [# pixels/col] = %d; nPixels_tot = %d\n", 
           nColumns_psf, nRows_psf, nPixels_psf);
  }
  else
    printf("* No PSF image supplied -- no image convolution will be done!\n");

  /* Read in oversampled PSF image, if supplied */
  if (options.psfOversampledImagePresent) {
    if (options.psfOversamplingScale < 1) {
      fprintf(stderr, "\n*** ERROR: the oversampling scale for the oversampled PSF was not supplied!\n\n");
      exit(-1);
    }
    if (! options.oversampleRegionSet) {
      fprintf(stderr, "\n*** ERROR: the oversampling region within the main image was not defined!\n\n");
      exit(-1);
    }
    printf("Reading oversampled PSF image (\"%s\") ...\n", options.psfOversampledFileName.c_str());
    psfOversampledPixels = ReadImageAsVector(options.psfOversampledFileName, 
    							&nColumns_psf_oversampled, &nRows_psf_oversampled);
    if (psfOversampledPixels == NULL) {
      fprintf(stderr, "\n*** ERROR: Unable to read oversampled PSF image file \"%s\"!\n\n", 
    			options.psfOversampledFileName.c_str());
      exit(-1);
    }
    nPixels_psf_oversampled = nColumns_psf_oversampled * nRows_psf_oversampled;
    printf("naxis1 [# pixels/row] = %d, naxis2 [# pixels/col] = %d; nPixels_tot = %d\n", 
           nColumns_psf_oversampled, nRows_psf_oversampled, nPixels_psf_oversampled);
    // Determine oversampling region
    GetAllCoordsFromBracket(options.psfOversampleRegion, &x1_oversample, &x2_oversample, 
    						&y1_oversample, &y2_oversample);
  }

  if (! options.subsamplingFlag)
    printf("* Pixel subsampling has been turned OFF.\n");


  
  /* Set up the model object */
  theModel = new ModelObject();
  // Put limits on number of FFTW and OpenMP threads, if user requested it
  if (options.maxThreadsSet)
    theModel->SetMaxThreads(options.maxThreads);

  
  /* Add functions to the model object; also tells model object where function
     sets start */
  status = AddFunctions(theModel, functionList, functionBlockIndices, options.subsamplingFlag);
  if (status < 0) {
  	fprintf(stderr, "*** ERROR: Failure in AddFunctions!\n\n");
  	exit(-1);
  }

  
  // Add PSF image vector, if present (needs to be added prior to image data, so that
  // ModelObject can figure out proper internal model-image size
  if (options.psfImagePresent) {
    status = theModel->AddPSFVector(nPixels_psf, nColumns_psf, nRows_psf, psfPixels);
    if (status < 0) {
      fprintf(stderr, "*** ERROR: Failure in ModelObject::AddPSFVector!\n\n");
  	  exit(-1);
    }
  }

  /* Define the size of the requested model image */
  theModel->SetupModelImage(nColumns, nRows);
  // Add oversampled PSF image vector and corresponding info, if present
  if (options.psfOversampledImagePresent)
    theModel->AddOversampledPSFVector(nPixels_psf_oversampled, nColumns_psf_oversampled, 
    			nRows_psf_oversampled, psfOversampledPixels, options.psfOversamplingScale,
    			x1_oversample, x2_oversample, y1_oversample, y2_oversample);

  theModel->PrintDescription();
  theModel->SetDebugLevel(options.debugLevel);


  // Set up parameter vector(s), now that we know how many total parameters
  // there will be
  nParamsTot = theModel->GetNParams();
  printf("%d total parameters\n", nParamsTot);
  if (nParamsTot != (int)parameterList.size()) {
  	fprintf(stderr, "*** ERROR: number of input parameters (%d) does not equal", 
  	       (int)parameterList.size());
  	fprintf(stderr, " required number of parameters for specified functions (%d)!\n\n",
  	       nParamsTot);
  	exit(-1);
 }
    
  /* Copy parameters into C array and generate the model image */
  paramsVect = (double *) calloc(nParamsTot, sizeof(double));
  for (int i = 0; i < nParamsTot; i++)
    paramsVect[i] = parameterList[i];


  if (! printFluxesOnly) {
    // OK, we're generating a normal model image
    theModel->CreateModelImage(paramsVect);
  
    // TESTING (remove later)
    if (options.printImages)
      theModel->PrintModelImage();

    /* Save model image: */
    if (options.saveImage) {
      string  progName = "makeimage ";
      progName += VERSION_STRING;
      PrepareImageComments(&imageCommentsList, progName, options.configFileName,
    					options.psfImagePresent, options.psfFileName);
      printf("\nSaving output model image (\"%s\") ...\n", options.outputImageName.c_str());
      status = SaveVectorAsImage(theModel->GetModelImageVector(), options.outputImageName, 
                        nColumns, nRows, imageCommentsList);
      if (status != 0) {
        fprintf(stderr,  "\n*** WARNING: Unable to save output image file \"%s\"!\n\n", 
        			options.outputImageName.c_str());
      }
      // code for checking PSF convolution fixes [May 2012]
      if (options.saveExpandedImage) {
        string  tempName = "expanded_" + options.outputImageName;
        printf("\nSaving full (expanded) output model image (\"%s\") ...\n", tempName.c_str());
        status = SaveVectorAsImage(theModel->GetExpandedModelImageVector(), tempName, 
                          nColumns + 2*nColumns_psf, nRows + 2*nRows_psf, imageCommentsList);
        if (status != 0) {
          fprintf(stderr,  "\n*** WARNING: Unable to save output image file \"%s\"!\n\n", 
          			tempName.c_str());
        }
      }
    }
  
    /* Save individual-function images, if requested */
    if ((options.saveImage) && (options.saveAllFunctions)) {
      string  currentFilename;
      vector<string> functionNames;
      char  numstring[21];   // large enough to hold any 64-bit integer
      int  nFuncs = theModel->GetNFunctions();
      theModel->GetFunctionNames(functionNames);
      string  newString;
      char  *new_string;
      for (int i = 0; i < nFuncs; i++) {
        currentFilename = options.functionRootName;
        sprintf(numstring, "%d", i + 1);
        currentFilename += numstring;
        currentFilename += "_";
        currentFilename += functionNames[i];
        currentFilename += ".fits";
        printf("%s\n", currentFilename.c_str());
        // Add comments for FITS header, describing this function
        asprintf(&new_string, "FUNCTION %s", functionNames[i].c_str());
        newString = new_string;
        imageCommentsList.push_back(newString);
        status = SaveVectorAsImage(theModel->GetSingleFunctionImage(paramsVect, i), currentFilename, 
                        nColumns, nRows, imageCommentsList);
        if (status != 0) {
          fprintf(stderr,  "\n*** WARNING: Unable to save output single-function image file \"%s\"!\n\n", 
          			currentFilename.c_str());
        }
        imageCommentsList.pop_back();
      }
    }
  }
  
  
  /* Estimate component fluxes, if requested */
  if (options.printFluxes) {
    int  nComponents = theModel->GetNFunctions();
    double *fluxes = (double *) calloc(nComponents, sizeof(double));
    double *fractions = (double *) calloc(nComponents, sizeof(double));
    double  fraction, totalFlux, magnitude;
    vector<string> functionNames;
    
    theModel->GetFunctionNames(functionNames);
    
    printf("\nEstimating fluxes on %d x %d image", options.estimationImageSize,
    				options.estimationImageSize);
    if (options.magZeroPoint != NO_MAGNITUDES)
      printf(" (using zero point = %g)", options.magZeroPoint);
    printf("...\n\n");
    
    totalFlux = theModel->FindTotalFluxes(paramsVect, options.estimationImageSize,
    												options.estimationImageSize, fluxes);
    printf("Component                 Flux        Magnitude  Fraction\n");
    for (int n = 0; n < nComponents; n++) {
      fraction = fluxes[n] / totalFlux;
      fractions[n] = fraction;
      printf("%-25s %10.4e", functionNames[n].c_str(), fluxes[n]);
      if (options.magZeroPoint != NO_MAGNITUDES) {
        magnitude = options.magZeroPoint - 2.5*log10(fluxes[n]);
        printf("   %6.4f", magnitude);
      }
      else
        printf("      ---");
      printf("%10.5f\n", fraction);
    }

    printf("\nTotal                     %10.4e", totalFlux);
    if (options.magZeroPoint != NO_MAGNITUDES) {
      magnitude = options.magZeroPoint - 2.5*log10(totalFlux);
      printf("   %6.4f", magnitude);
    }

    free(fluxes);
    free(fractions);
    printf("\n\n");
  }
  
  
  printf("Done!\n\n");


  // Free up memory
  if (options.psfImagePresent)
    free(psfPixels);       // allocated in ReadImageAsVector()
  if (options.psfOversampledImagePresent)
    free(psfOversampledPixels);       // allocated in ReadImageAsVector()
  free(paramsVect);
  delete theModel;
  
  return 0;
}



void ProcessInput( int argc, char *argv[], makeimageCommandOptions *theOptions )
{

  CLineParser *optParser = new CLineParser();

  /* SET THE USAGE/HELP   */
  optParser->AddUsageLine("Usage: ");
  optParser->AddUsageLine("   makeimage [options] config-file");
  optParser->AddUsageLine(" -h  --help                   Prints this help");
  optParser->AddUsageLine(" -v  --version                Prints version number");
  optParser->AddUsageLine("     --list-functions         Prints list of available functions (components)");
  optParser->AddUsageLine("     --list-parameters        Prints list of parameter names for each available function");
  optParser->AddUsageLine("");
  optParser->AddUsageLine(" -o  --output <output-image.fits>        name for output image [default = modelimage.fits]");
  optParser->AddUsageLine("     --refimage <reference-image.fits>   reference image (for image size)");
  optParser->AddUsageLine("     --psf <psf.fits>                    PSF image to use (for convolution)");
  optParser->AddUsageLine("");
  optParser->AddUsageLine("     --overpsf <psf.fits>                Oversampled PSF image to use");
  optParser->AddUsageLine("     --overpsf_scale <n>                 Oversampling scale (integer)");
  optParser->AddUsageLine("     --overpsf_region <x1:x2,y1:y2>      Section of image to convolve with oversampled PSF");
  optParser->AddUsageLine("");
  optParser->AddUsageLine("     --ncols <number-of-columns>         x-size of output image");
  optParser->AddUsageLine("     --nrows <number-of-rows>            y-size of output image");
  optParser->AddUsageLine("     --nosubsampling                     Do *not* do pixel subsampling near centers");
//  optParser->AddUsageLine("     --printimage             Print out images (for debugging)");
  optParser->AddUsageLine("");
  optParser->AddUsageLine("     --output-functions <root-name>      Output individual-function images");
  optParser->AddUsageLine("");
  optParser->AddUsageLine("     --print-fluxes           Estimate total component fluxes (& magnitudes, if zero point is given)");
  optParser->AddUsageLine(EST_SIZE_HELP_STRING);
  optParser->AddUsageLine("     --zero-point <value>     Zero point (for estimating component & total magnitudes)");
  optParser->AddUsageLine("");
  optParser->AddUsageLine("     --nosave                 Do *not* save image (for testing, or for use with --print-fluxes))");
  optParser->AddUsageLine("");
  optParser->AddUsageLine("     --max-threads <int>      Maximum number of threads to use");
  optParser->AddUsageLine("");
  optParser->AddUsageLine("     --debug <n>              Set the debugging level (integer)");
  optParser->AddUsageLine("");
  optParser->AddUsageLine("EXAMPLES:");
  optParser->AddUsageLine("   makeimage model_config_a.dat");
  optParser->AddUsageLine("   makeimage model_config_b.dat --ncols 800 --nrows 800 --psf best_psf.fits -o testimage_convolved.fits");
  optParser->AddUsageLine("   makeimage bestfit_parameters.dat --print-fluxes --zero-point 26.24 --nosave");
  optParser->AddUsageLine("");


  /* by default all options are checked on the command line and from option/resource file */
  optParser->AddFlag("help", "h");
  optParser->AddFlag("version", "v");
  optParser->AddFlag("list-functions");
  optParser->AddFlag("list-parameters");
  optParser->AddFlag("printimage");
  optParser->AddFlag("save-expanded");
  optParser->AddFlag("nosubsampling");
  optParser->AddFlag("print-fluxes");
  optParser->AddFlag("nosave");
  optParser->AddOption("output", "o");      /* an option (takes an argument) */
  optParser->AddOption("ncols");      /* an option (takes an argument), supporting only long form */
  optParser->AddOption("nrows");      /* an option (takes an argument), supporting only long form */
  optParser->AddOption("refimage");      /* an option (takes an argument), supporting only long form */
  optParser->AddOption("psf");      /* an option (takes an argument), supporting only long form */
  optParser->AddOption("overpsf");
  optParser->AddOption("overpsf_scale");
  optParser->AddOption("overpsf_region");
  optParser->AddOption("zero-point");      /* an option (takes an argument), supporting only long form */
  optParser->AddOption("estimation-size");      /* an option (takes an argument), supporting only long form */
  optParser->AddOption("output-functions");      /* an option (takes an argument), supporting only long form */
  optParser->AddOption("max-threads");      /* an option (takes an argument), supporting only long form */
  optParser->AddOption("debug");

  /* parse the command line:  */
  int status = optParser->ParseCommandLine( argc, argv );
  if (status < 0) {
    printf("\nError on command line... quitting...\n\n");
    delete optParser;
    exit(1);
  }


  /* Process the results: actual arguments, if any: */
  if (optParser->nArguments() > 0) {
    theOptions->configFileName = optParser->GetArgument(0);
    theOptions->noConfigFile = false;
  }

  /* Process the results: options */
  // First two are options which print useful info and then exit the program
  if ( optParser->FlagSet("help") || optParser->CommandLineEmpty() ) {
    optParser->PrintUsage();
    delete optParser;
    exit(1);
  }
  if ( optParser->FlagSet("version") ) {
    printf("makeimage version %s\n\n", VERSION_STRING);
    delete optParser;
    exit(1);
  }
  if (optParser->FlagSet("list-functions")) {
    PrintAvailableFunctions();
    delete optParser;
    exit(1);
  }
  if (optParser->FlagSet("list-parameters")) {
    ListFunctionParameters();
    delete optParser;
    exit(1);
  }

  if (optParser->FlagSet("printimage")) {
    theOptions->printImages = true;
  }
  if (optParser->FlagSet("save-expanded")) {
    theOptions->saveExpandedImage = true;
  }
  if (optParser->FlagSet("nosubsampling")) {
    theOptions->subsamplingFlag = false;
  }
  if (optParser->FlagSet("nosave")) {
    theOptions->saveImage = false;
  }
  if (optParser->FlagSet("print-fluxes")) {
    theOptions->printFluxes = true;
  }
  if (optParser->OptionSet("output")) {
    theOptions->outputImageName = optParser->GetTargetString("output");
    theOptions->noImageName = false;
  }
  if (optParser->OptionSet("refimage")) {
    theOptions->referenceImageName = optParser->GetTargetString("refimage");
    theOptions->noRefImage = false;
  }
  if (optParser->OptionSet("psf")) {
    theOptions->psfFileName = optParser->GetTargetString("psf");
    theOptions->psfImagePresent = true;
    printf("\tPSF image = %s\n", theOptions->psfFileName.c_str());
  }
  if (optParser->OptionSet("overpsf")) {
    theOptions->psfOversampledFileName = optParser->GetTargetString("overpsf");
    theOptions->psfOversampledImagePresent = true;
    printf("\tOversampled PSF image = %s\n", theOptions->psfOversampledFileName.c_str());
  }
  if (optParser->OptionSet("overpsf_scale")) {
    if (NotANumber(optParser->GetTargetString("overpsf_scale").c_str(), 0, kPosInt)) {
      fprintf(stderr, "*** ERROR: overpsf_scale should be a positive integer!\n");
      delete optParser;
      exit(1);
    }
    theOptions->psfOversamplingScale = atoi(optParser->GetTargetString("overpsf_scale").c_str());
    printf("\tPSF oversampling scale = %d\n", theOptions->psfOversamplingScale);
  }
  if (optParser->OptionSet("overpsf_region")) {
    theOptions->psfOversampleRegion = optParser->GetTargetString("overpsf_region");
    theOptions->oversampleRegionSet = true;
    printf("\tPSF oversampling region = %s\n", theOptions->psfOversampleRegion.c_str());
  }
  if (optParser->OptionSet("ncols")) {
    if (NotANumber(optParser->GetTargetString("ncols").c_str(), 0, kPosInt)) {
      fprintf(stderr, "*** ERROR: ncols should be a positive integer!\n\n");
      delete optParser;
      exit(1);
    }
    theOptions->nColumns = atol(optParser->GetTargetString("ncols").c_str());
    theOptions->nColumnsSet = true;
  }
  if (optParser->OptionSet("nrows")) {
    if (NotANumber(optParser->GetTargetString("nrows").c_str(), 0, kPosInt)) {
      fprintf(stderr, "*** ERROR: nrows should be a positive integer!\n\n");
      delete optParser;
      exit(1);
    }
    theOptions->nRows = atol(optParser->GetTargetString("nrows").c_str());
    theOptions->nRowsSet = true;
  }
  if (optParser->OptionSet("zero-point")) {
    if (NotANumber(optParser->GetTargetString("zero-point").c_str(), 0, kAnyReal)) {
      fprintf(stderr, "*** ERROR: zero point should be a real number!\n");
      delete optParser;
      exit(1);
    }
    theOptions->magZeroPoint = atof(optParser->GetTargetString("zero-point").c_str());
    printf("\tmagnitude zero point = %g\n", theOptions->magZeroPoint);
  }
  if (optParser->OptionSet("estimation-size")) {
    if (NotANumber(optParser->GetTargetString("estimation-size").c_str(), 0, kPosInt)) {
      fprintf(stderr, "*** ERROR: estimation size should be a positive integer!\n\n");
      delete optParser;
      exit(1);
    }
    theOptions->estimationImageSize = atol(optParser->GetTargetString("estimation-size").c_str());
  }
  if (optParser->OptionSet("output-functions")) {
    theOptions->functionRootName = optParser->GetTargetString("output-functions");
    theOptions->saveAllFunctions = true;
  }
  if (optParser->OptionSet("max-threads")) {
    if (NotANumber(optParser->GetTargetString("max-threads").c_str(), 0, kPosInt)) {
      fprintf(stderr, "*** ERROR: max-threads should be a positive integer!\n\n");
      delete optParser;
      exit(1);
    }
    theOptions->maxThreads = atol(optParser->GetTargetString("max-threads").c_str());
    theOptions->maxThreadsSet = true;
  }
  if (optParser->OptionSet("debug")) {
    if (NotANumber(optParser->GetTargetString("debug").c_str(), 0, kAnyInt)) {
      fprintf(stderr, "*** ERROR: debug should be an integer!\n");
      delete optParser;
      exit(1);
    }
    theOptions->debugLevel = atol(optParser->GetTargetString("debug").c_str());
  }

  if ((theOptions->nColumns) && (theOptions->nRows))
    theOptions->noImageDimensions = false;
  
  delete optParser;

}


// Note that we only use options from the config file if they have *not* already been set
// by the command line (i.e., command-line options override config-file values).
void HandleConfigFileOptions( configOptions *configFileOptions, makeimageCommandOptions *mainOptions )
{
	int  newIntVal;
	
  if (configFileOptions->nOptions == 0)
    return;

  for (int i = 0; i < configFileOptions->nOptions; i++) {
    
    if ((configFileOptions->optionNames[i] == kNCols1) || 
    		(configFileOptions->optionNames[i] == kNCols2)) {
      if (mainOptions->nColumnsSet) {
        printf("nColumns (x-size of image) value in config file ignored (using command-line value)\n");
      } else {
        if (NotANumber(configFileOptions->optionValues[i].c_str(), 0, kPosInt)) {
          fprintf(stderr, "*** ERROR: NCOLS should be a positive integer!\n");
          exit(1);
        }
        newIntVal = atoi(configFileOptions->optionValues[i].c_str());
        printf("Value from config file: nColumns = %d\n", newIntVal);
        mainOptions->nColumns = newIntVal;
      }
      continue;
    }
    if (configFileOptions->optionNames[i] == kNRows) {
      if (mainOptions->nRowsSet) {
        printf("nRows (y-size of image) value in config file ignored (using command-line value)\n");
      } else {
        if (NotANumber(configFileOptions->optionValues[i].c_str(), 0, kPosInt)) {
          fprintf(stderr, "*** ERROR: NROWS should be a positive integer!\n");
          exit(1);
        }
        newIntVal = atoi(configFileOptions->optionValues[i].c_str());
        printf("Value from config file: nRows = %d\n", newIntVal);
        mainOptions->nRows = newIntVal;
      }
      continue;
    }
    // we only get here if we encounter an unknown option
    printf("Unknown keyword (\"%s\") in config file ignored\n", 
    				configFileOptions->optionNames[i].c_str());
    
  }
}



/* END OF FILE: makeimage_main.cpp --------------------------------------- */
