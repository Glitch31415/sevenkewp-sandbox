#!/bin/bash
set -e

# run from the location of this script
cd "$(dirname "$0")"
cd ..

rm -rf build
mkdir build
cd build
<<<<<<< HEAD
cmake -DBUILD_SERVER=ON -DINCREASED_SERVER_LIMITS=ON -j 20 ..
cmake --build . --config Release -j 20
=======
cmake -DBUILD_SERVER=ON -DINCREASED_SERVER_LIMITS=OFF ..
cmake --build . --config Release
>>>>>>> 00d71cc24759aa0e4b7e506965e441c691e27ba6
