/*
 * logging_api.h
 *
 * Get timing info from HPC applications
 */

#ifndef LOGGING_API_H_
#define LOGGING_API_H_

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <typeinfo>
#include <sstream>
#include <iterator>
#include <map>
#include <fstream>
#include <sys/stat.h>
#include <errno.h>

extern int ntimestep;
extern int curTs;
extern int nprocs;
extern int rank;
extern std::string namespath; // call path of functions


static void set_timestep(int t, int n) {curTs = t; ntimestep = n;} // set the number of timesteps and current timestep
static void set_rank(int r, int n) {rank = r; nprocs = n;} // set the number of processes and the current rank
static void set_namespath(std::string name) {namespath = name;} // set the call path of functions

static std::map<std::string, std::string> output; // store the output dictionary

// the class to collect timing info
class Events {

private:
	std::chrono::time_point<std::chrono::system_clock> start_time;
	double elapsed_time = 0; // cost of a event 
	std::string name; // name of a event
	std::string tags; // self-defined tag of a event (e.g., COMM)
	int is_loop = 0; // for loops
	int loop_ite = 0; // the iteration in a loop

	void constr_help(std::string name) {
		auto start = std::chrono::system_clock::now(); // get start time of a event
		start_time = start;
		if (namespath == "") { namespath += name; } // set name-path as key
		else { namespath += ">" + name; } // concatenate name-path (e.g., main<computation)
	}

public:
	// constructors with different parameters
	Events(std::string n): name(n) { constr_help(n); }
	Events(std::string n, std::string t): name(n), tags(t) { constr_help(n); }
	Events(std::string n, int loop, int ite): name(n), is_loop(loop), loop_ite(ite) { constr_help(n); }
	Events(std::string n, std::string t, int loop, int ite): name(n), tags(t), is_loop(loop), loop_ite(ite) { constr_help(n); }
	
	// destructor 
	~Events() {
		auto end_time = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end_time-start_time; // calculate duration
		elapsed_time = elapsed_seconds.count();

		std::string delimiter = ">";
		std::size_t found = namespath.rfind(delimiter);

		// // set value (time and tag) of each function across all the time-steps
		if (curTs == 0 && output[namespath] == ""){
			output[namespath] += tags + "/" + std::to_string(is_loop) + ";" + std::to_string(elapsed_time);
		}
		else {
			if (is_loop == 2) { // mode for summing up time of loop events
				if (loop_ite == 0){ output[namespath] += "-" + std::to_string(elapsed_time); }
				else {
					if (curTs == 0) { delimiter = ";"; }
					else { delimiter = "-"; }
					int pos = output[namespath].rfind(delimiter); // find the position of current time
					// calculate the sum of the loop events
					double curTime = std::stod(output[namespath].substr(pos+1, output[namespath].length()-pos-1)) + elapsed_time;
					output[namespath].replace(pos+1, std::to_string(curTime).length(), std::to_string(curTime));
				}
			}
			else { // mode for storing all time of loop events
				if (loop_ite == 0){ output[namespath] += "-" + std::to_string(elapsed_time); }
				else { output[namespath] += "+" + std::to_string(elapsed_time); }
			}
		}
		namespath = namespath.substr(0, found); // back to last level
	}
};

/// the class for writing csv file
class CSVWrite {

private:
    std::ofstream fs;
    const std::string delimiter;
    bool begin;


    /// write string
	CSVWrite& write (const std::string & val) {
		if (!begin){ fs << delimiter; }
		else { begin = false; }
		fs << val;
		return *this;
    }

public:
	CSVWrite(const std::string filename, const std::string deli = ","): fs(), delimiter(deli), begin(true) {
        fs.exceptions(std::ios::failbit | std::ios::badbit);
        fs.open(filename); // open file
    }

    ~CSVWrite() { fs.flush(); fs.close(); } // flush and close file

	void endrow() { fs << std::endl; begin = true; } // end of each line

	CSVWrite& operator << ( CSVWrite& (* val)(CSVWrite&)) { return val(*this); } // overwrite operator <<

	CSVWrite& operator << (const std::string & val) { return write(val); } // write string
};

inline static CSVWrite& endrow(CSVWrite& file) {
    file.endrow();
    return file;
}

/// write csv file out
static void write_output(std::string filename, int flag=0) {

	if (rank == 0) {
		int ret = mkdir(filename.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
		if (ret != 0 && errno != EEXIST) {
			fprintf(stderr, "Error: failed to mkdir %s\n", filename.c_str());
		}
	}
	MPI_Barrier(MPI_COMM_WORLD);

	// file name for each process
	std::string file = filename + "_" + std::to_string(ntimestep) + "_" + std::to_string(nprocs) + "_" + std::to_string(rank) + ".csv"; 
	std::string filePath = filename + "/" + file;

	CSVWrite csv(filePath); // open CSV file

	// set CSV file Hearer
	csv << "id" << "tag" << "is_loop" << "times" << endrow;

	std::map<std::string, std::string> ::iterator p1;
	std::size_t found;
	
	// set CSV file content
	for (p1 = output.begin(); p1 != output.end(); p1++)  {
		std::string value, tag, loop, times;

		// get tag and times
		found = p1->second.find(';');
		value = p1->second.substr(0, found);
		times = p1->second.substr(found+1, p1->second.length()-found-2);

		found = value.find('/');
		tag = value.substr(0, found);
		loop = value.substr(found+1, 1);

		// set CSV file content
		csv << p1->first << tag << loop << times << endrow;
	}

	output.clear();
}

#endif /* LOGGING_API_H_ */
