# pmake

make c++ projects from templates.

# building & installation

you will need a c++23 compatible compiler.

``cmake --preset release``
``cmake --build build``
``cmake --install build`` (you may want to configure [https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX.html](-DCMAKE_INSTALL_PREFIX))

# usage

## help
``pmake --help``

## executable project
``pmake --name example``

## library project
``pmake --name example --kind library --mode static``

