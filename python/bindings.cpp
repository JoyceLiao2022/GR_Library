#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <gr/gr.h>
namespace py = pybind11;

PYBIND11_MODULE(gr_python, m) {
    m.doc() = "Python bindings for the GR library";
    
    // Assignment binding
    py::class_<gr::Assignment>(m, "Assignment")
        .def_readonly("variable", &gr::Assignment::variable)
        .def_readonly("states", &gr::Assignment::states);

    // GSR function binding
    m.def(
        "compute_gsrs_from_file",
        &gr::compute_gsrs_from_file,
        py::arg("filepath")
    );

    // GNR function binding
    m.def(
        "compute_gnrs_from_file",
        &gr::compute_gnrs_from_file,
        py::arg("filepath")
    );
}