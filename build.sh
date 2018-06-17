rm -rf build
rm -rf Test/build

(
mkdir build
cd build
cmake ..
make
make install
)

(
cd Test
mkdir build
cd build
cmake ..
make
./runTests
)
