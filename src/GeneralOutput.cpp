#include <healpix_map_fitsio.h> // For FITS files.
#include <levels_facilities.h>  // For something related to Healpix I don't remember.
#include <iomanip>              // For setprecision.
#include "GeneralOutput.hpp"    // I don't know why, maybe to avoid mismatches.
#include "corrlnfields_aux.hpp" // For SampleHeader and n2fz functions.
#include "Utilities.hpp"        // For warnings, errros and dynamic allocation.



/********************/
/*** Alm's output ***/
/********************/


// Prints ALL fields alm's to a TEXT file labeled by their FIELD IDs.
void GeneralOutput(Alm<xcomplex <double> > *af, const ParameterList & config, std::string keyword, int **fnz, int Nfields) {
  std::string filename;
  std::ofstream outfile; 
  int lminout, lmaxout, mmax, l, m, i;

  // If requested, write alm's to file:
  if (config.reads(keyword)!="0") {
    filename = config.reads(keyword);
    outfile.open(filename.c_str());
    if (!outfile.is_open()) warning("GeneralOutput: cannot open "+filename+" file.");
    outfile << SampleHeader(fnz, Nfields) <<std::endl<<std::endl;
    lminout = config.readi("LRANGE_OUT", 0);
    lmaxout = config.readi("LRANGE_OUT", 1);
    mmax = config.readi("MMAX_OUT");
    if (mmax>lminout) error ("GeneralOutput: current code only allows MMAX_OUT <= LMIN_OUT.");
    // Output all alm's:
    if (mmax<0) {
      for(l=lminout; l<=lmaxout; l++)
	for(m=0; m<=l; m++) {
	  outfile << l <<" "<< m;
	  for (i=0; i<Nfields; i++) outfile <<" "<<std::setprecision(10)<< af[i](l,m).re<<" "<<std::setprecision(10)<< af[i](l,m).im;
	  outfile<<std::endl;
	} 
    }
    // Truncate m in alm output:
    else {
     for(l=lminout; l<=lmaxout; l++)
	for(m=0; m<=mmax; m++) {
	  outfile << l <<" "<< m;
	  for (i=0; i<Nfields; i++) outfile <<" "<<std::setprecision(10)<< af[i](l,m).re<<" "<<std::setprecision(10)<< af[i](l,m).im;
	  outfile<<std::endl;
	}  
    }
    outfile.close();
    std::cout << ">> "+keyword+" written to "+filename<<std::endl;
  }
}


// Prints one single alm table to a TEXT file using a PREFIX and a FIELD ID.
void GeneralOutput(const Alm<xcomplex <double> > & a, const ParameterList & config, std::string keyword, int *fnz) {
  std::string filename;
  std::ofstream outfile; 
  char message[100];
  int lminout, lmaxout, mmax, l, m;

  // If requested, write alm's to file:
  if (config.reads(keyword)!="0") {
    sprintf(message, "%sf%dz%d.dat", config.reads(keyword).c_str(), fnz[0], fnz[1]);
    filename.assign(message);
    
    outfile.open(message);
    if (!outfile.is_open()) warning("GeneralOutput: cannot open "+filename+" file.");
    outfile << "# l, m, Re(alm), Im(alm)"<<std::endl<<std::endl;
    lminout = config.readi("LRANGE_OUT", 0);
    lmaxout = config.readi("LRANGE_OUT", 1);
    mmax = config.readi("MMAX_OUT");
    if (mmax>lminout) error ("GeneralOutput: current code only allows MMAX_OUT <= LMIN_OUT.");
    // Output all alm's:
    if (mmax<0) {
      for(l=lminout; l<=lmaxout; l++)
	for(m=0; m<=l; m++) {
	  outfile << l <<" "<< m;
	  outfile <<" "<<std::setprecision(10)<< a(l,m).re<<" "<<std::setprecision(10)<< a(l,m).im;
	  outfile<<std::endl;
	} 
    }
    // Truncate m in alm output:
    else {
     for(l=lminout; l<=lmaxout; l++)
	for(m=0; m<=mmax; m++) {
	  outfile << l <<" "<< m;
	  outfile <<" "<<std::setprecision(10)<< a(l,m).re<<" "<<std::setprecision(10)<< a(l,m).im;
	  outfile<<std::endl;
	}  
    }
    outfile.close();
    std::cout << ">> "+keyword+" written to "+filename<<std::endl;
  }
}



// Prints one single alm table to a TEXT file.
void GeneralOutput(const Alm<xcomplex <double> > & a, const ParameterList & config, std::string keyword) {
  std::string filename;
  std::ofstream outfile; 
  int lminout, lmaxout, mmax, l, m;

  // If requested, write alm's to file:
  if (config.reads(keyword)!="0") {
    filename = config.reads(keyword);
    outfile.open(filename.c_str());
    if (!outfile.is_open()) warning("GeneralOutput: cannot open "+filename+" file.");
    outfile << "# l, m, Re(alm), Im(alm)"<<std::endl<<std::endl;
    lminout = config.readi("LRANGE_OUT", 0);
    lmaxout = config.readi("LRANGE_OUT", 1);
    mmax = config.readi("MMAX_OUT");
    if (mmax>lminout) error ("GeneralOutput: current code only allows MMAX_OUT <= LMIN_OUT.");
    // Output all alm's:
    if (mmax<0) {
      for(l=lminout; l<=lmaxout; l++)
	for(m=0; m<=l; m++) {
	  outfile << l <<" "<< m;
	  outfile <<" "<<std::setprecision(10)<< a(l,m).re<<" "<<std::setprecision(10)<< a(l,m).im;
	  outfile<<std::endl;
	} 
    }
    // Truncate m in alm output:
    else {
     for(l=lminout; l<=lmaxout; l++)
	for(m=0; m<=mmax; m++) {
	  outfile << l <<" "<< m;
	  outfile <<" "<<std::setprecision(10)<< a(l,m).re<<" "<<std::setprecision(10)<< a(l,m).im;
	  outfile<<std::endl;
	}  
    }
    outfile.close();
    std::cout << ">> "+keyword+" written to "+filename<<std::endl;
  }
}



/********************/
/*** Maps output  ***/
/********************/


// Prints a list of maps to a many FITS files:
void GeneralOutput(Healpix_Map<double> *mapf, const ParameterList & config, std::string keyword, int **fnz, int Nfields) {
  std::string filename, tempstr;
  char message[100], message2[100], *arg[4];
  char opt1[]="-bar";
  int i;
  
  if (config.reads(keyword)!="0") {
    // Write to FITS:
    tempstr  = config.reads(keyword);
    for (i=0; i<Nfields; i++) {
      sprintf(message, "%sf%dz%d.fits", tempstr.c_str(), fnz[i][0], fnz[i][1]);
      filename.assign(message);
      sprintf(message2, "rm -f %s", message);
      system(message2); // Have to delete previous fits files first.
      write_Healpix_map_to_fits(filename,mapf[i],planckType<double>());
      std::cout << ">> "<<keyword<< "["<<i<<"] written to "<<filename<<std::endl;
      // Write to TGA if requested:
      if (config.readi("FITS2TGA")==1 || config.readi("FITS2TGA")==2) {
	sprintf(message2, "%sf%dz%d.tga", tempstr.c_str(), fnz[i][0], fnz[i][1]);
	arg[1]=message; arg[2]=message2; arg[3]=opt1;
	map2tga_module(4, (const char **)arg);
	std::cout << ">> "<<keyword<< "["<<i<<"] written to "<<message2<<std::endl;
	if (config.readi("FITS2TGA")==2) {
	  sprintf(message2, "rm -f %s", message);
	  system(message2);
	  std::cout << "-- "<<filename<<" file removed."<<std::endl;
	}
      }
    }
  }
}


// Prints two lists of maps to a single TEXT file.
void GeneralOutput(Healpix_Map<double> *gamma1, Healpix_Map<double> *gamma2, 
		   const ParameterList & config, std::string keyword, int **fnz, int N1, int N2) {
  std::string filename;
  std::ofstream outfile;
  int i, j, Nfields, field, z, npixels, *nside, n;
  pointing coord;

  if (config.reads(keyword)!="0") {
    Nfields=N1*N2; 
    
    // Check which maps are allocated and avoid different-size maps.
    j=0;
    nside = vector<int>(0,Nfields-1);
    for (i=0; i<Nfields; i++) {
      nside[i]  = gamma1[i].Nside(); // Record all maps Nsides.
      if (nside[i]!=gamma2[i].Nside()) error ("GeneralOutput: expecting pair of maps with same number of pixels.");
      if (nside[i]!=0) {             // Look for allocated maps.
	if (j==0) {                  // Initialize j to first non-zero Nside.
	  j=nside[i]; n=i;
	}
	else if (nside[i]!=j) error ("GeneralOutput: maps do not have the same number of pixels.");
      }
    }
    npixels=12*j*j;
    
    // Output to file:
    filename=config.reads(keyword);
    outfile.open(filename.c_str());
    if (!outfile.is_open()) warning("GeneralOutput: cannot open file "+filename);
    else {
      // Set Headers:
      outfile << "# theta, phi";
      for (i=0; i<Nfields; i++) if (nside[i]!=0) { 
	  n2fz(i, &field, &z, N1, N2);
	  outfile <<", f"<<field<<"z"<<z<<"[1]"<<", f"<<field<<"z"<<z<<"[2]";
	}
      outfile << std::endl;
      // LOOP over allocated fields:
      for (j=0; j<npixels; j++) {
	coord = gamma1[n].pix2ang(j);
	outfile << coord.theta <<" "<< coord.phi;
	for (i=0; i<Nfields; i++) if (nside[i]!=0) outfile <<" "<< gamma1[i][j]<<" "<< gamma2[i][j];
	outfile << std::endl;
      }
    }
    outfile.close();
    std::cout << ">> "<<keyword<<" written to "<<filename<<std::endl;
    free_vector(nside, 0,Nfields-1);
  } 
}


// Prints a list of maps to a single TEXT file.
void GeneralOutput(Healpix_Map<double> *mapf, const ParameterList & config, std::string keyword, int **fnz, int N1, int N2) {
  std::string filename;
  std::ofstream outfile;
  int i, j, Nfields, field, z, npixels, *nside, n;
  pointing coord;

  if (config.reads(keyword)!="0") {
    Nfields=N1*N2; 
    
    // Check which maps are allocated and avoid different-size maps.
    j=0;
    nside = vector<int>(0,Nfields-1);
    for (i=0; i<Nfields; i++) {
      nside[i] = mapf[i].Nside(); // Record all maps Nsides.
      if (nside[i]!=0) {          // Look for allocated maps.
	if (j==0) {               // Initialize j to first non-zero Nside.
	  j=nside[i]; n=i;
	}
	else if (nside[i]!=j) error ("GeneralOutput: maps do not have the same number of pixels.");
      }
    }
    npixels=12*j*j;
    
    // Output to file:
    filename=config.reads(keyword);
    outfile.open(filename.c_str());
    if (!outfile.is_open()) warning("GeneralOutput: cannot open file "+filename);
    else {
      // Set Headers:
      outfile << "# theta, phi";
      for (i=0; i<Nfields; i++) if (nside[i]!=0) { 
	  n2fz(i, &field, &z, N1, N2);
	  outfile << ", f"<<field<<"z"<<z;
	}
      outfile << std::endl;
      // LOOP over allocated fields:
      for (j=0; j<npixels; j++) {
	coord = mapf[n].pix2ang(j);
	outfile << coord.theta <<" "<< coord.phi;
	for (i=0; i<Nfields; i++) if (nside[i]!=0) outfile <<" "<< mapf[i][j];
	outfile << std::endl;
      }
    }
    outfile.close();
    std::cout << ">> "<<keyword<<" written to "<<filename<<std::endl;
    free_vector(nside, 0,Nfields-1);
  } 
}


// Prints one single map to FITS file based on a PREFIX and a FIELD ID:
void GeneralOutput(const Healpix_Map<double> & map, const ParameterList & config, std::string keyword, int *fnz) {
  std::string filename, tgafile;
  char *arg[4];
  char message1[100], message2[100];
  char opt1[]="-bar";
  if (config.reads(keyword)!="0") {
    sprintf(message1, "%sf%dz%d.fits", config.reads(keyword).c_str(), fnz[0], fnz[1]);
    filename.assign(message1);

    // Write to FITS:
    sprintf(message1, "rm -f %s", filename.c_str());
    system(message1); // Have to delete previous fits files first.
    write_Healpix_map_to_fits(filename, map, planckType<double>());
    std::cout << ">> "<<keyword<<" written to "<<filename<<std::endl;
    // Write to TGA if requested:
    if (config.readi("FITS2TGA")==1 || config.readi("FITS2TGA")==2) {
      tgafile = filename;
      tgafile.replace(tgafile.find(".fits"),5,".tga");
      sprintf(message1, "%s", filename.c_str());
      sprintf(message2, "%s", tgafile.c_str());
      arg[1]=message1; arg[2]=message2; arg[3]=opt1;
      map2tga_module(4, (const char **)arg);
      std::cout << ">> "<<keyword<<" written to "<<tgafile<<std::endl;
      if (config.readi("FITS2TGA")==2) {
	sprintf(message2, "rm -f %s", message1);
	system(message2);
	std::cout << "-- "<<filename<<" file removed."<<std::endl;
      }
    }
  } 
}


// Prints one single combination of kappa, gamma1 and gamma2 maps to FITS file based on a PREFIX and a FIELD ID:
void GeneralOutput(const Healpix_Map<double> & kmap, const Healpix_Map<double> & g1map, 
		   const Healpix_Map<double> & g2map, const ParameterList & config, std::string keyword, int *fnz) {
  std::string filename, tgafile;
  char *arg[4];
  char message1[100], message2[100];
  char opt1[]="-bar";
  if (config.reads(keyword)!="0") {
    sprintf(message1, "%sf%dz%d.fits", config.reads(keyword).c_str(), fnz[0], fnz[1]);
    filename.assign(message1);

    // Write to FITS:
    sprintf(message1, "rm -f %s", filename.c_str());
    system(message1); // Have to delete previous fits files first.
    write_Healpix_map_to_fits(filename, kmap, g1map, g2map, planckType<double>());
    std::cout << ">> "<<keyword<<" written to "<<filename<<std::endl;
    // Write to TGA if requested:
    if (config.readi("FITS2TGA")==1 || config.readi("FITS2TGA")==2) {
      tgafile = filename;
      tgafile.replace(tgafile.find(".fits"),5,".tga");
      sprintf(message1, "%s", filename.c_str());
      sprintf(message2, "%s", tgafile.c_str());
      arg[1]=message1; arg[2]=message2; arg[3]=opt1;
      map2tga_module(4, (const char **)arg);
      std::cout << ">> "<<keyword<<" written to "<<tgafile<<std::endl;
      if (config.readi("FITS2TGA")==2) {
	sprintf(message2, "rm -f %s", message1);
	system(message2);
	std::cout << "-- "<<filename<<" file removed."<<std::endl;
      }
    }
  } 
}


// Prints one single combination of kappa, gamma1 and gamma2 maps to FITS file.
void GeneralOutput(const Healpix_Map<double> & kmap, const Healpix_Map<double> & g1map, 
		   const Healpix_Map<double> & g2map, const ParameterList & config, std::string keyword) {
  std::string filename, tgafile;
  char *arg[4];
  char message1[100], message2[100];
  char opt1[]="-bar";
  if (config.reads(keyword)!="0") {
    // Write to FITS:
    filename=config.reads(keyword);
    sprintf(message1, "rm -f %s", filename.c_str());
    system(message1); // Have to delete previous fits files first.
    write_Healpix_map_to_fits(filename, kmap, g1map, g2map, planckType<double>());
    std::cout << ">> "<<keyword<<" written to "<<filename<<std::endl;
    // Write to TGA if requested:
    if (config.readi("FITS2TGA")==1 || config.readi("FITS2TGA")==2) {
      tgafile = filename;
      tgafile.replace(tgafile.find(".fits"),5,".tga");
      sprintf(message1, "%s", filename.c_str());
      sprintf(message2, "%s", tgafile.c_str());
      arg[1]=message1; arg[2]=message2; arg[3]=opt1;
      map2tga_module(4, (const char **)arg);
      std::cout << ">> "<<keyword<<" written to "<<tgafile<<std::endl;
      if (config.readi("FITS2TGA")==2) {
	sprintf(message2, "rm -f %s", message1);
	system(message2);
	std::cout << "-- "<<keyword<<" FITS file removed."<<std::endl;
      }
    }
  } 
}


// Prints one single map to FITS and/or TGA file.
void GeneralOutput(const Healpix_Map<double> & map, const ParameterList & config, std::string keyword) {
  std::string filename, tgafile;
  char *arg[4];
  char message1[100], message2[100];
  char opt1[]="-bar";
  if (config.reads(keyword)!="0") {
    // Write to FITS:
    filename=config.reads(keyword);
    sprintf(message1, "rm -f %s", filename.c_str());
    system(message1); // Have to delete previous fits files first.
    write_Healpix_map_to_fits(filename, map, planckType<double>());
    std::cout << ">> "<<keyword<<" written to "<<filename<<std::endl;
    // Write to TGA if requested:
    if (config.readi("FITS2TGA")==1 || config.readi("FITS2TGA")==2) {
      tgafile = filename;
      tgafile.replace(tgafile.find(".fits"),5,".tga");
      sprintf(message1, "%s", filename.c_str());
      sprintf(message2, "%s", tgafile.c_str());
      arg[1]=message1; arg[2]=message2; arg[3]=opt1;
      map2tga_module(4, (const char **)arg);
      std::cout << ">> "<<keyword<<" written to "<<tgafile<<std::endl;
      if (config.readi("FITS2TGA")==2) {
	sprintf(message2, "rm -f %s", message1);
	system(message2);
	std::cout << "-- "<<keyword<<" FITS file removed."<<std::endl;
      }
    }
  } 
}
