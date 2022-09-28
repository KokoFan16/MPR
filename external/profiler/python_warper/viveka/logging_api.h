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

// gather info from all the processes
static int gather_info() {

	double total_start = MPI_Wtime();

	double max_start = MPI_Wtime();
	std::map<std::string, std::string> ::iterator p1; // map pointer
	std::string message = ""; // merged message for sending
	int strLen = 0, keyLen = 0;

	// calculate total size of events ( e.g., "main" + " " + "comm")
	for (p1 = output.begin(); p1 != output.end(); p1++)
		keyLen += p1->first.size() + 1;

	// find the max total size of events among all processes and the corrsponding rank
	int elocal[2] = {keyLen, rank};
	int eglobal[2];
	MPI_Allreduce(elocal, eglobal, 1, MPI_2INT, MPI_MAXLOC, MPI_COMM_WORLD);
	double max_end = MPI_Wtime();

	double bcast_start = MPI_Wtime();
	std::string events = "";
	if (rank == eglobal[1]) { // concatenate events to a string for bcast
		for (p1 = output.begin(); p1 != output.end(); p1++)
			events += (p1->first) + ' ';
	}
	events.resize(eglobal[0]);
	// bcast the events string from the rank with max events to others
	MPI_Bcast((char*)events.c_str(), eglobal[0], MPI_CHAR, eglobal[1], MPI_COMM_WORLD);
	double bcast_end = MPI_Wtime();

	double pad_start = MPI_Wtime();
    std::vector<std::string> maxEvents;
    std::istringstream f(events);
    std::string s;
    p1 = output.begin();
    while (std::getline(f, s, ' ') && p1 != output.end()) {
    	std::string times;
    	// loop all the events, and set the time of this event to be "0.000000" if a process doesn't have it
    	if ( s != p1->first) {
    		times = "0.000000";
    		for (int t = 1; t < ntimestep; t++) {times += "-0.000000";}
    	}
    	else { // else calculate times for all events
			std::size_t found = p1->second.rfind(";");
			times = p1->second.substr(found+1, p1->second.length()-found-1);
	    	p1++;
    	}
    	message += times + ' ';
    	maxEvents.push_back(s);
    }
	message.pop_back();
	message += ','; // add comma sat the end of each message
	strLen = message.length();
	double pad_end = MPI_Wtime();

	double gather_start = MPI_Wtime();
	long long totalLen = strLen * nprocs + 1;
	char* gather_buffer = (char*)malloc(totalLen * sizeof(char));
	MPI_Gather((char*)message.c_str(), strLen, MPI_CHAR, gather_buffer, strLen, MPI_CHAR, eglobal[1], MPI_COMM_WORLD);
	double gather_end = MPI_Wtime();

	double filter_start = MPI_Wtime();
	if (rank == eglobal[1]) {
		for (p1 = output.begin(); p1 != output.end(); p1++) {
			std::size_t loc = p1->second.find(';');
			p1->second.erase(loc+1, p1->second.size()-loc-1);
		}

		gather_buffer[totalLen-1] = '\0'; // end symbol of string
		std::string gather_message = std::string(gather_buffer); // convert it to string

		std::size_t pos;
		for (int i = 0; i < nprocs; i++) { // loop all the processes expect rank 0 
			pos = gather_message.find(',');
			std::string pmessage = gather_message.substr(0, pos); // message from a process
			std::size_t found;
			for (int j = 0; j < maxEvents.size(); j++) { // loop all the events
				found = pmessage.find(' ');
				output[maxEvents[j]] += pmessage.substr(0, found) + "|"; // add to corresponding event
				pmessage.erase(0, found+1);
			}
			gather_message.erase(0, pos+1);
		}
	}
	double filter_end = MPI_Wtime();

	double total_end = MPI_Wtime();
	double total_time = total_end - total_start;
	double max_time;
	MPI_Allreduce(&total_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
	
	// if (total_time == max_time) {
		std::cout << rank << ": total: " << total_time << ", " << 
			"findMaxE: " << (max_end - max_start) << ", " <<
			"bcast: " << (bcast_end - bcast_start) << ", " <<
			"padE: " << (pad_end - pad_start) << ", " <<
			"gather: " << (gather_end - gather_start) << ", " <<
			"filter: " << (filter_end - filter_start) << std::endl;
	// }

	free(gather_buffer);
	return eglobal[1];
}

/// write csv file out
static void write_output(std::string filename, int flag=0) {
	int master = gather_info(); // gather info from all the processes

	if (rank == master) { // rank 0 writes csv file
		std::string filePath = filename + "_" + std::to_string(ntimestep) + "_" + std::to_string(nprocs) + ".csv"; // create file path
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
	}

	output.clear();
}

#endif /* LOGGING_API_H_ */
