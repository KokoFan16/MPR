#ifndef EVENTS_H
#define EVENTS_H

#include <iostream>
#include <string>
#include <chrono>

#include "profiler.hpp"

using namespace std;

// the class to collect timing info
class Events {

    private:
        chrono::time_point<chrono::system_clock> start_time;
        double elapsed_time = 0; // cost of a event 
        string name; // name of a event
		int comEvent;
        string tags; // self-defined tag of a event (e.g., COMM)
        int is_loop = 0; // for loops
        int loop_ite = 0; // the iteration in a loop
        Profiler* context; // the context of a event

        void constr_help(string name);

    public:
        // constructors with different parameters
        Events(Profiler* ctx, string n, int ce);
        Events(Profiler* ctx, string n, int ce, string t);
        Events(Profiler* ctx, string n, int ce, int loop, int ite);
        Events(Profiler* ctx, string n, int ce, string t, int loop, int ite);
        
        // destructor 
        ~Events();
};


#endif /* EVENTS_H */
