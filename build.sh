#!/bin/sh
if [ ! -d "bin" ]; then
  mkdir bin
fi
cmake bin
pushd bin > /dev/null
make || exit
popd > /dev/null
pushd test > /dev/null
../bin/test/test || exit
popd > /dev/null

