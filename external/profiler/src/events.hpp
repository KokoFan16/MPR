#ifndef EVENTS_H
#define EVENTS_H

#include <iostream>
#include <string>
#include <chrono>

#include "profiler.hpp"

// the class to collect timing info
class Events {

    private:
		std::chrono::time_point<std::chrono::system_clock> start_time;
        double elapsed_time = 0; // cost of a event 
        std::string name; // name of a event
		int comEvent;
		std::string tags; // self-defined tag of a event (e.g., COMM)
        int is_loop = 0; // for loops
        int loop_ite = 0; // the iteration in a loop
        Profiler* context; // the context of a event

        void constr_help(std::string name);

    public:
        // constructors with different parameters
        Events(Profiler* ctx, std::string n, int ce);
        Events(Profiler* ctx, std::string n, int ce, std::string t);
        Events(Profiler* ctx, std::string n, int ce, int loop, int ite);
        Events(Profiler* ctx, std::string n, int ce, std::string t, int loop, int ite);
        
        // destructor 
        ~Events();
};


#endif /* EVENTS_H */
