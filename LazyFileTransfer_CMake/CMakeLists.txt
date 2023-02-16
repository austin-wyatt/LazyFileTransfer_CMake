# CMakeList.txt : CMake project for LazyFileTransfer_CMake, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (LazyFileTransfer "LazyFileTransfer.cpp"  "Source.h" "public.h" "Destination.h" "Source.cpp" "Destination.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET LazyFileTransfer PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
