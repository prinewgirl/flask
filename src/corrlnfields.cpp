/* corrlnfields: Written by Henrique S. Xavier on Nov-2014
   e-mail: hsxavier@if.usp.br
 */

#include <iostream>
#include "ParameterList.hpp"    // Configuration and input system.
#include "Utilities.hpp"        // Error handling, tensor allocations.
#include "gsl_aux.hpp"          // Using and reading GSL matrices.
#include "s2kit10_naive.hpp"    // For Discrete Legendre Transforms.
#include <gsl/gsl_linalg.h>     // Cholesky descomposition.
#include <gsl/gsl_randist.h>    // Random numbers.
#include <math.h>               // Log function.
#include <cstdlib>              // For function 'system'.
#include <string>               // For function 'to_string'.
#include <iomanip>              // For 'setprecision'.

std::ofstream debugfile;        // Send debugging messages to this file.



/********************/
/*** Main Program ***/
/********************/
int main (int argc, char *argv[]) {
  using std::cout; using std::endl; using std::string; using std::ofstream; // Basic stuff.
  using namespace ParDef; ParameterList config;                             // Easy configuration file use.
  char message[100];                                                        // Handling warnings and errors.
  std::string filename, tempstr;
  std::ofstream outfile, samplefile;                                        // File for output.
  std::ifstream infile;                                                     // File for input.
  enum simtype {gaussian, lognormal}; simtype dist;                         // For specifying simulation type.
  gsl_matrix *CovMatrix, **CovByl; 
  long CovSize;
  int status, i, j, l, m, Nfields;
  double *means, *variances, *shifts, **aux; 
  long long1, long2;
  // Functions defined in the end of this file:
  void CorrGauss(double **gaus1, gsl_matrix *L, double **gaus0);
  int GetGaussCorr(double *gXi, double *lnXi, int XiLength, double mean1, double shift1, double mean2, double shift2);
  int GetGaussCov(gsl_matrix *gCovar, gsl_matrix *lnCovar, double *means, double *shifts);
  double Gauss2LNvar(double gvar, double mean, double variance, double shift);
  int getll(const std::string filename);
  std::string getllstr(const std::string filename);
  std::string SampleHeader(std::string fieldsfile);
  void fz2n (int a1, int a2, int *n, int N1, int N2);
  void n2fz (int n, int *a1, int *a2, int N1, int N2);
  void test_fzij (int N1, int N2);
  std::string PrintOut(std::string prefix, int i, int j, int N1, int N2, double *x, double *y, int length);
  gsl_set_error_handler_off();                                              // !!! All GSL return messages MUST be checked !!!

  // Opening debug file for dumping information about the program:
  //debugfile.open("debug.log");
  //if (!debugfile.is_open()) warning("corrlnfields: cannot open debug file.");

  // Testing the code:
  cout << "Testing the code... "; cout.flush();
  test_fzij(3,11); test_fzij(13,4);
  cout << "done.\n";


  // Loading config file:
  if (argc<=1) { cout << "You must supply a config file." << endl; return 0;}
  config.load(argv[1]);
  cout << endl;
  cout << "-- Configuration setup:\n";
  cout << "   File: "<<argv[1]<<endl;
  config.lineload(argc, argv);
  config.show();
  cout << endl; 
  // - Lognormal or Gausian realizations:
  if (config.reads("DIST")=="LOGNORMAL") dist=lognormal;
  else if (config.reads("DIST")=="GAUSSIAN") dist=gaussian;
  else error("corrlnfields: unknown DIST: "+config.reads("DIST"));
  
  // Listing files to use based on CL_PREFIX:
  sprintf(message, "ls %s* > corrlnfields.temp", config.reads("CL_PREFIX").c_str());
  system(message);
  
  /********************************************/
  /*** PART 1: Load C(l)s and organize them ***/
  /********************************************/
  void CountEntries(std::string filename, long *nr, long *nc);
  void getcovid(const std::string filename, int *a1, int *a2, int *b1, int *b2);
  const int NCLMAX=500;
  int a1, a2, b1, b2, N1, N2, Nlinput, **fnz, **NentMat;
  long Nentries[NCLMAX], ncols;
  double ***ll, ***Cov, *wrapper[2];
  bool *fnzSet, **IsSet;
  
  // Get file list and find out how many C(l)s there are:  
  i=0; N1=0; N2=0, Nlinput=0;
  infile.open("corrlnfields.temp");
  if (!infile.is_open()) error("corrlnfields: cannot open file corrlnfields.temp");
  while (infile >> filename) {
    getcovid(filename, &a1, &a2, &b1, &b2);
    if (a1>N1) N1=a1; if (b1>N1) N1=b1;                // Get number of fields.
    if (a2>N2) N2=a2; if (b2>N2) N2=b2;                // Get number of z bins.
    CountEntries(filename, &(Nentries[i]), &ncols);    // Get number of Nls.
    if (ncols!=2) error("corrlnfields: wrong number of columns in file "+filename);
    if (Nentries[i]>Nlinput) Nlinput=Nentries[i];          // Record maximum number of ls.
    i++;
    if (i>NCLMAX) error("corrlnfields: reached maximum number of C(l)s. Increase NCLMAX.");
  }
  infile.clear();
  infile.seekg(0);
  cout << "Nfields: " << N1 << " Nzs: " << N2 << endl;
  
  // Allocate memory to store C(l)s:
  // First two indexes are CovMatrix indexes and last is for ll.
  // fnz stores the order that the fields are stored in CovMatrix.
  fnz     =     matrix<int>(0, N1*N2-1, 0, 1);                 // Records what field is stored in each element of CovMatrix.
  fnzSet  =    vector<bool>(0, N1*N2-1);                       // For bookkeeping.
  ll      = tensor3<double>(0, N1*N2-1, 0, N1*N2-1, 0, Nlinput); // Records the ll for each C(l) file. 
  Cov     = tensor3<double>(0, N1*N2-1, 0, N1*N2-1, 0, Nlinput); // Records the C(l) for each C(l) file.
  IsSet   =    matrix<bool>(0, N1*N2-1, 0, N1*N2-1);           // For bookkeeping.
  NentMat =     matrix<int>(0, N1*N2-1, 0, N1*N2-1);           // Number of C(l) entries in file.
  for(i=0; i<N1*N2; i++) for(j=0; j<N1*N2; j++) IsSet[i][j]=0;
  for(i=0; i<N1*N2; i++) fnzSet[i]=0;
  
  // Read C(l)s and store in data-cube:
  m=0;
  while (infile >> filename) {
    // Find CovMatrix indexes of C(l):
    getcovid(filename, &a1, &a2, &b1, &b2);
    //fzfz2ij(a1, a2, b1, b2, &i, &j, N1, N2); //i=(a1-1)*N2+a2; j=(b1-1)*N2+b2;
    fz2n(a1, a2, &i, N1, N2); fz2n(b1, b2, &j, N1, N2); 
    cout << filename << " goes to ["<<i<<", "<<j<<"]" << endl;
    // Record the order of the fields in CovMatrix:
    if (fnzSet[i]==0) { fnz[i][0] = a1; fnz[i][1] = a2; fnzSet[i] = 1; }
    else if (fnz[i][0] != a1 || fnz[i][1] != a2) error("corrlnfields: field order in CovMatrix is messed up!"); 
    if (fnzSet[j]==0) { fnz[j][0] = b1; fnz[j][1] = b2; fnzSet[j] = 1; }
    else if (fnz[j][0] != b1 || fnz[j][1] != b2) error("corrlnfields: field order in CovMatrix is messed up!");
    // Import data:
    wrapper[0] = &(ll[i][j][0]);
    wrapper[1] = &(Cov[i][j][0]);
    ImportVecs(wrapper, Nentries[m], 2, filename.c_str());
    NentMat[i][j] = Nentries[m];
    IsSet[i][j]=1; 
    m++;
  };
  infile.close();

  // Check if every field was assigned a position in the CovMatrix:
  for (i=0; i<N1*N2; i++) if (fnzSet[i]==0) error("corrlnfields: some position in CovMatrix is unclaimed.");
  free_vector(fnzSet, 0, N1*N2-1);
  // If positions are OK and output required, print them out:
  if (config.reads("FLIST_OUT")!="0") {
    outfile.open(config.reads("FLIST_OUT").c_str());
    if (!outfile.is_open()) error("corrlnfields: cannot open FLIST_OUT file.");
    PrintTable(fnz, N1*N2, 2, &outfile);
    outfile.close();
    cout << "Written field list to "+config.reads("FLIST_OUT")<<endl;
  }
  
  /**************************************************/
  /*** PART 2: Prepare for Cholesky decomposition ***/
  /**************************************************/
  double *tempCl, *LegendreP, *workspace, *xi, lsup, supindex, *theta, *DLTweights, *lls;
  const int HWMAXL = 10000000; int lastl = HWMAXL, Nls;
  
  // Load means and shifts data file:
  cout << "Loading means and shifts from file "+config.reads("MEANS_SHIFTS")+":\n";
  aux   = LoadTable<double>(config.reads("MEANS_SHIFTS"), &long1, &long2); 
  Nfields = (int)long1; // From now on will use Nfields instead of N1*N2, so the check below is important!
  if (Nfields != N1*N2) error("corrlnfields: number of means and shifts do not match number of C(l)s.");
  fnzSet = vector<bool>(0, Nfields-1); for (i=0; i<Nfields; i++) fnzSet[i]=0;
  means  = vector<double>(0, Nfields-1); 
  if (dist==lognormal) shifts = vector<double>(0, Nfields-1);
  for (j=0; j<Nfields; j++) {
    fz2n((int)aux[j][0], (int)aux[j][1], &i, N1, N2); // Find conventional position of field in arrays.
    if (fnzSet[i]==1) error ("corrlnfields: found more than one mean & shift entry for the same f-z.");
    fnzSet[i] = 1; 
    means[i]  = aux[j][2];  
    if (dist==lognormal) shifts[i] = aux[j][3];
  }
  for (i=0; i<Nfields; i++) if (fnzSet[i]!=1) error("corrlnfields: some mean & shift were not set.");
  if (dist==lognormal) for (i=0; i<Nfields; i++) if(means[i]+shifts[i]<=0) { // Sanity check.
	printf(message, "corrlnfields: mean+shift at position %d must be greater than zero.", i); error(message);
      }
  free_vector(fnzSet, 0, Nfields-1);
  free_matrix(aux, 0, Nfields-1, 0, long2-1);
  cout << "Done.\n";
  
  // Look for the maximum l value described by all C(l)s:
  for(i=0; i<Nfields; i++) for(j=0; j<Nfields; j++) if (IsSet[i][j]==1) {
	if (ll[i][j][NentMat[i][j]-1]>HWMAXL) error ("corrlnfields: too high l in C(l)s: increase HWMAXL.");
	if (ll[i][j][NentMat[i][j]-1]<lastl) lastl = (int)ll[i][j][NentMat[i][j]-1];
      }
  Nls=lastl+1; // l=0 is needed for DLT. Nls is known as 'bandwidth' (bw) in s2kit 1.0 code.

  // Allocate gsl_matrices that will receive covariance matrices for each l.
  cout << "Allocating data-cube necessary for Cholesky decomposition... "; cout.flush();
  tempCl = vector<double>(0, lastl);
  CovByl = GSLMatrixArray(Nls, Nfields, Nfields);
  cout << "done.\n";


  /*****************************************************************/
  /*** Initialization necessary in case of lognormal simulations ***/
  /*****************************************************************/
  if (dist==lognormal) {
    cout << "LOGNORMAL realizations: will compute auxiliary gaussian C(l)s:\n";
    // Loads necessary memory:
    cout << "Allocating extra memory... "; cout.flush();
    workspace  = vector<double>(0, 16*Nls-1);
    LegendreP  = vector<double>(0, 2*Nls*Nls-1);
    xi         = vector<double>(0, 2*Nls-1);
    theta      = vector<double>(0, 2*Nls-1);
    lls        = vector<double>(0, lastl);
    DLTweights = vector<double>(0, 4*Nls-1);
    // Initialize vectors:
    for (i=0; i<=lastl; i++) lls[i]=(double)i;
    ArcCosEvalPts(2*Nls, theta);
    for (i=0; i<2*Nls; i++) theta[i] = theta[i]*180.0/M_PI; 
    cout << "done.\n";

    // Loads C(l) exponential suppression:
    lsup     = config.readd("SUPPRESS_L");
    supindex = config.readd("SUP_INDEX"); 
    
    // Load s2kit 1.0 Legendre Polynomials:
    cout << "Generating table of Legendre polynomials and sampling angles... "; cout.flush();
    PmlTableGen(Nls, 0, LegendreP, workspace);
    cout << "done.\n";

    // Compute s2kit 1.0 Discrete Legendre Transform weights:
    cout << "Calculating forward DLT weights... "; cout.flush();
    makeweights(Nls, DLTweights);
    cout << "done.\n";
  }

  // LOOP over all C(l)s already set.
  for(i=0; i<Nfields; i++)
    for(j=0; j<Nfields; j++) 
      if (IsSet[i][j]==1) {
	cout << "** Transforming C(l) in ["<<i<<", "<<j<<"]:\n";
	// Interpolate C(l) for every l; input C(l) might not be like that:
	cout << "   Interpolating input C(l) for all l's... "; cout.flush();
	GetAllLs(ll[i][j], Cov[i][j], NentMat[i][j], tempCl, lastl);
	cout << "              done.\n";
	
	if (dist==lognormal) {              /** LOGNORMAL ONLY **/
	  // Compute correlation function:
	  cout << "   DLT (inverse) to obtain the correlation function... "; cout.flush();
	  ModCl4DLT(tempCl, lastl, lsup, supindex);
	  Naive_SynthesizeX(tempCl, Nls, 0, xi, LegendreP);
	  cout << "  done.\n";
	  if (config.reads("XIOUT_PREFIX")!="0") { // Write it out if requested:
	    filename=PrintOut(config.reads("XIOUT_PREFIX"), i, j, N1, N2, theta, xi, 2*Nls);
	    cout << "   Correlation function written to "+filename<<endl;
	  }
	  // Transform Xi(theta) to auxiliary gaussian Xi(theta):
	  cout << "   Computing associated gaussian correlation function... "; cout.flush(); 
	  status=GetGaussCorr(xi, xi, 2*Nls, means[i], shifts[i], means[j], shifts[j]);
	  cout << "done.\n";
	  if (status==EDOM) error("corrlnfields: GetGaussCorr found bad log arguments.");
	  if (config.reads("GXIOUT_PREFIX")!="0") { // Write it out if requested:
	    filename=PrintOut(config.reads("GXIOUT_PREFIX"), i, j, N1, N2, theta, xi, 2*Nls);
	    cout << "   Associated Gaussian correlation function written to "+filename<<endl;
	  }
	  // Transform Xi(theta) back to C(l):
	  cout << "   DLT (forward) to obtain the angular power spectrum... "; cout.flush(); 
	  Naive_AnalysisX(xi, Nls, 0, DLTweights, tempCl, LegendreP, workspace);
	  ApplyClFactors(tempCl, Nls);
	  cout << "done.\n";
	  if (config.reads("GCLOUT_PREFIX")!="0") { // Write it out if requested:
	    filename=PrintOut(config.reads("GCLOUT_PREFIX"), i, j, N1, N2, lls, tempCl, Nls);
	    cout << "   C(l) for auxiliary Gaussian variables written to "+filename<<endl;
	  }	  
	}                                 /** END OF LOGNORMAL ONLY **/ 
	
	// Save gaussian C(l):
	for (l=0; l<Nls; l++) CovByl[l]->data[i*Nfields+j]=tempCl[l];		
      } // End of LOOP over C(l)[i,j] that were set.
  
  // Freeing memory: from now on we only need CovByl, means, shifts, fnz.
  cout << "Massive memory deallocation... "; cout.flush();
  free_vector(tempCl, 0, lastl);
  free_tensor3(Cov,    0, Nfields-1, 0, Nfields-1, 0, Nlinput); 
  free_tensor3(ll,     0, Nfields-1, 0, Nfields-1, 0, Nlinput); 
  free_matrix(NentMat, 0, Nfields-1, 0, Nfields-1);
  if (dist==lognormal) {
    free_vector(workspace, 0, 16*Nls-1);
    free_vector(LegendreP, 0, 2*Nls*Nls-1);
    free_vector(xi, 0, 2*Nls-1);
    free_vector(theta, 0, 2*Nls-1);
    free_vector(lls, 0, lastl);
    free_vector(DLTweights, 0, 4*Nls-1); 
  }
  cout << "done.\n";

  
  // Set Cov(l)[i,j] = Cov(l)[j,i]
  cout << "Set remaining covariance matrices elements based on symmetry... "; cout.flush(); 
  for(i=0; i<Nfields; i++)
    for(j=0; j<Nfields; j++) 
      if (IsSet[i][j]==0) {
	if (IsSet[j][i]==0) {
	  sprintf(message,"corrlnfields: [%d,%d] could not be set because [%d,%d] was not set.",i,j,j,i);
	  error(message);
	}
	for (l=0; l<Nls; l++) CovByl[l]->data[i*Nfields+j] = CovByl[l]->data[j*Nfields+i];
	IsSet[i][j] = 1;
      }
  cout << "done.\n";
  free_matrix(IsSet, 0, Nfields-1, 0, Nfields-1);
  
  
  /******************************************/
  /*** PART 3: The Cholesky decomposition ***/
  /******************************************/
  const double OneOverSqr2=0.7071067811865475;
  double **gaus0, **gaus1, ***almRe, ***almIm;
  gsl_rng *rnd;
  int lmax, lmin, mMin, mMax, NofM;
 
  lmax = config.readi("LMAX");
  lmin = config.readi("LMIN");
  NofM = config.readi("NM");
  if (NofM<0) {
    mMin = -lmax;
    mMax =  lmax;
  }
  else { 
    mMin = -1*NofM/2 + (NofM+1)%2; 
    mMax =    NofM/2;
  }

  // Allocate memory:
  gaus0 = matrix<double>(0,Nfields-1, 0,1); // Complex random variables, [0] is real, [1] is imaginary part.
  gaus1 = matrix<double>(0,Nfields-1, 0,1); 
  //almRe = tensor3<double>(0, lmax, 0, NofM-1,0,Nfields-1); // Temporary, must think of something clever.
  //almIm = tensor3<double>(0, lmax, 0, NofM-1,0,Nfields-1); // This fails of NofM<0!!!
  almRe = tensor3<double>(lmin, lmax, mMin, mMax,0,Nfields-1); // Temporary, must think of something clever.
  almIm = tensor3<double>(lmin, lmax, mMin, mMax,0,Nfields-1); // This fails of NofM<0!!!


  if (dist==lognormal) variances = vector<double>(0,Nfields-1);
  // - Set random number generator
  rnd = gsl_rng_alloc(gsl_rng_mt19937);
  if (rnd==NULL) error("corrlnfields: gsl_rng_alloc failed!");
  gsl_rng_set(rnd, config.readi("RNDSEED"));    // set random seed
  // Open file to receive alm's:
  samplefile.open(config.reads("SAMPLE_OUT").c_str());
  if (!samplefile.is_open()) 
    warning("corrlnfields: cannot open "+config.reads("SAMPLE_OUT")+" file.");
  samplefile << SampleHeader(config.reads("FLIST_IN")) <<endl<<endl; //Maybe here we should use fnz from RAM.
  

  // LOOP over l's:
  for (l=lmin; l<=lmax; l++) { // skip l=0.
    cout << "** Working with cov. matrix for l="<<l<<":\n";
    // Get variances from Cov. matrix:
    if (dist==lognormal) {
      for (i=0; i<Nfields; i++) variances[i] = CovByl[l]->data[i*Nfields+i];  
    }
    
    // Perform a Cholesky decomposition:
    cout << "   Performing a Cholesky decomposition... "; cout.flush();
    status = gsl_linalg_cholesky_decomp(CovByl[l]);
    if (status==GSL_EDOM) error("Cholesky decomposition failed: matrix is not positive-definite.");
    cout << "done.\n";
    // Output file if requested:
    if (config.reads("CHOLESKY_PREFIX")!="0") {
      filename=config.reads("CHOLESKY_PREFIX")+"l"+ZeroPad(l,lmax)+".dat";
      outfile.open(filename.c_str());
      if (!outfile.is_open()) warning("corrlnfields: cannot open file "+filename);
      else { 
	PrintGSLMatrix(CovByl[l], &outfile); outfile.close();
	cout << "   Cholesky decomposition output matrix written to " << filename << endl;
      }  
    }

    // LOOP over realizations of alm's for a fixed l:
    cout << "   Generating random alm's... "; cout.flush();
    if (NofM<0) { mMin=-l; mMax=l; }
    for (m=mMin; m<=mMax; m++) {
      // Generate independent 1sigma complex random variables:
      for (i=0; i<Nfields; i++) {
	gaus0[i][0] = gsl_ran_gaussian(rnd, OneOverSqr2);
	gaus0[i][1] = gsl_ran_gaussian(rnd, OneOverSqr2);
      }
      // Generate correlated complex gaussian variables according to CovMatrix:
      CorrGauss(gaus1, CovByl[l], gaus0);
      for (i=0; i<Nfields; i++) {
	almRe[l][m][i] = gaus1[i][0];
	almIm[l][m][i] = gaus1[i][1];
      }
      samplefile << l <<" "<< m;
      for (i=0; i<Nfields; i++) samplefile <<" "<<std::setprecision(10)<< almRe[l][m][i]<<" "<<std::setprecision(10)<< almIm[l][m][i];
      samplefile<<endl;
      /*
      // If DIST=LOGNORMAL, transform variables to lognormal:
      if (dist==lognormal)
	for (i=0; i<Nfields; i++) gaus0[i] = Gauss2LNvar(gaus1[i], means[i], variances[i], shifts[i]);
      else
	for (i=0; i<Nfields; i++) gaus0[i] = gaus1[i] + means[i];
      
      //sprintf(message,"%d %d %.8g %.8g %.8g %.8g",l,m,gaus0[0],gaus0[1],gaus0[2],gaus0[3]);
      //samplefile << message << endl;
      samplefile << l <<" "<< m;
      for (i=0; i<CovSize; i++) samplefile <<" "<<std::setprecision(10)<< gaus0[i];
      samplefile<<endl;
      */
    } // End of LOOP over realizations of alm's for fixed l.
    cout << "done.\n";
    
  } // End of LOOP over l's.
  samplefile.close();
  free_matrix(gaus0,0,CovSize-1,0,1);
  free_matrix(gaus1,0,CovSize-1,0,1);
  if (dist==lognormal) free_vector(variances, 0, CovSize-1);
  if (NofM<0) {mMin = -lmax; mMax = lmax;}
  free_tensor3(almRe, lmin, lmax, mMin, mMax, 0, Nfields-1);
  free_tensor3(almIm, lmin, lmax, mMin, mMax, 0, Nfields-1);
  free_GSLMatrixArray(CovByl, Nls);
  
  // End of the program
  if (dist==lognormal) free_vector(shifts, 0, Nfields-1);
  free_vector(means, 0, Nfields-1);
  system("rm -f corrlnfields.temp");
  gsl_rng_free(rnd);
  //debugfile.close();
  cout << "\nTotal number of warnings: " << warning("count") << endl;
  cout<<endl;
  return 0;
}


/*** Multiply Lower-triangular matrix L to complex vector gaus0 and return gaus1 ***/
void CorrGauss(double **gaus1, gsl_matrix *L, double **gaus0) {
  long i, j;

  for(i=0; i<L->size1; i++) {
    gaus1[i][0]=0; // Re    
    gaus1[i][1]=0; // Im
    for(j=0; j<=i; j++) { // L matrix stored as vector in row-major order.
      gaus1[i][0] += L->data[i*L->size1+j] * gaus0[j][0];
      gaus1[i][1] += L->data[i*L->size1+j] * gaus0[j][1];
    }
  }
}


/*** Transforms a correlation function of lognormal variables lnXi into a corr. function of associated gaussian variables gXi ***/
int GetGaussCorr(double *gXi, double *lnXi, int XiLength, double mean1, double shift1, double mean2, double shift2) {
  int i, status=0;
  double arg, bad=-666.0;
  char message[100];
  
  for (i=0; i<XiLength; i++) {
    arg = 1.0 + lnXi[i]/(mean1+shift1)/(mean2+shift2);
    if (arg <= 0) {
      sprintf(message, "GetGaussCorr: lnXi[%d] leads to bad log argument, gXi[%d] set to %g.", i, i, bad);
      warning(message);
      status=EDOM;
      gXi[i] = bad;
    }
    else gXi[i] = log(arg);
  }
  return status;
}


/*** Transform Cov. matrix of lognormal variables into 
     the Cov. matrix of associated gaussian variables  ***/
int GetGaussCov(gsl_matrix *gCovar, gsl_matrix *lnCovar, double *means, double *shifts) {
  double arg;
  long i, j;
  char message[100]; int status=0; double bad=-666.0;
  
  for(i=0; i<lnCovar->size1; i++)
    for(j=0; j<lnCovar->size2; j++) {
      arg = lnCovar->data[i*(lnCovar->size1)+j]/(means[i]+shifts[i])/(means[j]+shifts[j]) + 1.0;
      // If arg<0, it is an error.
      if (arg<=0) {
	sprintf(message,"GetGaussCov: lnCovar[%ld][%ld] leads to bad log argument, gCovar[%ld][%ld] set to %g.",i,j,i,j,bad);
	warning(message);
	status=EDOM;
	gCovar->data[i*(gCovar->size1)+j] = bad;
      }
      // If arg>0, it is valid.
      else gCovar->data[i*(gCovar->size1)+j] = log(arg);  
    }
  return status;
}


/*** Compute a log-normal variable from a gaussian one ***/
double Gauss2LNvar(double gvar, double mean, double variance, double shift) {
  double expsigma2, expmu;
  
  expsigma2 = 1 + variance/pow(mean+shift,2);
  expmu = (mean+shift)/sqrt(expsigma2);

  return expmu*exp(gvar)-shift;
}


/*** Get a number that specify the l of the cov. matrix ***/
int getll(const std::string filename) {
  int i=0, num=0, fileL;
  
  fileL=filename.length();
  // Find a number:
  while (isdigit(filename.c_str()[i])==0) {i++; if(i>=fileL) error("getll: cannot find any number.");}
  // Read the number:
  while (isdigit(filename.c_str()[i])!=0) {num = num*10 + (filename.c_str()[i]-'0'); i++;}
  // Check if there are more numbers in filename:
  while (i<=fileL) {
    if (isdigit(filename.c_str()[i])!=0) error("getll: found more numbers than expected.");
    i++;
  }

  // Return number found:
  return num;
}


/*** Returns getll as string ***/
std::string getllstr(const std::string filename) {
  std::stringstream ss;
  ss << getll(filename);
  return ss.str();
}


/*** Function for writing the header of alm's file ***/
std::string SampleHeader(std::string fieldsfile) {
  std::stringstream ss;
  int **fz; 
  long nr, nc, i, j;

  fz = LoadTable<int>(fieldsfile, &nr, &nc, 1);
  if (nc!=2) error("SampleHeader: expect 2 columns in file "+fieldsfile);
 
  ss << "# l, m";
  for (i=1; i<=nr; i++) ss << ", f" << fz[i][1] << "z" << fz[i][2] << "Re, f" << fz[i][1] << "z" << fz[i][2] << "Im";
  
  return ss.str(); 
}



/*** Get four numbers separated by characters that specify the fields and redshifts
     of the correlation function. ***/
void getcovid(const std::string filename, int *a1, int *a2, int *b1, int *b2) {
  int i=0, num, index, fileL;
  
  fileL=filename.length();
  // LOOP over the four indexes that indentifies the C(l):
  for(index=1; index<=4; index++) {
    num=0;
    // Find a number:
    while (isdigit(filename.c_str()[i])==0) {i++; if(i>=fileL && index!=4) error("getcovid: cannot find four numbers.");}
    // Read the number:
    while (isdigit(filename.c_str()[i])!=0) {num = num*10 + (filename.c_str()[i]-'0'); i++;}
    // Save it as an index:
    switch (index) {
    case 1: *a1 = num; break;
    case 2: *a2 = num; break;
    case 3: *b1 = num; break;
    case 4: *b2 = num; break;
    }
  }
  // Check if there are more numbers in filename:
  while (i<=fileL) {
    if (isdigit(filename.c_str()[i])!=0) error("getcovid: found more numbers than expected.");
    i++;
  }
}


/*** Find out number of columns and rows in file ***/
void CountEntries(std::string filename, long *nr, long *nc) {
  using std::ifstream;
  using std::string;
  using std::istringstream;
  using std::ostringstream;
  long nrows=0, ncols=0;
  ifstream file;
  istringstream inputline; ostringstream outputline;
  string word, phrase;
  
  // Open file
  file.open(filename.c_str());
  if (!file.is_open()) error("CountEntries: cannot open file.");
  
  // Count lines and columns:
  getline(file,phrase);
  outputline << phrase;
  inputline.str(outputline.str());
  while (inputline >> word) ncols++;
  while(!file.eof()) {getline(file,phrase); nrows++;}

  file.close();
  *nr=nrows+1;
  *nc=ncols;
}


/*** Assign a matrix column n to a variable 'a' identified by a1 and a2  ***/
void fz2n (int a1, int a2, int *n, int N1, int N2) {
  if (a2>N2 || a1>N1 || a1<1 || a2<1) warning("fz2n: unexpected input values.");
  *n = (a1-1)*N2+a2-1; 
}


/*** The inverse of ij2fzfz above ***/
void n2fz (int n, int *a1, int *a2, int N1, int N2) {
  if (n<0 || n>=N1*N2) warning("n2fz: unexpected input values.");
  *a2 = n%N2+1;
  *a1 = n/N2+1;
}


/*** Assign a matrix row i to a variable 'a' identified by a1 and a2 ***/
/*** Assign a matrix column j to a variable 'b' identified by b1 and b2  ***/
void fzfz2ij (int a1, int a2, int b1, int b2, int *i, int *j, int N1, int N2) {
  if (a2>N2 || b2>N2 || a1>N1 || b1>N1 || a1<1 || a2<1 || b1<1 || b2<1) warning("fzfz2ij: unexpected input values.");
  fz2n(a1, a2, i, N1, N2);
  fz2n(b1, b2, j, N1, N2);
}

/*** The inverse of ij2fzfz above ***/
void ij2fzfz (int i, int j, int *a1, int *a2, int *b1, int *b2, int N1, int N2) {
  if (i<0 || j<0 || i>=N1*N2 || j>=N1*N2) warning("ij2fzfz: unexpected input values.");
  n2fz(i, a1, a2, N1, N2);
  n2fz(j, b1, b2, N1, N2);
}


/*** Function for testing the assignments above ***/
void test_fzij (int N1, int N2) {
  int a1, a2, b1, b2, i, j, newa1, newa2, newb1, newb2;
  bool **IsSet;

  IsSet = matrix<bool>(0,N1*N2-1,0,N1*N2-1);
  for(i=0; i<N1*N2; i++) for(j=0; j<N1*N2; j++) IsSet[i][j]=0;
  
  for (a1=1; a1<=N1; a1++)
    for (a2=1; a2<=N2; a2++)
      for (b1=1; b1<=N1; b1++)
	for (b2=1; b2<=N2; b2++) {
	  fzfz2ij(a1, a2, b1, b2, &i, &j, N1, N2); 
	  if (IsSet[i][j]==1) error("test_fzij: tried to set [i,j] already set.");
	  IsSet[i][j]=1;
	  ij2fzfz(i, j, &newa1, &newa2, &newb1, &newb2, N1, N2);
	  if(newa1!=a1 || newa2!=a2 || newb1!=b1 || newb2!=b2) error("test_fzij: function ij2fzfz not the inverse of fzfz2ij."); 
	}
  for(i=0; i<N1*N2; i++) for(j=0; j<N1*N2; j++) if (IsSet[i][j]!=1) error("Matrix [i,j] not fully populated.");

  free_matrix(IsSet,0,N1*N2-1,0,N1*N2-1);
}



/*** Export function y(x) for the field combination [i,j] to file ***/
std::string PrintOut(std::string prefix, int i, int j, int N1, int N2, double *x, double *y, int length) {
  int a1, a2, b1, b2;
  char message[100];
  std::string filename;
  double *wrapper[2];
  std::ofstream outfile;

  wrapper[0] =  x;
  wrapper[1] =  y;

  n2fz(i, &a1, &a2, N1, N2); n2fz(j, &b1, &b2, N1, N2);
  sprintf(message, "%sf%dz%df%dz%d.dat", prefix.c_str(),a1,a2,b1,b2);
  filename.assign(message);

  outfile.open(message);
  if (!outfile.is_open()) error("PrintOut: cannot open file "+filename);
  PrintVecs(wrapper, length, 2, &outfile);
  outfile.close();

  return filename;
}
