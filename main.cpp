#include <iostream>

#include "FileWatcher/FileWatcher.h"
#include "ConnectionAPI/Connection.cpp"

void fileWatcherTest();
void socketTest();

int main() {
    int selection;
    std::cout << "Menu options:" << std::endl;

    // Add new options as needed
    std::cout << "0 - fileWatcherTest" << std::endl;
    std::cout << "1 - socketTest" << std::endl;

    std::cout << "Enter selection: ";

    // read the input
    std::cin>>selection;
    switch(selection){
        case 0:
            std::cout << "FileWatcher Test Initialized" << std::endl;
            fileWatcherTest();
            break;
        case 1:
            std::cout << "Socket Test Initialized" << std::endl;
            socketTest();
            break;
        default:
            std::cout << "Error!" << std::endl;
    }
    return 0;
}

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
    Connection s("0.0.0.0", 5005);
    std::cout << s.read(); //Leggo quello che mi arriva appena instauro la connessione
    std::string message;
    //std::cin >> message; //Dovrei leggere da tastiera ma se lo faccio legge solo fino al primo spazio
    message = "login guido guido.poli";
    s.send(message); //FIXME non manda il messaggio
    std::cout << s.read() << std::endl;

    //Per provare l'esempio funzionante
    /*Connection s;
    s.prova("0.0.0.0", 1234);*/
}