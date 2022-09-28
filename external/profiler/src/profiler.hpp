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

using namespace std;

class Params {
	public:
		string tagloop;
		int nloop;
		vector<double> times;
//		string timeGather;
};

class Profiler {
	private:
		std::string filename;
		int write_mode;
	 	void sync_events();
		int nagg = 0;
		void write(char* buf, string fp, int n);
//		void gather_info();

	public:
		double timegap;
		int ntimestep;
		int curTs;
		int nprocs;
		int rank;
		int dump_count;
		int name_id = 0;
		string namespath; // call path of functions
		map<string, Params> output;
//		map<string, vector<string>> output; // store the output dictionary
		std::chrono::system_clock::time_point pstart;
		set<string> noncomEvents;
		map<string, int> name_encodes;
//		set<string> cachedEvents;

		Profiler(string outname, double tg, int p, int np, int nts);

		void set_context(string name, int t, int wmode, int na=0);
        void dump(); // gather info from all the processes
};

#endif /* PROFILER_H */
