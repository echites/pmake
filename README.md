# pmake

make c++ projects from templates.

# building & installation

you will need a c++23 compatible compiler.

``cmake --preset release``\
``cmake --build build``\
``cmake --install build`` (you may want to configure [-DCMAKE_INSTALL_PREFIX](https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX.html))

# usage

* ``pmake --help``
* ``pmake --name example``
* ``pmake --name example --kind library --mode static``

