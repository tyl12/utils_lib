rm -rf build
rm -rf Test/build

(
set -e
mkdir build
cd build
cmake ..
make
make install
)

(
set -e
cd Test
mkdir build
cd build
cmake ..
make
./runTests
)

