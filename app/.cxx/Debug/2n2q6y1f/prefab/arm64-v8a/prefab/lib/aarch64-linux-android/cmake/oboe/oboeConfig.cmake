if(NOT TARGET oboe::oboe)
add_library(oboe::oboe SHARED IMPORTED)
set_target_properties(oboe::oboe PROPERTIES
    IMPORTED_LOCATION "/home/patrick-reynolds/.gradle/caches/8.9/transforms/c497d726db2938f1f0cde736cf8e39b6/transformed/oboe-1.10.0/prefab/modules/oboe/libs/android.arm64-v8a/liboboe.so"
    INTERFACE_INCLUDE_DIRECTORIES "/home/patrick-reynolds/.gradle/caches/8.9/transforms/c497d726db2938f1f0cde736cf8e39b6/transformed/oboe-1.10.0/prefab/modules/oboe/include"
    INTERFACE_LINK_LIBRARIES ""
)
endif()

