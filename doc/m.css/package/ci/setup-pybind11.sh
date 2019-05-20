set -e

wget --no-clobber https://github.com/pybind/pybind11/archive/v2.2.4.tar.gz && tar -xzf v2.2.4.tar.gz

cd pybind11-2.2.4
mkdir -p build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/pybind11 \
    -DPYBIND11_PYTHON_VERSION=3.6 \
    -DPYBIND11_TEST=OFF \
    -G Ninja
ninja install
