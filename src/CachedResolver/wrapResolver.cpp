#include "resolver.h"

#include <pxr/pxr.h>
#include "pxr/usd/ar/resolver.h"
#include <pybind11/pybind11.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace py = pybind11;

void wrapResolver(py::module_ &m)
{
    using This = CachedResolver;

    py::class_<This, ArResolver, std::shared_ptr<This>>(m, "Resolver")
        .def(py::init<>())  // remove if your resolver is non-default-constructible
    ;
}

void wrapArResolver(py::module_ &m)
{
	py::class_<ArResolver, std::shared_ptr<ArResolver>>(m, "ArResolver");
}
