#!/bin/bash
set -e

# run from the location of this script
cd "$(dirname "$0")"
cd ..

rm -rf build
mkdir build
cd build
<<<<<<< HEAD
cmake -DBUILD_SERVER=ON -DINCREASED_SERVER_LIMITS=ON ..
cmake --build . --config Release
=======
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SERVER=ON -DINCREASED_SERVER_LIMITS=OFF ..
cmake --build .
>>>>>>> a89d3d3c287b675e5a54df2e93e7445e6b1e3592
