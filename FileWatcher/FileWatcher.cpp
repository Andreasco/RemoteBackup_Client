//
// Created by cbrugo on 22/09/20.
//

#include "FileWatcher.h"


// Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
void FileWatcher::start(const std::function<void (std::string, FileStatus)> &action) {
    while(running_) {
        // Wait for "delay" milliseconds
        std::this_thread::sleep_for(delay);
        std::cout << "Check" <<std::endl;
        auto it = paths_.begin();

        //std::cout << "Test file/folder order in Filewatcher::start" << std::endl;
        // Check if a file was deleted
        while (it != paths_.end()) {
            std::cout << "File check - " << it->first << std::endl;
            if (!std::filesystem::exists(complete_path(it->first))) {
                action(complete_path(it->first), FileStatus::erased);
                it = paths_.erase(it);
            } else {
                it++;
            }
        }
        //std::cout << "End test on Filewatcher::start" << std::endl;

        // Check if a file was created or modified
        for(auto &file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
            //std::cout << "To check: " << file.path().string() <<std::endl;
            auto current_file_last_write_time = std::filesystem::last_write_time(file);

            // File creation
            if(!contains(relative(file.path().string()))) {
                //std::cout << "new" <<std::endl;
                paths_[relative(file.path().string())] = current_file_last_write_time;
                action(file.path().string(), FileStatus::created);
                // File modification
            } else {
                //std::cout << "not new" <<std::endl;
                if(paths_[relative(file.path().string())] != current_file_last_write_time) {
                    paths_[relative(file.path().string())] = current_file_last_write_time;
                    action(file.path().string(), FileStatus::modified);
                }
            }
        }
        //std::cout << std::endl;
    }
}

void FileWatcher::initial_check(std::map<std::string, std::filesystem::file_time_type> &server, bool server_to_client, const std::function<void (std::string, FileStatus)> &action){

    //Copy directories server -> client
    auto it_client = paths_.begin();


    while (it_client != paths_.end()) {
        //std::cout << "Next to check:  " << it_client->first << std::endl;
        if (!map_contains(server, it_client->first)) {
            //FILE PRESENTE SU CLIENT E NON SU SERVER
            std::cout << "File " << it_client->first << " presente su client e non su server" << std::endl;
            //action(complete_path(it_client->first), FileStatus::erased);
            //it_client = paths_.erase(it_client);   --- solo se si copia server in client
        } else {
            if(it_client->second > server[it_client->first]){
                //FILE PIU' RECENTE SU CLIENT
                std::cout << "File " << it_client->first << " presente su entrambi (più recente su client)" << std::endl;
            } else if (it_client->second < server[it_client->first]){
                //FILE PIU' RECENTE SU SERVER
                std::cout << "File " << it_client->first << " presente su entrambi (più recente su server)" << std::endl;
            } else {
                //FILE IDENTICI
                std::cout << "File " << it_client->first << " presente su entrambi" << std::endl;
            }

        }
        it_client++;
    }

    auto it_server = server.begin();


    while (it_server != server.end()) {
        //FILE PRESENTE SU SERVER E NON SU CLIENT
        if (!contains(it_server->first)) {
            std::cout << "File " << it_server->first << " presente su server e non su client" << std::endl;
            //action(it_client->first, FileStatus::erased);
        }
        it_server++;
    }
}

bool map_contains(std::map<std::string, std::filesystem::file_time_type> &file_map, const std::string &key){
    auto el = file_map.find(key);
    return el != file_map.end();
}