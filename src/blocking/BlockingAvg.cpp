/////////////////////////////////////
///     \file blockingAvg.cpp
///     \author Marielle Pinheiro
///     \version March 26, 2015


/*This code fills an array with 31 days (31*nsteps) of PV
data and adds the sum of this array to the index of a 365-day
array that corresponds to the center date of the 31-day array.
The value for each day is then divided by the number of years *31*
nsteps to get the daily average for the (n)-year period. 
*/
#include "BlockingUtilities.h"
#include "CommandLine.h"
#include "Exception.h"
#include "Announce.h"
#include "TimeObj.h"

#include "DataVector.h"
#include "DataMatrix.h"

#include "netcdfcpp.h"
#include "NetCDFUtilities.h"

#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <cstdio>
#include <iostream>
#include <sstream>
int main(int argc, char **argv){
  NcError error(NcError::verbose_nonfatal);

  try{
    std::string fileList;
    std::string strfile_out;
    std::string varName;
    std::string avgName;
    std::string tname,latname,lonname;
    bool missingFiles;

    BeginCommandLine()
      CommandLineString(fileList, "inlist", "");
      CommandLineString(strfile_out, "out", "");
      CommandLineString(varName, "varname","");
      CommandLineString(avgName, "avgname","");
      CommandLineBool(missingFiles, "missing");
      CommandLineString(tname,"tname","time");
      CommandLineString(latname,"latname","lat");
      CommandLineString(lonname,"lonname","lon");
      ParseCommandLine(argc, argv);


    EndCommandLine(argv)
    AnnounceBanner();
    if (fileList == ""){
      _EXCEPTIONT("No file list (--inlist) provided");
    }
    if (strfile_out == ""){
      _EXCEPTIONT("No output file name (--out) provided");
    }
    if (varName == ""){
      _EXCEPTIONT("No variable name (--varname) specified");
    }
    if (avgName == ""){
      _EXCEPTIONT("No average name (--avgname) specified");
    }
    if (fileList == ""){
      _EXCEPTIONT("No file list (--inlist) specified");
    }

    //Create list of input files
    std::vector<std::string> InputFiles;
    GetInputFileList(fileList, InputFiles);
    int nFiles = InputFiles.size();

    //Open first file 
    NcFile infile(InputFiles[0].c_str());
    int nTime = infile.get_dim(tname.c_str())->size();
    int nLat = infile.get_dim(latname.c_str())->size();
    int nLon = infile.get_dim(lonname.c_str())->size();

    //IPV variable and data matrix
    NcVar *inPV = infile.get_var(varName.c_str());
    DataMatrix3D<double> IPVData(nTime, nLat, nLon);
    inPV->set_cur(0,0,0);
    inPV->get(&(IPVData[0][0][0]),nTime,nLat,nLon);

    //time variable
    NcVar *timeVal = infile.get_var(tname.c_str());
    DataVector<double> timeVec(nTime);
    timeVal->set_cur((long) 0);
    timeVal->get(&(timeVec[0]),nTime);

    //Time and Calendar attributes
    NcAtt *attTime = timeVal->get_att("units");
    if (attTime == NULL){
      _EXCEPTIONT("Time variable has no units attribute.");
    }
    std::string strTimeUnits = attTime->as_string(0);

    NcAtt *attCal = timeVal->get_att("calendar");
    if(attCal==NULL){
      _EXCEPTIONT("Time variable has no calendar attribute.");
    }
    std::string strCalendar = attCal->as_string(0);
    if (strncmp(strCalendar.c_str(), "gregorian",9)==0){
      strCalendar = "standard";
    }

    //Resolution of the time axis
    double tRes;
    if ((strTimeUnits.length() >= 11) && \
      (strncmp(strTimeUnits.c_str(), "days since ", 11) == 0)){
      tRes = timeVec[1]-timeVec[0];
    }
    else {
      tRes = (timeVec[1]-timeVec[0])/24.0;
    }

    //Length of 31 days axis
    int arrLen = int(31.0/tRes);


    //Start date of first file
    //Parse time units of first file to determine start date 
    int yearLen = 365;
    int dateYear=0;
    int dateMonth=0;
    int dateDay=0;
    int dateHour=0;
    ParseTimeDouble(strTimeUnits, strCalendar, timeVec[0], dateYear,\
      dateMonth, dateDay, dateHour);

    int day = DayInYear(dateMonth,dateDay);
    int dateIndex = day + 15;

    bool leap = checkFileLeap(strTimeUnits, strCalendar, dateYear, \
      dateMonth, dateDay, dateHour, timeVec[nTime-1]);

    //3D matrix to store averaged values
    DataMatrix3D<double> avgStoreVals(yearLen,nLat,nLon);
    DataMatrix3D<double> avgCounts(yearLen,nLat,nLon);

    //Number of time steps per day
    int nSteps = 1/tRes;
    //std::cout<<"tRes is "<<tRes<<" and nSteps is "<<nSteps<<std::endl;
    double endTime = timeVec[nTime-1];
    int currArrIndex = 0;

    //3D 31-day array
    DataMatrix3D<double> currFillData(arrLen,nLat,nLon);

    //File counter
    int x=0;
    //Start and end for time dimension
    int tStart = 0;
    int tEnd;
    if (nTime>arrLen){
      tEnd = arrLen;
    }
    else{
      tEnd = nTime;
    }

    int leapYear=0;
    int leapMonth=0;
    int leapDay=0;
    int leapHour=0;


    //Values in case of missing files
    double contCheck = 0.;
    double missingValue=-999999.9;
    bool hasMissingValues = false;
    

    //First while loop: open files and fill until 31 days array full
    while (currArrIndex<arrLen){
      //First file
      for (int t=tStart; t<tEnd; t++){
        if (leap==true){
        //Check if time is a leap year date
          ParseTimeDouble(strTimeUnits, strCalendar, timeVec[t], leapYear,\
            leapMonth,leapDay,leapHour);
          if (leapMonth==2 && leapDay == 29){
            while (leapMonth ==2 && leapDay ==29){
            //  std::cout<<"Leap day! Skipping this step."<<std::endl;
              t++;
              ParseTimeDouble(strTimeUnits, strCalendar, timeVec[t], leapYear,\
                leapMonth, leapDay, leapHour);
            }
          }
        }
        //Fill the 31 day array with PV data
        for (int a=0; a<nLat; a++){
          for (int b=0; b<nLon; b++){
            currFillData[currArrIndex][a][b] = IPVData[t][a][b];
          }
        }
        currArrIndex++;
       // std::cout<<"current array index is "<<currArrIndex<<std::endl;
      }
      //If the 31 days aren't yet filled, keep going
      if (currArrIndex<arrLen){
        infile.close();
        //Open new file and increment file counter
        x+=1;
        NcFile infile(InputFiles[x].c_str());
        nTime = infile.get_dim(tname.c_str())->size();
        nLat = infile.get_dim(latname.c_str())->size();
        nLon = infile.get_dim(lonname.c_str())->size();

        //IPV variable
        NcVar *inPV = infile.get_var(varName.c_str());
        IPVData.Initialize(nTime,nLat,nLon);

        int nDims = inPV->num_dims();
        if (nDims != 3){
          _EXCEPTIONT("Variable does not have the correct number of dimensions (3).");
        } 

        inPV->set_cur(0,0,0);
        inPV->get(&(IPVData[0][0][0]),nTime,nLat,nLon);

        //Time variable
        NcVar *timeVal = infile.get_var(tname.c_str());
        timeVec.Initialize(nTime);
        timeVal->set_cur((long) 0);
        timeVal->get(&(timeVec[0]),nTime);

        contCheck = tBetweenFiles(strTimeUnits, timeVec[0], endTime);
        
        //If missing hasn't been specified:

        if (contCheck>tRes){
          if (!missingFiles){
          //check to make sure that there isn't a file missing in the list!
          //otherwise will mess up averages
           // std::cout << "contCheck is "<< contCheck << std::endl;
            _EXCEPTIONT("New file is not continuous with previous file."); 
          } else{
          //Fill in the days that are missing with the missingvalue 
          //contCheck units are in days, tRes is a fraction of a day
          //So the number of indices to be filled in is contCheck/tRes     
            MissingFill(missingValue,tRes,contCheck,nLat,nLon,arrLen,\
              currArrIndex,dateIndex,currFillData); 
          }
          
        }

        //reset leap year check
        leap = checkFileLeap(strTimeUnits, strCalendar, dateYear, \
          dateMonth, dateDay, dateHour,timeVec[nTime-1]);

        //reset ending time of current file for next continuity check
        endTime = timeVec[nTime-1];

        //check that tEnd doesn't exceed 31 days       
        int tCheck = currArrIndex + nTime;

        if (tCheck > arrLen){
          tEnd = arrLen-currArrIndex;
        }
        else{
          tEnd = nTime;
        }
      }     
    }
    //Fill yearly array with sum of 31 days
    //Check if the array contains missing values
    if (missingFiles){
      hasMissingValues = missingValCheck(currFillData,arrLen,missingValue);
    }    

    if (hasMissingValues){
      //std::cout<< "This array has missing values, will not be added to sum."<<std::endl;
    }
    else{
      //std::cout<<"No missing values found. Adding array to sum."<<std::endl;
      for (int t=0; t<arrLen; t++){
        for (int a=0; a<nLat; a++){
          for (int b=0; b<nLon; b++){
            avgStoreVals[dateIndex][a][b] += currFillData[t][a][b];
          }
        }
      }
   //increase count by 1
      for (int a=0; a<nLat; a++){
        for (int b=0; b<nLon; b++){
          avgCounts[dateIndex][a][b] +=1.0;
        }
      }

    }
   
    //std::cout<<"Filled date index number "<<dateIndex<<std::endl; 
    dateIndex+=1;
    currArrIndex = 0;
  //Check if new file needs to be opened before entering while loop

    if(tEnd>=nTime){
      infile.close();
      //std::cout<<"First while loop. Closed "<<InputFiles[x]<<std::endl;
      x+=1;
      NcFile infile(InputFiles[x].c_str());
      nTime = infile.get_dim(tname.c_str())->size();
      nLat = infile.get_dim(latname.c_str())->size();
      nLon = infile.get_dim(lonname.c_str())->size();

      //IPV variable
      NcVar *inPV = infile.get_var(varName.c_str());
      IPVData.Initialize(nTime,nLat,nLon);

      inPV->set_cur(0,0,0);
      inPV->get(&(IPVData[0][0][0]),nTime,nLat,nLon);

      //Time variable
      NcVar *timeVal = infile.get_var(tname.c_str());
      timeVec.Initialize(nTime);

      timeVal->set_cur((long) 0);
      timeVal->get(&(timeVec[0]),nTime);

      leap = checkFileLeap(strTimeUnits, strCalendar, dateYear, \
        dateMonth, dateDay, dateHour, timeVec[nTime-1]);
   
      tStart = 0;
      tEnd = tStart + nSteps;
    }

    else if (tEnd<nTime){
      //std::cout<<"First while loop. Still have data left on current file. Will continue on."<<std::endl;
      tStart = tEnd;
      tEnd = tStart + nSteps;
    }

    //entering while loop.

    bool newFile = false;

    while (x<nFiles){
      //std::cout<<"7: Inside while loop: file currently "<<InputFiles[x]<<" and nTime is "\
        <<nTime<<"; tStart/end is "<<tStart<<" and "<<tEnd<<std::endl;
      //std::cout<<"Current date index is "<<dateIndex<<std::endl;
       //1: check if current day is a leap day
      if (leap==true){
      //  std::cout<<"Checking date for leap day."<<std::endl;
        ParseTimeDouble(strTimeUnits, strCalendar, timeVec[tStart], leapYear,\
          leapMonth, leapDay, leapHour);
        if (leapMonth==2 && leapDay ==29){
          tStart = tEnd;
          tEnd = tStart + nSteps;
          //std::cout<<"#1: This day is a leap day. Resetting tStart/tEnd to "\
            <<tStart<<" and "<<tEnd<<std::endl;
        } 
        if (tEnd>nTime){
          newFile = true;
          //std::cout<<"For leap day file, reached EOF."<<std::endl;
        }
      }
      //2: if new file needs to be opened, open it
      if(newFile==true){
        endTime = timeVec[nTime-1];
        //cases with incomplete days
        if (nTime-tStart < nSteps) {
          double extraSteps = nSteps - (nTime-tStart);
        //  std::cout<<"Need to add "<<extraSteps<<" extra steps."<<std::endl;
          endTime += (extraSteps * tRes);
        //  std::cout<<"New end time is "<<endTime<<std::endl;
        }
      //  std::cout<<"#2: Reached end of file."<<std::endl;
        infile.close();
        std::cout<<"Closed "<<InputFiles[x]<<std::endl;
        x+=1;
        if (x<nFiles){
          NcFile infile(InputFiles[x].c_str());
          newFile = false;
          //std::cout<<"x is currently "<<x<<", opening file "<<InputFiles[x]<<std::endl;
          nTime = infile.get_dim(tname.c_str())->size();
          nLat = infile.get_dim(latname.c_str())->size();
          nLon = infile.get_dim(lonname.c_str())->size();

        //IPV variable
          NcVar *inPV = infile.get_var(varName.c_str());
          IPVData.Initialize(nTime, nLat, nLon);

          inPV->set_cur(0,0,0);
          inPV->get(&(IPVData[0][0][0]),nTime,nLat,nLon);

        //Time variable
          NcVar *timeVal = infile.get_var(tname.c_str());
          timeVec.Initialize(nTime);
          timeVal->set_cur((long) 0);
          timeVal->get(&(timeVec[0]),nTime);

          ParseTimeDouble(strTimeUnits, strCalendar, timeVec[0],dateYear,\
            dateMonth,dateDay,dateHour);

         //Check file continuity
          contCheck = tBetweenFiles(strTimeUnits, timeVec[0],endTime);

          if (contCheck > tRes){
            if (!missingFiles){
            //  std::cout<<"contCheck is "<<contCheck<<std::endl;
              _EXCEPTIONT("New file is not continuous with previous file");
            }else{
              MissingFill(missingValue, tRes, contCheck, nLat, nLon, arrLen, \
                currArrIndex,dateIndex, currFillData);
            }
          }           

          leap = checkFileLeap(strTimeUnits, strCalendar, dateYear,\
            dateMonth, dateDay, dateHour, timeVec[nTime-1]);

          tStart = 0;
          tEnd = tStart + nSteps;
        }
      }    
      //3: fill array for next value
      if (newFile==false){
      //  std::cout<<"At time "<<tStart<<" to "<<tEnd<< "Filled in values at array index "<<currArrIndex<<" that will fill date "<<dateIndex<<std::endl;
       
      //  if (nTime-tStart < nSteps) {
        //  std::cout<< "Incomplete day. Skipping."<<std::endl;
      //  }
 
        for (int t=tStart; t<tEnd; t++){
          for (int a=0; a<nLat; a++){
            for (int b=0; b<nLon; b++){
              if (nTime-tStart < nSteps){
                currFillData[currArrIndex][a][b] = missingValue;
              }
              else{
                if (std::fabs(IPVData[t][a][b]) > 10e10){
                  std::cout<<"WARNING: File "<<InputFiles[x]<<" has suspicious value! "<<IPVData[t][a][b]<<std::endl;
                }
                currFillData[currArrIndex][a][b] = IPVData[t][a][b];
              }
            }
          }
          currArrIndex+=1;
        //  std::cout<< "Array index is now "<<currArrIndex<<" (arrlen is "<<arrLen<<")"<<std::endl;
        }
        //check for periodic boundary condition for 31 day array
        if (currArrIndex>=arrLen){
       //   std::cout<<"currArrIndex is "<<currArrIndex<<" and periodic boundary: 31 day length met or exceeded."<<std::endl;
          currArrIndex-=arrLen;
        }
     //Check to see if array contains missing values
     if (missingFiles){
       hasMissingValues = missingValCheck(currFillData,arrLen,missingValue);
     }
     //Do not add sum of array to average if array contains missing values!
     if (hasMissingValues){
       //std::cout<<"This array has missing values, will not be added to sum."<<std::endl;
     }else{
       //std::cout<<"Filled in values at date index "<<dateIndex<<std::endl;
       //Fill date with sum of new array values
          for (int t=0; t<arrLen; t++){
            for (int a=0; a<nLat; a++){
              for (int b=0; b<nLon; b++){
                avgStoreVals[dateIndex][a][b] += currFillData[t][a][b];
              }
            }
          }
        //increase count
          for (int a=0; a<nLat; a++){
            for (int b=0; b<nLon; b++){
              avgCounts[dateIndex][a][b] += 1.0;
            }
          }
        }
        //regardless of missing data or not, proceed to next date
        dateIndex+=1;
        //periodic boundary condition for year
        if(dateIndex>=yearLen){
          dateIndex-=yearLen;
        }
        if (tEnd >= nTime){
       //   std::cout<<"Current tEnd is "<<tEnd << " and nTime is "<<nTime<<std::endl;
          newFile = true;
       //   std::cout<<"Else: Reached EOF. Will open new file in next iteration."<<std::endl;

        }
        else{
          tStart = tEnd;
          tEnd = tStart + nSteps;
        //  std::cout<<"Filled array. Resetting tstart/tend to "<<tStart<< " and "<<tEnd<<std::endl;
          //Check for leap year
          if (leap==true){
          //  std::cout<<"Checking date for leap day."<<std::endl;
            ParseTimeDouble(strTimeUnits, strCalendar, timeVec[tStart], leapYear,\
              leapMonth, leapDay, leapHour);
            if (leapMonth==2 && leapDay ==29){
              tStart = tEnd;
              tEnd = tStart + nSteps;
              //std::cout<<"This day is a leap day. Resetting tStart/tEnd to "\
                <<tStart<<" and "<<tEnd<<std::endl;
            }
            //re-check tEnd
            if (tEnd>=nTime){
              newFile = true;
            } 
          }
        }
    //    else{
    //      newFile = true;
    //      std::cout<<"tEnd is "<<tEnd<<", nTime is "<<nTime<<", tStart is "<<tStart<<std::endl;
   //       std::cout<<"Else: Reached EOF. Will open new file in next iteration."<<std::endl;
     //   }
      }
    }
    
    //average all values
    for (int t=0; t<yearLen; t++){
      for (int a=0; a<nLat; a++){
        for (int b=0; b<nLon; b++){
          avgStoreVals[t][a][b] = avgStoreVals[t][a][b]/(avgCounts[t][a][b]*31.0*nSteps);
        }
      }
    }
    std::cout<<"Averaged values"<<std::endl;

    //Get existing file info to copy to output file

    NcFile refFile(InputFiles[0].c_str());

    NcDim *inLatDim = refFile.get_dim(latname.c_str());
    NcDim *inLonDim = refFile.get_dim(lonname.c_str());
    NcVar *inLatVar = refFile.get_var(latname.c_str());
    NcVar *inLonVar = refFile.get_var(lonname.c_str());


  //Output to file
    NcFile outfile(strfile_out.c_str(), NcFile::Replace, NULL, 0, NcFile::Offset64Bits);

    NcDim *outTime = outfile.add_dim(tname.c_str(), yearLen);
    DataVector<int> timeVals(yearLen);
    for (int t=0; t<yearLen; t++){
      timeVals[t] = t+1;
    }

    NcVar *outTimeVar = outfile.add_var(tname.c_str(), ncInt, outTime);
    outTimeVar->set_cur((long) 0);
    outTimeVar->put(&(timeVals[0]),yearLen);

    //Add time units (necessary for time conversion)
    outTimeVar->add_att("units","days since 0001-01-01"); 

    NcDim *outLat = outfile.add_dim(latname.c_str(), nLat);
    NcDim *outLon = outfile.add_dim(lonname.c_str(), nLon);
    NcVar *outLatVar = outfile.add_var(latname.c_str(), ncDouble, outLat);
    NcVar *outLonVar = outfile.add_var(lonname.c_str(), ncDouble, outLon);

    copy_dim_var(inLatVar,outLatVar);
    copy_dim_var(inLonVar,outLonVar);

    NcVar *outAvgIPV = outfile.add_var(avgName.c_str(), ncDouble, outTime, outLat, outLon);
    outAvgIPV->set_cur(0,0,0);
    outAvgIPV->put(&(avgStoreVals[0][0][0]),yearLen,nLat,nLon);

    refFile.close();
    outfile.close();
  }

  catch (Exception & e){
    std::cout<<e.ToString() <<std::endl;
  }
//  infile.close();
//  outfile.close();

  
}

