cmake_minimum_required(VERSION 3.16)
project(RemoteBackup_Client)

set(CMAKE_CXX_STANDARD 17)

add_executable(RemoteBackup_Client testMain.cpp FileWatcher/FileWatcher.cpp
        FileWatcher/FileWatcher.h ConnectionAPI/Connection.h ChecksumAPI/SHA256.cpp ChecksumAPI/SHA256.h ChecksumAPI/Checksum.cpp ChecksumAPI/Checksum.h ConnectionAPI/Connection.cpp)

set(Boost_INCLUDE_DIR /usr/local/opt/boost-1.73.0/include)
set(Boost_LIBRARY_DIR /usr/local/opt/boost-1.73.0/lib)
set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
set(OPENSSL_USE_STATIC_LIBS TRUE)

find_package(OpenSSL REQUIRED)
find_package(Boost COMPONENTS serialization filesystem REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

target_link_libraries(RemoteBackup_Client ${Boost_LIBRARIES} OpenSSL::Crypto OpenSSL::SSL)