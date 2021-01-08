#include <iostream>

#include "FileWatcher/FileWatcher.h"
#include "ConnectionAPI/Connection.h"
#include "ChecksumAPI/Checksum.h"

void fileWatcherTest () {
    std::cout << "Enter path to watch: ";
    std::string path_to_watch;
    std::cin >> path_to_watch;

    //path_to_watch.erase(path_to_watch.length()-1);  To remove \n
    std::cout << "Watching path " << path_to_watch << std::endl;

    // Create a Connection
    //Connection conn_("0.0.0.0", 5004, "/Users/andreascopp/Desktop/Client-TestFiles");
    Connection conn_("0.0.0.0", 5004, path_to_watch);
    conn_.login("guido", "guido.poli");
    std::cout << conn_.read_string(); // Read server confirm G.R.

    if(DEBUG)
        std::cout << "[DEBUG] Filewatcher Object Initialization" << std::endl;

    // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
    FileWatcher fw{path_to_watch, std::chrono::milliseconds(5000), conn_};
    if(DEBUG)
        std::cout << "[DEBUG] Filewatcher Object Initialized" << std::endl;

    fw.initial_check([] (const std::string& file_path, Connection& conn_, FileStatus status) -> void {
        // Process only regular files, all other file types are ignored

        if(!std::filesystem::is_regular_file(std::filesystem::path(file_path)) && status!=FileStatus::missing) {
            return;
        }

        switch(status) {
            case FileStatus::created:
                conn_.add_file(file_path);
                std::cout << "File created: " << file_path << '\n';
                break;
            case FileStatus::modified:
                conn_.update_file(file_path);
                std::cout << "File modified: " << file_path << '\n';
                break;
            case FileStatus::missing:
                conn_.get_file(file_path);
                std::cout << "File retreived: " << file_path << '\n';
                break;
            default:
                std::cout << "Error! Unknown file status.\n";
        }
    });

    if(DEBUG)
        std::cout << "[DEBUG] Filewatcher Initial Check Completed" << std::endl;

    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    fw.start([] (const std::string& file_path, Connection& conn_, FileStatus status) -> void {
        // Process only regular files, all other file types are ignored

        if(!std::filesystem::is_regular_file(std::filesystem::path(file_path)) && status!=FileStatus::erased) {
            return;
        }
        switch(status) {
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
}

void sendStringTest(){
    Connection s("0.0.0.0", 5004, "/Users/andreascopp/Desktop/Client-TestFiles");
    std::cout << s.read_string();
    std::string message;
    while(message != "stop") {
        std::getline(std::cin, message);
        s.send_string(message);
        std::cout << s.read_string();
    }
}

void getFileTest(){
    Connection s("0.0.0.0", 5004, "/Users/andreascopp/Desktop/Client-TestFiles");
    std::string input;
    std::cout << "Do you want to login now?(y/n): ";
    std::getline(std::cin, input);
    if (input == "y") {
        s.login("guido", "guido.poli");
        std::cout << s.read_string(); // Read server confirm

        std::cout << "Do you want to read the file now?(y/n): ";
        std::getline(std::cin, input);
        if (input == "y") {
            s.get_file("/Appunti_di_Topologia_(Ferrarotti).pdf");

            s.close_connection();
        }
        else if (input == "n"){
            std::cout << "Ok bye!" << std::endl;
        }
    }
}

void checksumTest(){
    //std::string checksum = get_file_checksum("/Users/andreascopp/Desktop/Client-TestFiles/invio_client.txt");
    std::string checksum = get_file_checksum("/Users/andreascopp/Desktop/Client-TestFiles/invio_client.txt");
    std::cout << "File checksum: " << checksum << std::endl;
}

void addFileTest(){
    Connection s("0.0.0.0", 5004, "/Users/andreascopp/Desktop/Client-TestFiles");

    std::string input;
    std::cout << "Do you want to login now?(y/n): ";
    std::getline(std::cin, input);
    if (input == "y") {
        s.login("guido", "guido.poli");
        std::cout << s.read_string(); // Read server confirm

        std::cout << "Do you want to send the file now?(y/n): ";
        std::getline(std::cin, input);
        if (input == "y") {
            s.add_file("/invio_client.txt");
            //s.add_file("/Users/andreascopp/Desktop/Client-TestFiles/invio_client2.txt");

            // In order to wait the sending of the files, since they are made by different threads
            //std::this_thread::sleep_for(std::chrono::seconds(5));
            s.close_connection();
        }
        else if (input == "n"){
            std::cout << "Ok bye!" << std::endl;
        }
    }
}

void getFilesystemStatusTest(){
    Connection s("0.0.0.0", 5004, "/Users/andreascopp/Desktop/Client-TestFiles");

    std::string input;
    std::cout << "Do you want to login now?(y/n): ";
    std::getline(std::cin, input);
    if (input == "y") {
        s.login("guido", "guido.poli");
        std::cout << s.read_string(); // Read server confirm

        std::cout << "Do you want to get the filesystem status now?(y/n): ";
        std::getline(std::cin, input);
        if (input == "y") {
            std::unordered_map<std::string, std::string> m = s.get_filesystem_status();

            // Print the filesystem
            std::for_each(m.begin(),
                          m.end(),
                          [](const std::pair<std::string, std::string> &p) {
                              std::cout << "{" << p.first << ": " << p.second << "}\n";
                          });

            s.close_connection();
        }
        else if (input == "n"){
            std::cout << "Ok bye!" << std::endl;
        }
    }
}

void prova(){
    std::string file_path = "/Users/andreascopp/Desktop/Client-TestFiles/fol/fol2/prova.txt";

    std::string path_to_directory(file_path.substr(0, file_path.find_last_of('/')));
    std::filesystem::create_directories(path_to_directory);
}

int main() {
    std::cout << "Menu options:" << std::endl;

    // Add new options as needed
    std::cout << "0 - fileWatcherTest" << std::endl;
    std::cout << "1 - sendStringTest" << std::endl;
    std::cout << "2 - getFileTest" << std::endl;
    std::cout << "3 - checksumTest" << std::endl;
    std::cout << "4 - addFileTest" << std::endl;
    std::cout << "5 - getFilesystemStatusTest" << std::endl;

    std::cout << "Enter selection: ";

    // Read the input
    std::string input;
    std::getline(std::cin, input);
    int selection = std::stoi(input);
    switch(selection){
        case 0:
            std::cout << "FileWatcher Test Initialized" << std::endl;
            fileWatcherTest();
            break;
        case 1:
            std::cout << "Send String Test Initialized" << std::endl;
            sendStringTest();
            break;
        case 2:
            std::cout << "Get File Test Initialized" << std::endl;
            getFileTest();
            break;
        case 3:
            std::cout << "Checksum Test Initialized" << std::endl;
            checksumTest();
            break;
        case 4:
            std::cout << "Add File Test Initialized" << std::endl;
            addFileTest();
            break;
        case 5:
            std::cout << "Get Filesystem Status Test Initialized" << std::endl;
            getFilesystemStatusTest();
            break;
        case 6:
            prova();
            break;
        default:
            std::cout << "Error!" << std::endl;
    }
    return 0;
}