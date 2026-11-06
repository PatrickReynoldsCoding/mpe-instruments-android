add_executable(bench bench.cpp)
target_link_libraries(bench mpe_engine)
add_executable(bench_wasm bench_wasm.cpp)
target_link_libraries(bench_wasm mpe_engine)
