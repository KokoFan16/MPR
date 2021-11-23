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

#include "csvWrite.h"

extern int ntimestep;
extern int curTs;
extern int nprocs;
extern int curRank;
extern std::string namespath; // call path of functions


static void set_timestep(int t, int n) {curTs = t; ntimestep = n;} // set the number of timesteps and current timestep
static void set_rank(int r, int n) {curRank = r; nprocs = n;} // set the number of processes and the current rank
static void set_namespath(std::string name) {namespath = name;} // set the call path of functions

static std::map<std::string, std::string> output; // store the output dictionary

// the class to collect timing info
class Events
{
private:
	std::chrono::time_point<std::chrono::system_clock> start_time;
	double elapsed_time = 0;
	std::string name;
	std::string tags;
	int is_loop = 0;
	int loop_ite = 0;
	long bsize = 0;


public:
	Events(std::string n, std::string t, long size=0, int loop=0, int ite=0)
	{
		name = n;
		tags = t;
		is_loop = loop;
		loop_ite = ite;
		bsize = size;
		auto start = std::chrono::system_clock::now();
		start_time = start; // get start time
		if (namespath == "") { namespath += name; } // set name-path as key
		else { namespath += "-" + name; }
	}
	~Events() {
		auto end_time = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end_time-start_time; // calculate duration
		elapsed_time = elapsed_seconds.count();

		std::string delimiter = "-";
		std::size_t found = namespath.rfind(delimiter);

		// set value (time and tag) of each function across all the time-steps
		if (curTs == 0 && output[namespath] == ""){
			output[namespath] += tags + "-" + std::to_string(bsize) + "-" + std::to_string(is_loop) + ";" + std::to_string(elapsed_time);
		}
		else {
			if (is_loop == 2) {
				if (loop_ite == 0){ output[namespath] += "-" + std::to_string(elapsed_time); }
				else {
					int found;
					if (curTs == 0) {
						found = output[namespath].rfind(";");
					}
					else {
						found = output[namespath].rfind("-");
					}
					double curTime = std::stod(output[namespath].substr(found+1, output[namespath].length()-found-1)) + elapsed_time;
					output[namespath].replace(found+1, std::to_string(curTime).length(), std::to_string(curTime));
				}
			}
			else {
				if (loop_ite == 0){ output[namespath] += "-" + std::to_string(elapsed_time); }
				else { output[namespath] += "+" + std::to_string(elapsed_time); }
			}
		}
		namespath = namespath.substr(0, found); // back to last level
	}
};

// gather info from all the processes
static int gather_info()
{
	std::map<std::string, std::string> ::iterator p1; // map pointer
	std::string message = ""; // merged message for sending
	int strLen = 0;

	std::string events;
	for (p1 = output.begin(); p1 != output.end(); p1++)
		events += (p1->first) + ' ';
	events.pop_back();

	int elocal[2] = {int(events.size()), curRank};
	int eglobal[2];
	MPI_Allreduce(elocal, eglobal, 1, MPI_2INT, MPI_MAXLOC, MPI_COMM_WORLD);
	events.resize(eglobal[0]);
	MPI_Bcast((char*)events.c_str(), eglobal[0], MPI_CHAR, eglobal[1], MPI_COMM_WORLD);

    std::vector<std::string> maxEvents;
    std::istringstream f(events);
    std::string s;
    p1 = output.begin();
    while (std::getline(f, s, ' ') && p1 != output.end()) {
    	std::string times;
    	if ( s != p1->first){
    		times = "0.000000";
    		for (int t = 1; t < ntimestep; t++) {times += "-0.000000";}
    	}
    	else {
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

	int* messageLens = (int*)malloc(nprocs * sizeof(int));
	MPI_Gather(&strLen, 1, MPI_INT, messageLens, 1, MPI_INT, eglobal[1], MPI_COMM_WORLD);

	int *displs = NULL;
	int totalLen = 0;
	char* gather_buffer = NULL;
	if (curRank == eglobal[1]) {
		displs = (int*)malloc(nprocs * sizeof(int));
		for (int i = 0; i < nprocs; i++) {
			displs[i] = totalLen;
			totalLen += messageLens[i];
		}
		gather_buffer = (char*)malloc((totalLen+1) * sizeof(char));
	}

	MPI_Gatherv((char*)message.c_str(), strLen, MPI_CHAR, gather_buffer, messageLens, displs, MPI_CHAR, eglobal[1], MPI_COMM_WORLD);
	free(messageLens);
	free(displs);


	if (curRank == eglobal[1])
	{
		for (p1 = output.begin(); p1 != output.end(); p1++) {
			std::size_t loc = p1->second.find(';');
			p1->second.erase(loc+1, p1->second.size()-loc-1);
		}

		gather_buffer[totalLen-1] = '\0'; // end symbol of string
		std::string gather_message = std::string(gather_buffer); // convert it to string

		std::size_t pos;
		for (int i = 0; i < nprocs; i++) // loop all the processes expect rank 0
		{
			pos = gather_message.find(',');
			std::string pmessage = gather_message.substr(0, pos); // message from a process
			std::size_t found;
			for (int j = 0; j < maxEvents.size(); j++) // loop all the events
			{
				found = pmessage.find(' ');
				output[maxEvents[j]] += pmessage.substr(0, found) + "|"; // add to corresponding event
				pmessage.erase(0, found+1);
			}
			gather_message.erase(0, pos+1);
		}
	}

	free(gather_buffer);

	return eglobal[1];
}

static void write_output(std::string filename, int flag=0)
{
	int master = gather_info(); // gather info from all the processes

	if (curRank == master) // rank 0 writes csv file
	{
		std::string filePath = filename + ".csv"; // create file path
		csvfile csv(filePath); // open CSV file

		// set CSV file Hearer
		csv << "id" << "parent" << "tag" << "size" << "is_loop" << "times" << endrow;

		std::map<std::string, std::string> ::iterator p1;
		std::size_t found;
		// set CSV file content
		for (p1 = output.begin(); p1 != output.end(); p1++)  {
			std::size_t found = p1->first.rfind("-");
			std::string parent, value, times, tag, size, loop;
			// get parent
			if (found != std::string::npos) { parent = p1->first.substr(0, found); }
			else { parent = "null"; }

			// get tag and times
			found = p1->second.find(';');
			value = p1->second.substr(0, found);
			times = p1->second.substr(found+1, p1->second.length()-found-2);

			found = value.find('-');
			tag = value.substr(0, found);
			std::string temp = value.substr(found+1, value.length()-found-1);
			found = temp.find('-');
			size = temp.substr(0, found);
			loop = temp.substr(found+1, 1);

			// set CSV file content
			csv << p1->first << parent << tag << size << loop << times << endrow;
		}
	}

	output.clear();
}

#endif /* LOGGING_API_H_ */
