g++ --version

#! https://stackoverflow.com/a/246128
SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
ROOT_DIR=$(realpath $SCRIPT_DIR/..)

echo "Got root of directory: $ROOT_DIR"
mkdir -p $ROOT_DIR/build

c_params="-O3 -Wall -std=c++20 -I $ROOT_DIR/src/include"
pybind_params="-O3 -Wall -fvisibility=hidden -shared -std=c++20 -fPIC $(python3-config --includes) -I $ROOT_DIR/src/include -I $ROOT_DIR/extern/pybind11/include"
pybind_extension=$(python3-config --extension-suffix)

execute() {
    command=$1
    echo "Running \"$command\""
    $command

    status=$?
    if [ $status -ne 0 ]; then
        echo "::error::\"$command\" exit with status $status"
        exit $status
    fi
}

execute "g++ $pybind_params $ROOT_DIR/src/core/c_utils.cpp -o $ROOT_DIR/src/core/c_utils$pybind_extension"
execute "g++ $c_params $ROOT_DIR/src/learn.cpp -o $ROOT_DIR/build/learn.exe"
