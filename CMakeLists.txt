cmake_minimum_required(VERSION 3.16)
project(RemoteBackup_Client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-pthread" )

add_executable( RemoteBackup_Client newMain.cpp
        FileWatcher/FileWatcher.cpp
        FileWatcher/FileWatcher.h
        ConnectionAPI/Connection.h
        ConnectionAPI/Connection.cpp
        ChecksumAPI/SHA256.cpp
        ChecksumAPI/SHA256.h
        ChecksumAPI/Checksum.cpp
        ChecksumAPI/Checksum.h
        RemoteBackup/RemoteBackup.cpp
        RemoteBackup/RemoteBackup.h)


find_package(Boost REQUIRED COMPONENTS serialization filesystem)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(RemoteBackup_Client ${Boost_LIBRARIES})

find_package(OpenSSL REQUIRED)
target_link_libraries(RemoteBackup_Client OpenSSL::SSL)