cmake \
    -Bbuild \
    -DCMAKE_BUILD_TYPE=Debug \
    -GNinja \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_C_COMPILER=clang-18 \
    -DCMAKE_CXX_COMPILER=clang++-18 \
    
cmake --build build

./build/Playground_artefacts/Debug/Standalone/Playground