cmake_minimum_required(VERSION 3.18)
project(mpe_engine CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(MPE_SANITIZE "" CACHE STRING "address, undefined or thread: build the engine and tests with that sanitizer")
if(MPE_SANITIZE)
  add_compile_options(-fsanitize=${MPE_SANITIZE} -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=${MPE_SANITIZE})
endif()


# Where the instruments come from: the faust-mpe-instruments submodule.
set(MPE_INSTRUMENTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/mpe-instruments"
    CACHE PATH "checkout of faust-mpe-instruments")
set(FAUST_EXECUTABLE faust CACHE STRING "faust compiler")
execute_process(COMMAND ${FAUST_EXECUTABLE} --version OUTPUT_QUIET ERROR_QUIET RESULT_VARIABLE faust_rc)
if(NOT faust_rc EQUAL 0)
  message(FATAL_ERROR "cannot run '${FAUST_EXECUTABLE}'. Install Faust, or pass -DFAUST_EXECUTABLE=...")
endif()
# Our own copy of the few Faust headers needed (see third_party/faust/README.md).
set(FAUST_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/faust/include")

file(GLOB_RECURSE DSP_FILES RELATIVE "${MPE_INSTRUMENTS_DIR}/instruments" CONFIGURE_DEPENDS
     "${MPE_INSTRUMENTS_DIR}/instruments/*/*.dsp")
list(SORT DSP_FILES)
if(NOT DSP_FILES)
  message(FATAL_ERROR "no .dsp files under ${MPE_INSTRUMENTS_DIR}/instruments (git submodule update --init)")
endif()

set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen")
file(MAKE_DIRECTORY "${GEN_DIR}")
file(GLOB MPE_LIBS "${MPE_INSTRUMENTS_DIR}/lib/*.lib")

set(INSTR_SOURCES "")
set(REG_DECL "")
foreach(rel ${DSP_FILES})
  get_filename_component(grp "${rel}" DIRECTORY)
  get_filename_component(stem "${rel}" NAME_WE)
  set(id "${grp}/${stem}")
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" fn "${grp}_${stem}")
  set(cls "Inst_${fn}")
  set(gen "${GEN_DIR}/${fn}.cpp")
  add_custom_command(
    OUTPUT "${gen}"
    COMMAND ${FAUST_EXECUTABLE} -I "${MPE_INSTRUMENTS_DIR}/lib" -lang cpp -i -cn ${cls} -o "${gen}"
            "${MPE_INSTRUMENTS_DIR}/instruments/${rel}"
    DEPENDS "${MPE_INSTRUMENTS_DIR}/instruments/${rel}" ${MPE_LIBS}
    COMMENT "faust ${id}")
  set(wrap "${GEN_DIR}/${fn}_entry.cpp")
  file(WRITE "${wrap}" "#define GEN \"${gen}\"\n#define CLASS ${cls}\n#define FN ${fn}\n#include \"${CMAKE_CURRENT_SOURCE_DIR}/src/instrument_entry.cpp\"\n")
  set_source_files_properties("${wrap}" PROPERTIES OBJECT_DEPENDS "${gen}"
                              COMPILE_FLAGS "-w -I${FAUST_INCLUDE_DIR}")
  list(APPEND INSTR_SOURCES "${wrap}")
  string(APPEND REG_DECL "dsp* mpe_create_${fn}();\nconst char* mpe_meta_${fn}(const char*);\n")
endforeach()

# Effects from engine/fx/*.dsp: compiled like the instruments, but not in the instrument table.
foreach(fx reverb)
  set(fn "fx_${fx}")
  set(gen "${GEN_DIR}/${fn}.cpp")
  add_custom_command(
    OUTPUT "${gen}"
    COMMAND ${FAUST_EXECUTABLE} -lang cpp -i -cn Fx_${fx} -o "${gen}" "${CMAKE_CURRENT_SOURCE_DIR}/fx/${fx}.dsp"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/fx/${fx}.dsp"
    COMMENT "faust fx/${fx}")
  set(wrap "${GEN_DIR}/${fn}_entry.cpp")
  file(WRITE "${wrap}" "#define GEN \"${gen}\"\n#define CLASS Fx_${fx}\n#define FN ${fn}\n#include \"${CMAKE_CURRENT_SOURCE_DIR}/src/instrument_entry.cpp\"\n")
  set_source_files_properties("${wrap}" PROPERTIES OBJECT_DEPENDS "${gen}"
                              COMPILE_FLAGS "-w -I${FAUST_INCLUDE_DIR}")
  list(APPEND INSTR_SOURCES "${wrap}")
endforeach()

# registry.cpp: the table of instruments. Names come from each .dsp's `declare name`.
set(REG "#include \"mpe/engine.h\"\n#include <string>\n#include <vector>\n#include <cstring>\n#include <cstdlib>\n${REG_DECL}\nnamespace mpe {\n")
string(APPEND REG "static std::vector<InstrumentInfo> build() {\n  std::vector<InstrumentInfo> v;\n")
foreach(rel ${DSP_FILES})
  get_filename_component(grp "${rel}" DIRECTORY)
  get_filename_component(stem "${rel}" NAME_WE)
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" fn "${grp}_${stem}")
  string(APPEND REG "  { const char* n = mpe_meta_${fn}(\"name\"); const char* d = mpe_meta_${fn}(\"description\");\n")
  string(APPEND REG "    v.push_back({ \"${grp}/${stem}\", \"${grp}\", n[0] ? n : \"${stem}\", d, mpe_create_${fn}, mpe_meta_${fn} }); }\n")
endforeach()
string(APPEND REG "  return v;\n}\nconst InstrumentInfo* instruments(int& count) {\n  static const std::vector<InstrumentInfo> all = build();\n  count = (int)all.size();\n  return all.data();\n}\nint findInstrument(const char* id) {\n  int n = 0; const InstrumentInfo* a = instruments(n);\n  for (int i = 0; i < n; i++) if (std::strcmp(a[i].id, id) == 0) return i;\n  return -1;\n}\n}  // namespace mpe\n")
file(WRITE "${GEN_DIR}/registry.cpp.tmp" "${REG}")
configure_file("${GEN_DIR}/registry.cpp.tmp" "${GEN_DIR}/registry.cpp" COPYONLY)

add_library(mpe_engine STATIC src/clean.cpp src/engine.cpp "${GEN_DIR}/registry.cpp" ${INSTR_SOURCES})
target_include_directories(mpe_engine PUBLIC include)
target_include_directories(mpe_engine SYSTEM PUBLIC "${FAUST_INCLUDE_DIR}")
target_compile_options(mpe_engine PRIVATE -Wall -Wextra)
# DSP is always optimised, also in an Android debug build (which is -O0 otherwise: the
# instruments then take several times the CPU and the phone numbers mean nothing).
if(NOT MPE_SANITIZE)
  target_compile_options(mpe_engine PRIVATE -O3)
endif()

option(MPE_BUILD_TESTS "build the tests and tools" ON)
if(MPE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
  add_subdirectory(tools)
endif()
