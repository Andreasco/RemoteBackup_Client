//
// Created by cbrugo on 22/09/20.
//

#ifndef REMOTEBACKUP_CLIENT_FILEWATCHER_H
#define REMOTEBACKUP_CLIENT_FILEWATCHER_H

#include <iostream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <map>
#include <string>
#include <functional>
#include "../ConnectionAPI/Connection.cpp"

// Define available file changes
enum class FileStatus {created, modified, erased};

bool map_contains(std::map<std::string, std::filesystem::file_time_type> &file_map, const std::string &key);

class FileWatcher {
public:
    std::string path_to_watch;
    // Time interval at which check for changes
    std::chrono::duration<int, std::milli> delay;

    // Keep a record of files from the base directory and their last modification time
    FileWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay, Connection& conn_) : path_to_watch{path_to_watch}, delay{delay}, conn_(conn_) {
        //std::cout << "Test file/folder order in Filewatcher constructor" << std::endl;
        for(auto &file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
            //std::cout << "File - " << file.path().string() << std::endl;
            //std::cout << "File (rel) - " << relative(file.path().string()) << std::endl;
            paths_[relative(file.path().string())] = std::filesystem::last_write_time(file);
        }
        //std::cout << "End test on Filewatcher constructor" << std::endl;
    }

    void start(const std::function<void (std::string, Connection&, FileStatus)> &action);
    void initial_check(std::map<std::string, std::filesystem::file_time_type> &server, bool server_to_client, const std::function<void (std::string, FileStatus)> &action);

    //for tests
    std::map<std::string, std::filesystem::file_time_type> get_map(){
        return paths_;
    }

private:
    Connection &conn_;
 std::map<std::string, std::filesystem::file_time_type> paths_;
    bool running_ = true;

    // Check if "paths_" contains a given key
    // If the compiler supports C++20 use paths_.contains(key) instead of this function
    bool contains(const std::string &key) {
        auto el = paths_.find(key);
        return el != paths_.end();
    }

    std::string complete_path(const std::string &rel_path){
        return path_to_watch + "/" + rel_path;
    }

    std::string relative(const std::string path){
        return path.substr(path_to_watch.length()+1, path.length()-path_to_watch.length()-1);
    }
};


#endif //REMOTEBACKUP_CLIENT_FILEWATCHER_H
