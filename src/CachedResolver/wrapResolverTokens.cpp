#include "resolverTokens.h"

#include <pybind11/pybind11.h>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace py = pybind11;

namespace {
    template <typename Cls>
    void _AddToken(Cls& cls, const char* name, const TfToken& token) {
        cls.def_property_readonly_static(
            name,
            [token](py::object /* self */) {
                return token.GetString();
            }
            // REMOVE docstring entirely or replace with a literal if needed
        );
    }
}

void wrapResolverTokens(py::module_ &m)
{
    auto cls = py::class_<CachedResolverTokensType>(m, "Tokens", py::module_local());
    _AddToken(cls, "mappingPairs", CachedResolverTokens->mappingPairs);
}
