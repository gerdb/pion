# Find all source files in a single current directory
# Save the name to DIR_pics_SRCS
aux_source_directory(. DIR_pics_SRCS)

# Generate the link library
add_library(pics ${DIR_pics_SRCS})
target_link_libraries(pics PUBLIC)