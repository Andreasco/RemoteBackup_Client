cmake_minimum_required(VERSION 3.16)
project(RemoteBackup_Client)

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS "-pthread")

add_executable(RemoteBackup_Client testMain.cpp FileWatcher/FileWatcher.cpp
        FileWatcher/FileWatcher.h ConnectionAPI/Connection.cpp
        ChecksumAPI/SHA256.h ChecksumAPI/SHA256.cpp
        ChecksumAPI/Checksum.h ChecksumAPI/Checksum.cpp)

# In order to include Boost libraries
#set(Boost_INCLUDE_DIR /usr/local/opt/boost-1.73.0/include)
#set(Boost_LIBRARY_DIR /usr/local/opt/boost-1.73.0/lib)
find_package(Boost COMPONENTS serialization filesystem)
IF (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
    target_link_libraries(RemoteBackup_Client ${Boost_LIBRARIES})
endif()