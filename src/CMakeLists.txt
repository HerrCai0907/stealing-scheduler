aux_source_directory(. stealing_scheduler_lib_srcs)

add_library(stealing-scheduler
    ${stealing_scheduler_lib_srcs}
)

target_include_directories(stealing-scheduler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
