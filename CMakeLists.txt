cmake_minimum_required(VERSION 3.16)
project(RemoteBackup_Client)

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS -pthread)

add_executable(RemoteBackup_Client testMain.cpp FileWatcher/FileWatcher.cpp
        FileWatcher/FileWatcher.h ConnectionAPI/Connection.cpp)

# In order to include Boost libraries
find_package(Boost)
IF (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
endif()