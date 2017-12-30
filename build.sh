#!/bin/sh

BUILD_DEBUG=0

# Parsing arguments
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -d|--debug)
    BUILD_DEBUG=1
    shift # past argument
    shift # past value
    ;;
    -r|--release)
    BUILD_DEBUG=0
    shift # past argument
    shift # past value
    ;;
    *)    # unknown option
    shift # past argument
    ;;
esac
done

if [ ! -d "bin" ]; then
  mkdir bin
fi
pushd bin > /dev/null
if [[ "$BUILD_DEBUG" -eq 1 ]]; then
  echo Building Debug...
  cmake -DCMAKE_BUILD_TYPE=Debug ..
else
  echo Building Release...
  cmake -DCMAKE_BUILD_TYPE=Release ..
fi
cmake ..
make || exit
popd > /dev/null
pushd test > /dev/null
../bin/test/test || exit
popd > /dev/null

