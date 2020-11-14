#include <iostream>

#include "FileWatcher/FileWatcher.h"
#include "ConnectionAPI/Connection.cpp"

void fileWatcherTest () {
    std::cout << "Enter path to watch: ";
    std::string path_to_watch;
    std::cin>>path_to_watch;

    // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
    FileWatcher fw{path_to_watch, std::chrono::milliseconds(5000)};

    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    fw.start([] (std::string path_to_watch, FileStatus status) -> void {
        // Process only regular files, all other file types are ignored
        if(!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased) {
            return;
        }

        switch(status) {
            case FileStatus::created:
                std::cout << "File created: " << path_to_watch << '\n';
                break;
            case FileStatus::modified:
                std::cout << "File modified: " << path_to_watch << '\n';
                break;
            case FileStatus::erased:
                std::cout << "File erased: " << path_to_watch << '\n';
                break;
            default:
                std::cout << "Error! Unknown file status.\n";
        }
    });
}

void socketTest(){
    Connection s("0.0.0.0", 5007);
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

void sendFileTest(){
    Connection s("0.0.0.0", 1234);
    std::string input;
    while (input != "y" && input != "n") {
        std::cout << "Do you want to send the file now?(y/n): ";
        std::getline(std::cin, input);
        if (input == "y") {
            s.send_file("/Users/andreascopp/Desktop/Client-TestFiles/invio_client_grande.txt");
        }
        else if (input == "n"){
            std::cout << "Ok bye!" << std::endl;
        }
        else
            std::cout << "Enter a valid selection!" << std::endl;
    }
}

void readFileTest(){
    Connection s("0.0.0.0", 1234);
    std::string input;
    s.read_file();
}

int main() {
    std::cout << "Menu options:" << std::endl;

    // Add new options as needed
    std::cout << "0 - fileWatcherTest" << std::endl;
    std::cout << "1 - socketTest" << std::endl;
    std::cout << "2 - sendFileTest" << std::endl;
    std::cout << "3 - readFileTest" << std::endl;

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
            std::cout << "Socket Test Initialized" << std::endl;
            socketTest();
            break;
        case 2:
            std::cout << "Send File Test Initialized" << std::endl;
            sendFileTest();
            break;
        case 3:
            std::cout << "Read File Test Initialized" << std::endl;
            readFileTest();
            break;
        default:
            std::cout << "Error!" << std::endl;
    }
    return 0;
}