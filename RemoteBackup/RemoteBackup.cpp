//
// Created by cbrugo on 14/01/21.
//

#include "../FileWatcher/FileWatcher.h"
#include "../ConnectionAPI/Connection.h"
#include "../ChecksumAPI/Checksum.h"

void RemoteBackup(){
    std::cout << "Enter path to watch: ";
    std::string path_to_watch;
    std::getline(std::cin, path_to_watch);

    std::cout << "Watching path " << path_to_watch << std::endl;

    // Create a Connection
    std::cout << "LOGIN REQUESTED" << std::endl;
    std::string username, password;

    std::cout << "Username: ";
    std::getline(std::cin, username);

    std::cout << "Password: ";
    std::getline(std::cin, password);


    try {
        Connection conn_("0.0.0.0", 5004, path_to_watch);
        while (!conn_.login(username, password)) {
            std::cout << "[ERROR] Incorrect username or password" << std::endl;
            std::cout << "Username: ";
            std::getline(std::cin, username);

            std::cout << "Password: ";
            std::getline(std::cin, password);
        }
        std::cout << "Login successful!" << std::endl;


        if (DEBUG)
            std::cout << "[DEBUG] Filewatcher Object Initialization" << std::endl;

        // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
        FileWatcher fw{path_to_watch, std::chrono::milliseconds(5000), conn_};
        if (DEBUG)
            std::cout << "[DEBUG] Filewatcher Object Initialized" << std::endl;

        fw.initial_check([](const std::string &file_path, Connection &conn_, FileStatus status) -> void {
            // Process only regular files, all other file types are ignored

            if (!std::filesystem::is_regular_file(std::filesystem::path(file_path)) && status != FileStatus::missing) {
                return;
            }

            switch (status) {
                case FileStatus::created:
                    conn_.add_file(file_path);
                    std::cout << "File added to server: " << file_path << '\n';
                    break;
                case FileStatus::modified:
                    conn_.update_file(file_path);
                    std::cout << "File updated on server: " << file_path << '\n';
                    break;
                case FileStatus::missing:
                    conn_.get_file(file_path);
                    std::cout << "File retreived from server: " << file_path << '\n';
                    break;
                default:
                    std::cout << "Error! Unknown file status.\n";
            }
        });

        if (DEBUG)
            std::cout << "[DEBUG] Filewatcher Initial Check Completed" << std::endl;

        // Start monitoring a folder for changes and (in case of changes)
        // run a user provided lambda function
        fw.start([](const std::string &file_path, Connection &conn_, FileStatus status) -> void {
            // Process only regular files, all other file types are ignored

            if (!std::filesystem::is_regular_file(std::filesystem::path(file_path)) && status != FileStatus::erased) {
                return;
            }
            switch (status) {
                case FileStatus::created:
                    conn_.add_file(file_path);
                    std::cout << "File created: " << file_path << '\n';
                    break;
                case FileStatus::modified:
                    conn_.update_file(file_path);
                    std::cout << "File modified: " << file_path << '\n';
                    break;
                case FileStatus::erased:
                    conn_.remove_file(file_path);
                    std::cout << "File erased: " << file_path << '\n';
                    break;
                default:
                    std::cout << "Error! Unknown file status.\n";
            }
        });
    } catch(std::exception &e){
        if(DEBUG) {
            std::cout << "[ERROR] Runtime error: " << e.what() << std::endl;
        }
        std::cout << "A fatal error occurred, please try again." << std::endl;
    }
}