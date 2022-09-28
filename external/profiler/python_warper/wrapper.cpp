#include <pybind11/pybind11.h>
#include <iostream>
#include <string>

#include "profiler.hpp"
#include "events.hpp"

using namespace std;

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

PYBIND11_MODULE(viveka, m) {
    m.doc() = "viveka wrapped with pybind11";

    py::class_<Profiler>(m, "Profiler")
        .def("set_timestep", &Profiler::set_timestep)
        .def("set_rank", &Profiler::set_rank)
        .def("set_namespath", &Profiler::set_namespath)
        .def("dump", &Profiler::dump);
        
    // py::class_<Events>(m, "Events")
    //     .def(py::init<const std::string>())
    //     .def(py::init<const std::string, const std::string>())
    //     .def(py::init<const std::string, const int, const int>())
    //     .def(py::init<const std::string, const std::string, const int, const int>());

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}

