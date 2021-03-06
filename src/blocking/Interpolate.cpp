//////////////////////////////////
///
///    \file interpolate.cpp
///    \author Marielle Pinheiro
///    \version March 24, 2015

#include "Interpolate.h"
#include "BlockingUtilities.h"
#include "NetCDFUtilities.h"
#include "netcdfcpp.h"
#include "DataVector.h"
#include "DataMatrix3D.h"
#include "DataMatrix4D.h"
#include <cstdlib>
#include <cmath>
#include <cstring>

//////////////////////////////////////////////////////////////////////// 

void interp_util(NcFile & readin,
                 const std::string & strname_2d, 
                 const std::string & varlist,
                 NcFile & ifile_out) {

  //open 2D PS file
  NcFile readin_2d(strname_2d.c_str());
  if (!readin_2d.is_valid()) {
    _EXCEPTION1("Unable to open file \"%s\" for reading",
      strname_2d.c_str());
  }

  //Dimensions and associated variables
  NcDim *time = readin.get_dim("time");
  int time_len = time->size();
  NcVar *timevar = readin.get_var("time");

  NcDim *lev = readin.get_dim("lev");
  int lev_len = lev->size();

  NcDim *lat = readin.get_dim("lat");
  int lat_len = lat->size();
  NcVar *latvar = readin.get_var("lat");

  NcDim *lon = readin.get_dim("lon");
  int lon_len = lon->size();
  NcVar *lonvar = readin.get_var("lon");

  //Variables
/*  NcVar *temp = readin.get_var("T");
  NcVar *uvar = readin.get_var("U");
  NcVar *vvar = readin.get_var("V");
*/
  
  //2D variables
  NcVar *ps = readin_2d.get_var("PS");
  NcVar *hyam = readin_2d.get_var("hyam");
  NcVar *hybm = readin_2d.get_var("hybm");

  //Create pressure level vector
  int plev_len = (100000.0-5000.0)/(5000.0);

  DataVector<double> pVals(plev_len);

  for (int i=0; i<plev_len; i++){
    double pNum = 5000.0 * (i+1);
    pVals[i] = pNum;
  }
  
    //Write information to outfile
  NcDim *itime = ifile_out.add_dim("time", time_len);
  NcDim *ilev = ifile_out.add_dim("lev", plev_len);
  NcDim *ilat = ifile_out.add_dim("lat", lat_len);
  NcDim *ilon = ifile_out.add_dim("lon", lon_len);
    
  NcVar *itime_vals = ifile_out.add_var("time", ncDouble, itime);
  NcVar *ilat_vals = ifile_out.add_var("lat", ncDouble, ilat);
  NcVar *ilon_vals = ifile_out.add_var("lon", ncDouble, ilon);
  NcVar *ilev_vals = ifile_out.add_var("lev", ncDouble, ilev);

  copy_dim_var(timevar, itime_vals);
  copy_dim_var(latvar, ilat_vals);
  copy_dim_var(lonvar, ilon_vals);

  std::cout<<"writing new pressure level."<<std::endl;
    //Give pressure dimension values
  ilev_vals ->set_cur((long) 0);
  ilev_vals->put(&(pVals[0]), plev_len);

  //Split var list  
  std::string delim = ",";
  size_t pos = 0;
  std::string token;
  std::vector<std::string> varVec;
  std::string listcopy = varlist;

  while ((pos = listcopy.find(delim)) != std::string::npos){
    token = listcopy.substr(0,pos);
    varVec.push_back(token);
    listcopy.erase(0,pos + delim.length());
  }
  varVec.push_back(listcopy);

/*
  //reads the string (separated by the delimiter) into the vector
  while((pos = varlist.find(delim)) != std::string::npos){
//    token = varlist.substr(0,pos);
    varVec.push_back(token);
    varlist.erase(0,pos + delim.length());
//    varlist.erase((std::string::size_type)0,(std::string::size_type)pos + delim.length());

  }
  varVec.push_back(varlist);
*/

  for (int v=0; v<varVec.size(); v++){
    std::cout<<"Vector contains string "<<varVec[v].c_str()<<std::endl;
  }

  for (int v=0; v<varVec.size(); v++){
    NcVar * oldvar = readin.get_var(varVec[v].c_str());
    NcVar * newvar = ifile_out.add_var(varVec[v].c_str(), ncDouble,itime,ilev,ilat,ilon);
    interpolate_lev(oldvar,hyam,hybm,ps,ilev_vals,newvar);
  }

  //std::cout<<"Within interpolation file: about to call interpolate_lev"<<std::endl;
  //Add interpolated variables to interpolated outfile
/*  NcVar *itemp = ifile_out.add_var("T", ncDouble, itime, ilev, ilat, ilon);
  interpolate_lev(temp, hyam, hybm, ps, ilev_vals, itemp);

  NcVar *iu = ifile_out.add_var("U", ncDouble, itime, ilev, ilat, ilon);
  interpolate_lev(uvar, hyam, hybm, ps, ilev_vals, iu);

  NcVar *iv = ifile_out.add_var("V", ncDouble, itime, ilev, ilat, ilon);
  interpolate_lev(vvar, hyam, hybm, ps, ilev_vals, iv); 
*/
  readin_2d.close();  
  std::cout<<"Finished interpolating variables."<<std::endl;
} 
