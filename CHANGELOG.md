# Change Log for Imfit

(Formatting and design based on Olivier Lacan's [Keep a CHANGELOG](http://keepachangelog.com/))


## 1.2 -- 2015-05-xx
### Added: 
- Imfit can now optionally convolve part of the model image with an
oversampled PSF. E.g., you can specify that a 10x10-pixel region
centered on the galaxy nucleus should be computed using a
five-times-smaller pixel size and convolved with a corresponding
(five-times-oversampled) PSF image. The resulting oversampled and
convolved sub-image is then downsampled back to the main image pixel
scale before the model is compared with the data image (This is in
addition to the standard PSF convolution that imfit already allows,
where the PSF image and the computed model image have the same pixel
scale as the data image.)

- The saved best-fit parameter file produced by imfit now include a
brief summary of the fitting process and its outcome (fitting statistic
and minimization algorithm used, final best-fit value of statistic, AIC,
BIC), written in the header of the file as comments. (Thanks to Colleen Gilhuly
for suggesting this.)

- Imfit now attempts to estimate (and print) the total amount of memory
needed before it starts the fitting process. This estimate is crude (and
purely advisory), but may be useful in cases when fitting very large
images (and/or using very large PSF images) might run up against your
computer's memory limits.  Note that this currently does *not* account
for any use of an oversampled PSF. (Thanks to Lee Kelvin for helping demonstrate
the utility of this.)

### Changed:
- Updates to documentation.



## 1.1 -- 2014-10-22
### Added:
- New version of Poisson-based fit statistic for minimization ("Poisson 
maximum-likelihood-ratio statistic", via --poisson-mlr/--mlr flag) which is always >= 0, 
and can thus be used with Levenberg-Marquardt minimization, unlike the Cash statistic;
actual fit results should be effectively identical to Cash-statistic fits. 
Thanks to David Streich for pointing out this possibility.

- Full bootstrap-resampling output (i.e., the individual best-fit parameters from
each bootstrap iteration) can now be saved to a text file for later analysis,
via the --save-bootstrap option. (Thanks to David Streich for suggesting this.)

- Extra minimization algorithms from the NLopt library, specified via
the --nlopt command-line option (basically, this includes all the "local
derivative-free optimization" algorithms from NLopt; see
[http://ab-initio.mit.edu/wiki/index.php/NLopt_Algorithms](http://ab-
initio.mit.edu/wiki/index.php/NLopt_Algorithms) for more details). The
Nelder-Mead simplex algorithm is of course still available via its usual
flag (--nm), and is probably the best of all the NLopt algorithms for
general image-fitting.

- Command-line flag --fitstat-only, which is a synonym for --chisquare-only.

### Changed:
- Minor changes to the wording of help text and error messages.

- Intermediate output from Levenberg-Marquardt fitting now uses the term
"fit statistic" instead of "chi^2" (since L-M can now minimize modified Cash
statistic as well).

- Updates to documentation.



## 1.0.3 -- 2014-07-21
### Fixed:
- Fixes a bug in the newly introduced "--noisy" printing mode which
would print scrambled parameter values during the fit if one or more of
the parameters were held fixed. (This bug did *not* affect the actual
fitting or the final output values of the best-fit parameters.) Thanks
to Giulia Savorgnan for spotting the bug so quickly!



## 1.0.2 -- 2014-07-17
### Added:
- A new command-line option "--loud" (i.e., the opposite of "--quiet"),
which causes the L-M and N-M minimizers to print current model parameter
values during the fitting process (once per iteration for L-M and once
per 100 iterations for N-M simplex).

### Changed:
- Pixels with non-finite values in the image to be fitted are now
automatically masked (instead of causing imfit to complain and quit);
thanks to Giulia Savorgnan for suggesting this.

- Pixels in the input and/or error images which would produce non-finite
weight values (in the case of data-based chi^2 fitting) -- e.g., data
values <= 0 after correcting for any previous sky subtraction, for which
1/sqrt(data) produces non-finite values -- are now ignored *if* they are
masked (instead of causing imfit to complain and quit); thanks to
Francicso Carrera for suggesting this.

- The old random-number-generator code in the DE minimizer has been
replaced with same Mersenne Twister algorithm used elsewhere. This is
significant because the old RNG used the same seed each time, so the
sequence of "random numbers" it generated was always the same... This
means that subsequent fits using DE on the same input will no longer
produce exactly the same output parameters

- Fitting small images (e.g., <~ 200 x 200 pixels) on systems with many
(e.g., > 8) cores is now significantly faster; thanks to André
Luiz de Amorim for investigating this & figuring out how to make it
happen.

- Error messages are now sent to stderr rather than stdout (probably not
noticeably different unless you redirect the output a lot); some slight
changes in wording of error messages.



## 1.0.1 -- 2014-02-21
### Added:
- Added explicit printing of which statistic is being minimized at the
start of the minimization process.

### Changed:
- Added use of DE for bootstrap resampling if Cash statistic is used and
NLopt library wasn't available.

- Minor improvements to the documentation.

### Fixed:
- Fixed an uninitialized boolean-flag bug which was causing OpenMP failures 
(on at least some 32-bit Linux systems).

- Fixed handling of --no-openmp compilation option in SConstruct file so
that it actually turns off use of OpenMP (thanks to Sergio Pascual for
identifying this problem).

- Fixed handling of --no-nlopt compilation option (NO_NLOPT preprocessor
definition) in bootstrap-resampling code (thanks to Guillermo Barro for
identifying this problem).

- Fixed compiled Mac version so that it properly includes static library
code for NLopt (thanks to Giulia Savorgnan for spotting this problem).



## 1.0 -- 2013-11-06
Initial public release.