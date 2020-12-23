//
// Created by cbrugo on 22/09/20.
//

#include "FileWatcher.h"


// Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
void FileWatcher::start(const std::function<void (std::string, Connection&, FileStatus)> &action) {
    while(running_) {
        // Wait for "delay" milliseconds
        std::this_thread::sleep_for(delay);
        //std::cout << "Check" <<std::endl;
        auto it = paths_.begin();

        //std::cout << "Test file/folder order in Filewatcher::start" << std::endl;
        // Check if a file was deleted
        while (it != paths_.end()) {
            //std::cout << "File check - " << it->first << std::endl;
            if (!std::filesystem::exists(complete_path(it->first))) {
                action(complete_path(it->first), conn_, FileStatus::erased);
                it = paths_.erase(it);
            } else {
                it++;
            }
        }
        //std::cout << "End test on Filewatcher::start" << std::endl;

        // Check if a file was created or modified
        for(auto &file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
            //std::cout << "To check: " << file.path().string() <<std::endl;
            auto current_file_checksum = get_file_checksum(file.path().string());

            // File creation
            if(!contains(relative(file.path().string()))) {
                //std::cout << "new" <<std::endl;
                paths_[relative(file.path().string())] = current_file_checksum;
                action(file.path().string(), conn_, FileStatus::created);
                // File modification
            } else {
                //std::cout << "not new" <<std::endl;
                if(!checksums_equal(paths_[relative(file.path().string())], current_file_checksum)) {
                    paths_[relative(file.path().string())] = current_file_checksum;
                    action(file.path().string(), conn_, FileStatus::modified);
                }
            }
        }
        //std::cout << std::endl;
    }
}

void FileWatcher::initial_check(const std::function<void (std::string, Connection&, FileStatus)> &action){

    std::unordered_map<std::string, std::string> server = conn_.get_filesystem_status();
    //std::unordered_map<std::string, std::string> server = std::unordered_map<std::string, std::string>();
    if(DEBUG)
        std::cout << "[DEBUG] [FileWatcher] FileSystemStatus received" << std::endl;
    //Copy directories server -> client
    auto it_client = paths_.begin();


    while (it_client != paths_.end()) {
        //std::cout << "Next to check:  " << it_client->first << std::endl;
        if (!map_contains(server, it_client->first)) {
            //FILE PRESENTE SU CLIENT E NON SU SERVER
            if(DEBUG)
                std::cout << "[DEBUG] [FileWatcher] File " << it_client->first << " present on client and not on server" << std::endl;
            action(complete_path(it_client->first), conn_, FileStatus::created);
        } else {
            if(!checksums_equal(it_client->second, server[it_client->first])){
                //FILE CON INFO DIVERSE
                if(DEBUG)
                    std::cout << "[DEBUG] [FileWatcher] File " << it_client->first << " present on both (but different content)" << std::endl;
                action(complete_path(it_client->first), conn_, FileStatus::modified);
            } else {
                //FILE IDENTICI
                if(DEBUG)
                    std::cout << "[DEBUG] [FileWatcher] File " << it_client->first << " present on both" << std::endl;
            }

        }
        it_client++;
    }

    auto it_server = server.begin();


    while (it_server != server.end()) {
        //FILE PRESENTE SU SERVER E NON SU CLIENT
        if (!contains(it_server->first)) {
            if(DEBUG)
                std::cout << "[DEBUG] [FileWatcher] File " << it_server->first << " present on server and not on client" << std::endl;
            action(it_client->first, conn_, FileStatus::erased);
        }
        it_server++;
    }
}

bool map_contains(std::unordered_map<std::string, std::string> &file_map, const std::string &key){
    auto el = file_map.find(key);
    return el != file_map.end();
}