#include <iostream>

#include "FileWatcher/FileWatcher.h"
#include "ConnectionAPI/Connection.h"
#include "ChecksumAPI/Checksum.h"

void fileWatcherTest () {
    std::cout << "Enter path to watch: ";
    std::string path_to_watch;
    std::cin >> path_to_watch;

    // Create a Connection
    Connection conn_("0.0.0.0", 5004, path_to_watch);
    conn_.send_string("login guido guido.poli");
    std::cout << conn_.read_string(); // Read server confirm G.R.

    //conn_.read_string();


    if(DEBUG)
        std::cout << "[DEBUG] Filewatcher Object Initialization" << std::endl;

    // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
    FileWatcher fw{path_to_watch, std::chrono::milliseconds(5000), conn_};
    if(DEBUG)
        std::cout << "[DEBUG] Filewatcher Object Initialized" << std::endl;

    fw.initial_check([] (const std::string& file_path, Connection& conn_, FileStatus status) -> void {
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
    Connection s("0.0.0.0", 5007, "/Users/andreascopp/Desktop/Client-TestFiles/");
    std::cout << s.read_string(); //Leggo quello che mi arriva appena instauro la connessione
    std::string message;
    while(message != "stop") {
        std::getline(std::cin, message);
        s.send_string(message);
        std::cout << s.read_string();
    }

    //Per usare il metodo prova col server di prova
    /*Connection s;
    s.prova("0.0.0.0", 1234);*/
}

void readFileTest(){
    Connection s("0.0.0.0", 1234, "/Users/andreascopp/Desktop/Client-TestFiles/");
    std::string input;
    s.read_file();
}

void checksumTest(){
    //std::string checksum = get_file_checksum("/Users/andreascopp/Desktop/Client-TestFiles/invio_client.txt");
    std::string checksum = get_file_checksum("/Users/andreascopp/Desktop/Client-TestFiles/invio_client.txt");
    std::cout << "File checksum: " << checksum << std::endl;
}

void addFileTest(){
    std::string base_path_ = "/Users/andreascopp/Desktop/Client-TestFiles/";
    Connection s("0.0.0.0", 5004, base_path_);

    //std::cout << s.read_string() << std::endl;

    std::string input;
    std::cout << "Do you want login now?(y/n): ";
    std::getline(std::cin, input);
    if (input == "y") {
        s.send_string("login guido guido.poli");

        std::cout << "Do you want to send the file now?(y/n): ";
        std::getline(std::cin, input);
        if (input == "y") {
            s.add_file("/Users/andreascopp/Desktop/Client-TestFiles/invio_client.txt");
            //s.add_file("/Users/andreascopp/Desktop/Client-TestFiles/invio_client2.txt");

            // In order to wait the sending of the files, since they are made by different threads
            std::this_thread::sleep_for(std::chrono::seconds(5));
            s.send_string("close");
        }
        else if (input == "n"){
            std::cout << "Ok bye!" << std::endl;
        }
    }
}

void getFilesystemStatusTest(){
    std::string base_path_ = "/Users/andreascopp/Desktop/Client-TestFiles/";
    Connection s("0.0.0.0", 5004, base_path_);

    //std::cout << s.read_string() << std::endl;

    std::string input;
    std::cout << "Do you want login now?(y/n): ";
    std::getline(std::cin, input);
    if (input == "y") {
        s.send_string("login guido guido.poli");
        std::cout << s.read_string(); // Read server confirm G.R.

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

            // In order to wait the sending of the files, since they are made by different threads
            std::this_thread::sleep_for(std::chrono::seconds(5));
            s.send_string("close");
        }
        else if (input == "n"){
            std::cout << "Ok bye!" << std::endl;
        }
    }
}

int main() {
    std::cout << "Menu options:" << std::endl;

    // Add new options as needed
    std::cout << "0 - fileWatcherTest" << std::endl;
    std::cout << "1 - sendStringTest" << std::endl;
    std::cout << "2 - readFileTest" << std::endl;
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
            std::cout << "Read File Test Initialized" << std::endl;
            readFileTest();
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
        default:
            std::cout << "Error!" << std::endl;
    }
    return 0;
}