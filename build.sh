
(
set -e
cd utils && rm -rf build/ && mkdir -p build && cd build
cmake ..
make
make install
)

(
set -e
cd GTest && rm -rf build/ && mkdir build && cd build
cmake .. -DUTILS_PATH=`pwd`/../../utils/build/install
make
for f in test* ; do
    ./${f}
done
)

(
set -e
cd DesignPattern && rm -rf build/ && mkdir build && cd build
cmake ..
make
for f in test* ; do
    ./${f}
done
)
