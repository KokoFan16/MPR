#ifndef PROFILER_H_
#define PROFILER_H_

#include <iostream>
#include <string>
#include <map> 
#include <vector>
#include <sstream>
#include <iterator>
#include <mpi.h>
#include <stdio.h>
#include <set>
#include <unordered_set>
#include <math.h>

class Params {
	public:
		std::string tagloop;
		int nloop;
		std::vector<double> times;
//		string timeGather;
};

class Profiler {
	private:
		std::string filename;
		int write_mode;
	 	void sync_events();
		int nagg = 0;
		void write(char* buf, std::string fp, int n);
//		void gather_info();

	public:
		double timegap;
		int ntimestep;
		int curTs;
		int nprocs;
		int rank;
		int dump_count;
		int name_id = 0;
		std::string namespath; // call path of functions
		std::map<std::string, Params> output;
//		map<string, vector<string>> output; // store the output dictionary
		std::chrono::system_clock::time_point pstart;
		std::set<std::string> noncomEvents;
		std::map<std::string, int> name_encodes;
//		set<string> cachedEvents;

		Profiler(std::string outname, double tg, int p, int np, int nts);

		void set_context(std::string name, int t, int wmode, int na=0);
        void dump(); // gather info from all the processes
};

#endif /* PROFILER_H */
