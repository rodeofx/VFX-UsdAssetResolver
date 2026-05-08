# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository purpose

Rodeo FX fork of LucaScheller's `VFX-UsdAssetResolver` — reference implementations of Pixar USD AR 2.0 asset resolvers. Each resolver compiles to a C++ shared library plus a Python wrapper module loaded by USD via `plugInfo.json`.

## Building

Build invokes CMake. The resolver to build is selected by the `RESOLVER_NAME` environment variable (`fileResolver`, `pythonResolver`, `cachedResolver`, or `httpResolver`); CMake reads it and flips the corresponding `AR_<NAME>_BUILD` option ON.

- `./build.sh` — wipes `build/` and `dist/`, runs `cmake . -B build && cmake --build build && cmake --install build`. Edit the `export RESOLVER_NAME=...` line at the top to switch resolvers.
- `./rdo_build.sh` — Rodeo entrypoint: invokes `build.sh` twice inside `rez env` (once for standalone USD, once for Houdini). Use this when building in the Rodeo environment.
- `ctest -VV --test-dir build` — run the unittest-based test suite (registered via `add_test` in each resolver's `CMakeLists.txt`; uses `$HFS/python/bin/python -B -m unittest discover testenv`). Tests are disabled on Windows.
- Single test: `python -B -m unittest src/<Resolver>/testenv/test<Resolver>.py` after exporting `PYTHONPATH`, `PXR_PLUGINPATH_NAME`, and `LD_LIBRARY_PATH` to point at `dist/<resolverName>/`. The `CMakeLists.txt` in each resolver shows the exact env vars (search for `TESTS_ENV_*`).

`source setup.sh` configures the runtime env (`PYTHONPATH`, `PXR_PLUGINPATH_NAME`, `LD_LIBRARY_PATH`, `AR_SEARCH_PATHS`, etc.) for whichever resolver `RESOLVER_NAME` names. It also aliases `usdpython` to Houdini's standalone python, which is the way to drive the resolver from a REPL.

## Big-picture architecture

### Distribution targets (CMakeLists.txt:26)

`AR_DIST_NAME` is chosen automatically by inspecting env vars: `HOUDINI_NETWORK_DIR` → `houdini`, `MAYAUSD_NETWORK_DIR` → `maya`, else → `usd`. This drives where USD/Python headers and libs are pulled from, the lib prefix (`pxr_` vs `usd_`/`libusd_`), and the boost namespace (`hboost` for Houdini, `boost` otherwise). Install dir is `dist/<AR_DIST_NAME>/`.

### Boost namespace indirection

Houdini ships boost rebuilt under the `hboost` namespace via bcp. C++ source uses `BOOST_INCLUDE(...)` from `src/utils/boost_include_wrapper.h` instead of literal `<boost/...>` includes; `setBoostNamespace.cmake` defines `AR_BOOST_NAMESPACE` per target so the same source compiles against both. Don't hardcode `boost/...` in includes.

### CXX11 ABI auto-detection

`check_glibcxx_use_cxx11_abi` runs `readelf` on the existing `libusd_ar.so`/`libpxr_ar.so` to decide whether to compile with `_GLIBCXX_USE_CXX11_ABI=1` or `0`. This must match the USD/Houdini build to avoid std::string ABI mismatches at link time. Don't override manually.

### Per-resolver layout (src/<Resolver>/)

Every resolver follows the same shape:

- `resolver.cpp/h` — `ArResolver` subclass; AR_DEFINE_RESOLVER registers it. The "core" of each resolver.
- `resolverContext.cpp/h` — `ArResolverContext` payload (search paths, mapping pairs, cache).
- `resolverTokens.cpp/h` — `TfToken` constants exposed to Python.
- `debugCodes.cpp/h` — `TF_DEBUG_CODES(...)` for `TF_DEBUG=<NAME>_RESOLVER` logging.
- `wrapResolver*.cpp` — `pxr_boost::python` bindings; `module.cpp` + `moduleDeps.cpp` register the module with `Tf.PreparePythonModule`.
- `__init__.py` — Python module entry; calls `Tf.PreparePythonModule()`.
- `plugInfo.json.in` — USD plugin manifest, configured by CMake (`@VAR@` substitution).
- `testenv/test<Resolver>.py` — unittest suite executed by `ctest`.

CMake builds two shared libs per resolver: the C++ resolver itself (e.g. `cachedResolver.so`, no `lib` prefix) and a Python module (e.g. `_cachedResolver.so`). They install to `dist/<dist>/<resolverName>/lib/` and `dist/<dist>/<resolverName>/lib/python/usdAssetResolver/<Resolver>/`.

### Resolver flavors

- **FileResolver** — pure C++; resolves against `AR_SEARCH_PATHS`, supports a mapping file and `AR_SEARCH_REGEX_EXPRESSION` / `AR_SEARCH_REGEX_FORMAT` regex preformatting.
- **PythonResolver** — same surface as FileResolver but every method calls into `PythonExpose.py`. For RnD/prototyping; slow but flexible.
- **CachedResolver** — C++ resolver with an internal cache keyed off the context; cache misses redirect to `PythonExpose.py:ResolveAndCache`, results are cached on the C++ side via `context.AddCachingPair(...)`. Production-grade speed with Python-side flexibility. Rodeo's `PythonExpose.py` wires this into `rdo_publish_pipeline.manager` via the `rdojson:` URI scheme (declared in `plugInfo.json.in`); the env var `RDO_USD_CACHED_RESOLVER_DISABLE_SHOTGRID=1` short-circuits the ShotGrid lookup.
- **HttpResolver** — git submodule (`src/HttpResolver/arHttp`); the wrap-only build target lives under `src/HttpResolver/wrap`. Maintained upstream at `charlesfleche/arHttp`; only the build glue lives here.

### Things easy to miss

- The `RESOLVER_NAME` env var is a switch, not a build of all resolvers. Each `build.sh` run produces exactly one resolver's artifacts.
- `_IsFileRelativePath` (only `./` or `../` prefixed paths) is intentionally narrower than `TfIsRelativePath`; mixing the two will misclassify URI-like asset paths. See recent Rodeo commit history.
- `_GLIBCXX_USE_CXX11_ABI` is decided from a sibling .so on disk — you cannot build until USD is in `AR_PXR_LIB_DIR`.
- The `usdAssetResolver` Python parent package is set by `AR_RESOLVER_USD_PYTHON_MODULE_NAME` in the top-level CMakeLists; sub-resolvers install as `usdAssetResolver.<Resolver>`.
