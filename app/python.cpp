#include <pybind11/pybind11.h>

#include "run.h"

namespace {

namespace py = pybind11;

} // namespace

PYBIND11_MODULE(pylandbrush, m)
{
  py::class_<extension_registry>(m, "ExtensionRegistry", "Used for registering extensions from Python.")
    .def(py::init<>());

  m.def("main", &run_landbrush_app, py::arg("ext_registry") = extension_registry(), py::arg("app_name") = "landbrush");
}
