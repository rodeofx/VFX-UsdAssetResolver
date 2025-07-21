#include <pybind11/pybind11.h>

void wrapResolver(pybind11::module_ &);
void wrapResolverContext(pybind11::module_ &);
void wrapResolverTokens(pybind11::module_ &);

PYBIND11_MODULE(_cachedResolver, m) {
    wrapResolver(m);
    wrapResolverContext(m);
    wrapResolverTokens(m);
}
