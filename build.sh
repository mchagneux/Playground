cmake \
    -Bbuild \
    -GNinja \
    -DCMAKE_LINKER_TYPE=LLD \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_CXX_COMPILER=clang++-18 \
    -DCMAKE_C_COMPILER=clang-18
    # -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS} \
    #     -std=c++17 \
    #     -nostdinc++ \
    #     -nostdlib++ \
    #     -isystem /usr/lib/llvm-18/include/c++/v1 \
    #     -L /usr/lib                  \
    #     -Wl,-rpath /usr/lib          \
    #     -lc++ -lc++experimental -lc++abi" \
    # -DCMAKE_EXE_LINKER_FLAGS="${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++ -lc++abi" \
    # -DCMAKE_SHARED_LINKER_FLAGS="${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++ -lc++ -lc++abi" \

cmake --build build