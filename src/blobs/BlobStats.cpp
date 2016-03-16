///////////////////////////////////////////////////////////////////////////////
///
///	\file    StitchNodes.cpp
///	\author  Paul Ullrich
///	\version October 1st, 2014
///
///	<remarks>
///		Copyright 2000-2014 Paul Ullrich
///
///		This file is distributed as part of the Tempest source code package.
///		Permission is granted to use, copy, modify and distribute this
///		source code and its documentation under the terms of the GNU General
///		Public License.  This software is provided "as is" without express
///		or implied warranty.
///	</remarks>

#include "BlobUtilities.h"

#include "CommandLine.h"
#include "Exception.h"
#include "Announce.h"

#include "DataVector.h"
#include "DataMatrix.h"

#include "netcdfcpp.h"
#include "NetCDFUtilities.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <vector>
#include <iostream>
#include <string>
#include <set>
#include <map>

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Storage structure for data associated with Blobs.
///	</summary>
struct BlobQuantities {

	///	<summary>
	///		Output quantities.
	///	</summary>
	enum OutputQuantity {
		MinLat,
		MaxLat,
		MinLon,
		MaxLon,
		CentroidLat,
		CentroidLon,
		Area
	};

	///	<summary>
	///		Latitude-longitude bounding box for blob.
	///	</summary>
	LatLonBox box;

	///	<summary>
	///		Total area of blob.
	///	</summary>
	double dArea;

	///	<summary>
	///		Array of output variables associated with blob.
	///	</summary>
	std::vector<double> dOutputVars;
};

///	<summary>
///		A map between blob index and quantities associated with the blob.
///	</summary>
typedef std::map<int, BlobQuantities> BlobQuantitiesMap;

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {

	NcError error(NcError::silent_nonfatal);

try {

	// Input file
	std::string strInputFile;

	// Input file list
	std::string strInputFileList;

	// Output file
	std::string strOutputFile;

	// Input variable
	std::string strInputVariable;

	// Summary quantities
	std::string strOutputQuantities;

	// Display help message
	bool fHelp;

	// Parse the command line
	BeginCommandLine()
		CommandLineString(strInputFile, "infile", "");
		CommandLineString(strInputFileList, "inlist", "");
		CommandLineString(strOutputFile, "outfile", "");
		CommandLineString(strInputVariable, "invar", "");
		CommandLineString(strOutputQuantities, "out", "");
		CommandLineBool(fHelp, "help");

		ParseCommandLine(argc, argv);
	EndCommandLine(argv)

	AnnounceBanner();

	// Check input
	if ((strInputFile == "") && (strInputFileList == "")) {
		_EXCEPTIONT("No input file (--in) or (--inlist) specified");
	}
	if ((strInputFile != "") && (strInputFileList != "")) {
		_EXCEPTIONT("Only one input file (--in) or (--inlist) allowed");
	}

	// Check output
	if (strOutputFile == "") {
		_EXCEPTIONT("No output file (--outfile) specified");
	}

	// Check variable
	if (strInputVariable == "") {
		_EXCEPTIONT("No variable name (--invar) specified");
	}

	// Check output variable
	if (strOutputQuantities == "") {
		_EXCEPTIONT("No output quantities (--out) specified");
	}

	// Input file list
	std::vector<std::string> vecInputFiles;

	if (strInputFile != "") {
		vecInputFiles.push_back(strInputFile);
	}
	if (strInputFileList != "") {
		GetInputFileList(strInputFileList, vecInputFiles);
	}

	int nFiles = vecInputFiles.size();

	// Load in spatial dimension data
	int nLat;
	int nLon;

	DataVector<double> dataLatDeg;
	DataVector<double> dataLat;

	DataVector<double> dataLonDeg;
	DataVector<double> dataLon;

	double dAreaElement;

	{
		// Load the first netcdf input file
		NcFile ncInput(vecInputFiles[0].c_str());

		if (!ncInput.is_valid()) {
			_EXCEPTION1("Unable to open NetCDF file \"%s\"",
				vecInputFiles[0].c_str());
		}

		// Get latitude/longitude dimensions
		NcDim * dimLat = ncInput.get_dim("lat");
		if (dimLat == NULL) {
			_EXCEPTIONT("No dimension \"lat\" found in input file");
		}

		NcDim * dimLon = ncInput.get_dim("lon");
		if (dimLon == NULL) {
			_EXCEPTIONT("No dimension \"lon\" found in input file");
		}

		NcVar * varLat = ncInput.get_var("lat");
		if (varLat == NULL) {
			_EXCEPTIONT("No variable \"lat\" found in input file");
		}

		NcVar * varLon = ncInput.get_var("lon");
		if (varLon == NULL) {
			_EXCEPTIONT("No variable \"lon\" found in input file");
		}

		nLat = dimLat->size();
		nLon = dimLon->size();

		dataLatDeg.Initialize(nLat);
		dataLat.Initialize(nLat);

		dataLonDeg.Initialize(nLon);
		dataLon.Initialize(nLon);

		varLat->get(dataLatDeg, nLat);
		for (int j = 0; j < nLat; j++) {
			dataLat[j] = dataLatDeg[j] * M_PI / 180.0;
		}

		varLon->get(dataLonDeg, nLon);
		for (int i = 0; i < nLon; i++) {
			dataLon[i] = dataLonDeg[i] * M_PI / 180.0;
		}

		dAreaElement =
			M_PI / static_cast<double>(nLat)
			* 2.0 * M_PI / static_cast<double>(nLon);

		// Close first netcdf file
		ncInput.close();
	}

	// Parse the list of output quantities
	std::vector<BlobQuantities::OutputQuantity> vecOutputVars;

	{
		// Parse output quantities and store in vecOutputVars
		AnnounceStartBlock("Parsing output quantities");

		int iLast = 0;
		for (int i = 0; i <= strOutputQuantities.length(); i++) {

			if ((i == strOutputQuantities.length()) ||
				(strOutputQuantities[i] == ',') ||
				(strOutputQuantities[i] == ';')
			) {
				std::string strSubStr =
					strOutputQuantities.substr(iLast, i - iLast);
			
				int iNextOp = (int)(vecOutputVars.size());
				vecOutputVars.resize(iNextOp + 1);

				if (strSubStr == "minlat") {
					vecOutputVars[iNextOp] = BlobQuantities::MinLat;
				} else if (strSubStr == "maxlat") {
					vecOutputVars[iNextOp] = BlobQuantities::MaxLat;
				} else if (strSubStr == "minlon") {
					vecOutputVars[iNextOp] = BlobQuantities::MinLon;
				} else if (strSubStr == "maxlon") {
					vecOutputVars[iNextOp] = BlobQuantities::MaxLon;
				} else if (strSubStr == "centlat") {
					vecOutputVars[iNextOp] = BlobQuantities::CentroidLat;
				} else if (strSubStr == "centlon") {
					vecOutputVars[iNextOp] = BlobQuantities::CentroidLon;
				} else if (strSubStr == "area") {
					vecOutputVars[iNextOp] = BlobQuantities::Area;
				} else {
					_EXCEPTIONT("Invalid output quantity:  Expected\n"
						"[minlat, maxlat, minlon, maxlon, "
						"centlat, centlon, area]");
				}

				iLast = i + 1;
			}
		}

		AnnounceEndBlock("Done");
	}

	// Time index across all files
	int iTime = 0;

	// Open output file
	FILE * fpout = fopen(strOutputFile.c_str(), "w");
	if (fpout == NULL) {
		_EXCEPTIONT("Error opening output file");
	}

	// Loop through all files
	for (int f = 0; f < nFiles; f++) {

		// Load in each file
		NcFile ncInput(vecInputFiles[f].c_str());
		if (!ncInput.is_valid()) {
			_EXCEPTION1("Unable to open input file \"%s\"",
				vecInputFiles[f].c_str());
		}

		// Blob index data
		DataMatrix<int> dataIndex(nLat, nLon);

		// Get current time dimension
		NcDim * dimTime = ncInput.get_dim("time");

		int nLocalTimes = dimTime->size();

		// Load in indicator variable
		NcVar * varIndicator = ncInput.get_var(strInputVariable.c_str());

		if (varIndicator == NULL) {
			_EXCEPTION1("Unable to load variable \"%s\"",
				strInputVariable.c_str());
		}

		if (varIndicator->num_dims() != 3) {
			_EXCEPTION1("Incorrect number of dimensions for \"%s\""
				" (3 expected)", strInputVariable.c_str());
		}

		// Loop through all times
		for (int t = 0; t < nLocalTimes; t++, iTime++) {

			BlobQuantitiesMap mapOutputQuantities;

			// Load in the data at this time
			varIndicator->set_cur(t, 0, 0);
			varIndicator->get(&(dataIndex[0][0]), 1, nLat, nLon);

			// Loop over all locations
			for (int j = 0; j < nLat; j++) {
			for (int i = 0; i < nLon; i++) {
				if (dataIndex[j][i] != 0) {
					BlobQuantitiesMap::iterator iterOutput =
						mapOutputQuantities.find(
							dataIndex[j][i]);

					if (iterOutput == mapOutputQuantities.end()) {
						std::pair<BlobQuantitiesMap::iterator,bool> pr =
							mapOutputQuantities.insert(
								BlobQuantitiesMap::value_type(
									dataIndex[j][i],
									BlobQuantities()));

						if (pr.second == false) {
							_EXCEPTIONT("Map insertion failed");
						}
						iterOutput = pr.first;

					}

					// Insert point into array
					iterOutput->second.box.InsertPoint(j, i, nLat, nLon);

					// Add blob area
					iterOutput->second.dArea +=
						cos(dataLat[j]) * dAreaElement;
				}
			}
			}

			// Calculate vecOutputVars
			BlobQuantitiesMap::iterator iter = mapOutputQuantities.begin();
			for (; iter != mapOutputQuantities.end(); iter++) {
				for (int i = 0; i < vecOutputVars.size(); i++) {

					// Bounding box coordinates
					double dLat0 = dataLatDeg[iter->second.box.lat[0]];
					double dLat1 = dataLatDeg[iter->second.box.lat[1]];
					double dLon0 = dataLonDeg[iter->second.box.lon[0]];
					double dLon1 = dataLonDeg[iter->second.box.lon[1]];

					// Blob index
					fprintf(fpout, "%i", iter->first);

					// Minimum latitude
					if (vecOutputVars[i] == BlobQuantities::MinLat) {
						fprintf(fpout, "\t%1.5f", dLat0);
					}

					// Maximum latitude
					if (vecOutputVars[i] == BlobQuantities::MaxLat) {
						fprintf(fpout, "\t%1.5f", dLat1);
					}

					// Minimum longitude
					if (vecOutputVars[i] == BlobQuantities::MinLon) {
						fprintf(fpout, "\t%1.5f", dLon0);
					}

					// Maximum longitude
					if (vecOutputVars[i] == BlobQuantities::MaxLon) {
						fprintf(fpout, "\t%1.5f", dLon1);
					}

					// Centroid latitude
					if (vecOutputVars[i] == BlobQuantities::CentroidLat) {
						double dMidLat = 0.5 * (dLat0 + dLat1);

						fprintf(fpout, "\t%1.5f", dMidLat);
					}

					// Centroid longitude
					if (vecOutputVars[i] == BlobQuantities::CentroidLon) {
						double dMidLon;
						if (dLon0 <= dLon1) {
							dMidLon = 0.5 * (dLon0 + dLon1);
						} else {
							dMidLon = 0.5 * (dLon0 + dLon1 + 2.0 * M_PI);
							if (dMidLon > 2.0 * M_PI) {
								dMidLon -= 2.0 * M_PI;
							}
						}

						fprintf(fpout, "\t%1.5f", dMidLon);
					}

					// Area
					if (vecOutputVars[i] == BlobQuantities::Area) {
						fprintf(fpout, "\t%1.5f", iter->second.dArea);
					}
				}
				fprintf(fpout, "\n");
			}
		}
	}

	fclose(fpout);

	// Done
	AnnounceEndBlock("Done");

	AnnounceBanner();

} catch(Exception & e) {
	Announce(e.ToString().c_str());
}
}

///////////////////////////////////////////////////////////////////////////////

