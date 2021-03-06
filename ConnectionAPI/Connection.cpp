//
// Created by Andrea Scoppetta on 09/12/20.
//

#include "Connection.h"

/******************* CONSTRUCTOR **************************************************************************************/

Connection::Connection(std::string ip_address, int port_number, std::string base_path) try:
    io_context_(),
    server_ip_address_(std::move(ip_address)),
    server_port_number_(port_number),
    base_path_(std::move(base_path)),
    closed_(false) {
        main_socket_ = std::make_shared<tcp::socket>(io_context_);
        main_socket_->connect(tcp::endpoint(boost::asio::ip::address::from_string(server_ip_address_), server_port_number_));
    }
    catch(std::exception& e) {
        if(DEBUG) {
            std::cout << "[ERROR] Connection initialization error: " << e.what() << std::endl;
        }
        std::cout << "Could not initialize client, please try again." << std::endl;
        throw e;
    }

/******************* DESTRUCTOR ***************************************************************************************/

Connection::~Connection() {
    if(!closed_) {
        close_connection(false);
    }
}

/******************* ERROR HANDLING ***********************************************************************************/

void Connection::handle_add_file_error(const std::string &file_path){
    try{ //for open_new_connection
        close_connection(false);
        open_new_connection();
        add_file(file_path);
    }
    catch(std::exception &e){
        if(DEBUG) {
            std::cout << "[ERROR] Open new connection error: " << e.what() << std::endl;
        }
        std::cout << "The server is still down, I can't add the file right now." << std::endl;
        close_connection(false);
    }
}

void Connection::handle_update_file_error(const std::string &file_path){
    try{ //for open_new_connection
        close_connection(false);
        open_new_connection();
        update_file(file_path);
    }
    catch(std::exception &e){
        if(DEBUG) {
            std::cout << "[ERROR] Open new connection error: " << e.what() << std::endl;
        }
        std::cout << "The server is still down, I can't update the file right now." << std::endl;
        close_connection(false);
    }
}

void Connection::handle_remove_file_error(const std::string &file_path){
    try{ //for open_new_connection
        close_connection(false);
        open_new_connection();
        remove_file(file_path);
    }
    catch(std::exception &e){
        if(DEBUG) {
            std::cout << "[ERROR] Open new connection error: " << e.what() << std::endl;
        }
        std::cout << "The server is still down, I can't remove the file right now." << std::endl;
        close_connection(false);
    }
}

void Connection::handle_get_file_error(const std::string &file_path){
    try{ //for open_new_connection
        close_connection(false);
        open_new_connection();
        get_file(file_path);
    }
    catch(std::exception &e){
        if(DEBUG) {
            std::cout << "[ERROR] Open new connection error: " << e.what() << std::endl;
        }
        std::cout << "The server is still down, I can't get the file right now." << std::endl;
        close_connection(false);
    }
}

/******************* UTILITY METHODS **********************************************************************************/

void Connection::print_percentage(float percent) {
    float hashes = percent / 5;
    float spaces = std::ceil(100.0 / 5 - percent / 5);
    std::cout << "\r" << "[" << std::string(hashes, '#') << std::string(spaces, ' ') << "]";
    std::cout << " " << percent << "%";
    std::cout.flush();
}

std::string Connection::file_size_to_readable(int file_size) {
    std::string measures[] = {"bytes", "KB", "MB", "GB", "TB"};
    int i = 0;
    int new_file_size = file_size;
    while (new_file_size > 1024){ // NOTE on Mac is 1000, it uses powers of 10
        new_file_size = new_file_size / 1024;
        i++;
    }
    return std::to_string(new_file_size) + " " + measures[i];
}

void Connection::open_new_connection(){
    main_socket_ = std::make_unique<tcp::socket>(io_context_);
    main_socket_->connect(tcp::endpoint(boost::asio::ip::address::from_string(server_ip_address_), server_port_number_));
    closed_ = false;

    login(username_, password_);
}

void Connection::close_connection(bool comunicate_to_the_server) {
    handle_close_connection(main_socket_, comunicate_to_the_server);
}

/*void Connection::close_connection(const std::shared_ptr<tcp::socket> &socket) {
    handle_close_connection(socket);
}*/

void Connection::handle_close_connection(const std::shared_ptr<tcp::socket> &socket, bool comunicate_to_the_server){
    if(comunicate_to_the_server) {
        const std::string message = "close";
        send_string(message);
    }
    socket->close();
    closed_ = true;
}

/******************* STRINGS METHODS **********************************************************************************/

std::string Connection::read_string() {
    return handle_read_string(main_socket_);
}

/*std::string Connection::read_string(const std::shared_ptr<tcp::socket> &socket) {
    return handle_read_string(socket);
}*/

std::string Connection::handle_read_string(const std::shared_ptr<tcp::socket> &socket) {
    boost::asio::streambuf buf;
    boost::asio::read_until(*socket, buf, "\n");

    std::string data = boost::asio::buffer_cast<const char*>(buf.data());

    if(DEBUG) {
        std::cout << "[DEBUG] Client received: " + data << std::endl;
    }

    return data;
}

void Connection::send_string(const std::string &message) {
    handle_send_string(main_socket_, message);
}

/*void Connection::send_string(const std::shared_ptr<tcp::socket> &socket, const std::string &message) {
    handle_send_string(socket, message);
}*/

void Connection::handle_send_string(const std::shared_ptr<tcp::socket> &socket, const std::string &message) {
    if(DEBUG) {
        std::cout << "[DEBUG] Sending string: " << message << std::endl;
    }

    const std::string msg = message + "\n";
    boost::asio::write(*socket, boost::asio::buffer(msg));

    if(DEBUG) {
        std::cout << "[DEBUG] Client sent: " << message << std::endl;
    }
}

/******************* LOGIN ********************************************************************************************/

bool Connection::login(const std::string &username, const std::string &password){
    // If I'm logging in, I need to save the credentials to reopen a connection in case of errors
    username_ = username;
    password_ = password;

    std::ostringstream oss;
    oss << "login ";
    oss << username;
    oss << " ";
    oss << password;

    try {
        send_string(oss.str());
        std::string response = read_string();
        return response.find("[SERVER_SUCCESS]") != std::string::npos;
    } catch (std::exception &e) {
        if(DEBUG)
            std::cout << "[ERROR] Login error: " << e.what() << std::endl;
        std::cout << "There was an error with the server, I'll try to login again in 10 seconds." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        open_new_connection();
    }
}

/******************* FILES METHODS ************************************************************************************/

void Connection::remove_file(const std::string &file_path) {
    try {
        std::string cleaned_file_path = file_path.substr(base_path_.length(), file_path.length());

        std::ostringstream oss;
        oss << "removeFile ";
        oss << cleaned_file_path;
        //oss << "\n";
        send_string(oss.str());

        // Read the confirm of command receipt from the server
        //std::cout << read_string() << std::endl;

        // Read the confirm of receipt from the server
        //std::cout << read_string() << std::endl;
    } catch (std::exception &e){
        if(DEBUG)
            std::cout << "[ERROR] Remove file error: " << e.what() << std::endl;
        std::cout << "There was an error with the server. I'll try to remove the file again in 10 seconds." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        handle_remove_file_error(file_path);
    }
}

void Connection::update_file(const std::string &file_path) {
    try {
        handle_send_file(file_path, "updateFile");
    }
    catch (std::runtime_error& e){
        if(DEBUG) {
            std::cout << "[ERROR] Update file error: " << e.what() << std::endl;
        }
        std::cout << e.what() << " I'm trying to recover in 10 seconds." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        handle_update_file_error(file_path);
    }
    catch (std::exception& e){
        if(DEBUG) {
            std::cout << "[ERROR] Update file error: " << e.what() << std::endl;
        }
        std::cout << "There was an error with the server, I'll try to update the file again in 10 seconds." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        handle_update_file_error(file_path);
    }
}

void Connection::add_file(const std::string &file_path) {
    try {
        handle_send_file(file_path, "addFile");
    }
    catch (std::runtime_error& e){
        if(DEBUG) {
            std::cout << "[ERROR] Add file error: " << e.what() << std::endl;
        }
        std::cout << e.what() << "There was an error, I'm trying to recover in 10 seconds." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        handle_add_file_error(file_path);
    }
    catch (std::exception& e){
        if(DEBUG) {
            std::cout << "[ERROR] Add file error: " << e.what() << std::endl;
        }
        std::cout << "There was an error with the server, I'll try to send the file again in 10 seconds." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        handle_add_file_error(file_path);
    }
}

void Connection::handle_send_file(const std::string &file_path, const std::string &command) {
    // Open the file to send
    std::shared_ptr<std::ifstream> source_file = std::make_shared<std::ifstream>(file_path, std::ios_base::binary | std::ios_base::ate);

    if(!source_file->is_open()) {
        std::cout << "[ERROR] Failed to open " << file_path << std::endl;

        // Minor error handling
        std::string input;
        do {
            std::cout << "Do you want to try to open it again? (y/n)" << std::endl;
            std::getline(std::cin, input);
            if (input == "y"){
                for(int i = 0; i < 5; i++) {
                    source_file = std::make_shared<std::ifstream>(file_path,
                                                                  std::ios_base::binary | std::ios_base::ate);
                    if(source_file->is_open())
                        break;
                }

                if(!source_file->is_open()) {
                    std::cout << "I tried to open it again but I couldn't, this file won't be included in the backup\";"
                              << std::endl;
                    return;
                }
            }
            else if (input == "n") {
                std::ostringstream oss;
                oss << "Ok, the file: ";
                oss << file_path;
                oss << " won't be included in the backup";
                std::cout << oss.str() << std::endl;

                return;
            }
        } while (input != "y" && input != "n");
    }

    std::string cleaned_file_path = file_path.substr(base_path_.length(), file_path.length());
    size_t file_size = source_file->tellg();
    std::string file_size_readable = file_size_to_readable(file_size);

    if(DEBUG) {
        std::cout << "[DEBUG] " << file_path << " size is: " << file_size_readable << std::endl;
        std::cout << "[DEBUG] Cleaned file path: " << cleaned_file_path << std::endl;
    }

    std::ostringstream oss;
    oss << command + " ";
    oss << cleaned_file_path;
    oss << " ";
    oss << file_size;
    send_string(oss.str());

    // Read the confirm of command receipt from the server
    std::cout << read_string() << std::endl;

    do_send_file(source_file); // May throw runtime_error

    source_file->close();

    // Read the confirm of file receipt from the server
    std::cout << read_string() << std::endl;

    //std::cout << "\n" << "[INFO] File " << file_path << " sent successfully!" << std::endl;
    std::cout << "\n[INFO] File " << file_path << " sent successfully!" << std::endl;
}

void Connection::do_send_file(const std::shared_ptr<std::ifstream>& source_file) {
    // This buffer's size determines the MTU, therefore how many bytes at time get copied
    // from source file to such buffer, which means how many bytes get sent everytime
    boost::array<char, 1024> buf{};
    boost::system::error_code error;

    size_t file_size = source_file->tellg();
    source_file->seekg(0);

    if(DEBUG) {
        std::cout << "[DEBUG] Start sending file content" << std::endl;
    }

    long bytes_sent = 0;
    float percent = 0;
    print_percentage(percent);

    while(!source_file->eof()) {
        source_file->read(buf.c_array(), (std::streamsize)buf.size());

        int bytes_read_from_file = source_file->gcount(); //int is fine because i read at most buf's size, 1024 in this case

        // If uncommented the progress is printed in multiple lines succeded from this line
        // Use only to debug file reading otherwise the progress gets unreadable
        /*if(DEBUG){
            std::cout << "[DEBUG] Bytes read from file: " << bytes_read_from_file << std::endl;
        }*/

        if(bytes_read_from_file<=0) {
            //std::cout << "[ERROR] Read file to send error" << std::endl;
            throw std::runtime_error("Read file to send error.");
        }

        boost::asio::write(*main_socket_, boost::asio::buffer(buf.c_array(), source_file->gcount()),
                           boost::asio::transfer_all(), error);
        if(error) {
            //std::cout << "[ERROR] Send file error: " << error << std::endl;
            throw std::runtime_error("Send file error.");
        }

        bytes_sent += bytes_read_from_file;

        percent = std::ceil((100.0 * bytes_sent) / file_size);
        print_percentage(percent);
    }
}

void Connection::get_file(const std::string &file_path){
    try{
        handle_get_file(file_path);
    }
    catch (std::exception& e){
        if(DEBUG) {
            std::cout << "[ERROR] Save file error: " << e.what() << std::endl;
        }
        std::cout << "There was an error with the server, I'll try to retreive the file again in 10 seconds" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        handle_get_file_error(file_path);
    }
}

void Connection::handle_get_file(const std::string &file_path) {
    std::string cleaned_file_path = file_path.substr(base_path_.length(), file_path.length());
    std::string path_to_directory(file_path.substr(0, file_path.find_last_of('/')));

    if(DEBUG) {
        std::cout << "[DEBUG] Cleaned file path: " << cleaned_file_path << std::endl;
        std::cout << "[DEBUG] File path: " << file_path << std::endl;
        std::cout << "[DEBUG] Path to directory: " << path_to_directory << std::endl;
    }

    std::filesystem::create_directories(path_to_directory);

    // Open the file to save
    std::shared_ptr<std::ofstream> output_file = std::make_shared<std::ofstream>(file_path, std::ios_base::binary);
    if(!output_file) {
        //std::cout << "[ERROR] Failed to open " << file_path << std::endl;
        std::cout << "[ERROR] Failed to save " << file_path << std::endl;

        // Minor error handling
        std::string input;
        do {
            std::cout << "Do you want to try to save it again? (y/n)" << std::endl;
            std::getline(std::cin, input);
            if (input == "y") {
                for(int i = 0; i < 5; i++) {
                    output_file = std::make_shared<std::ofstream>(file_path, std::ios_base::binary);

                    if(output_file->is_open())
                        break;
                }

                if(!output_file->is_open()) {
                    std::cout << "I tried to save it again but I couldn't, this file won't be retreived\";"
                              << std::endl;
                    return;
                }
            }
            else if (input == "n") {
                std::ostringstream oss2;
                oss2 << "Ok, the file: ";
                oss2 << file_path;
                oss2 << " won't be retreived from the server";
                std::cout << oss2.str() << std::endl;

                return;
            }
        } while (input != "y" && input != "n");
    }

    boost::array<char, 1024> buf{};
    boost::asio::streambuf request_buf;

    // Send the command to request the file
    std::ostringstream oss;
    oss <<  "getFile ";
    oss << cleaned_file_path;
    send_string(oss.str());

    // Receive file size
    std::string response = read_string();
    int pos = response.find(' ');
    int file_size = std::stoi(response.substr(pos, response.length()));

    // Send confirm to the server
    send_string("[CLIENT_SUCCESS] File size received");

    if(DEBUG) {
        std::ostringstream oss2;
        oss2 << "[DEBUG] File size: ";
        oss2 << file_size;
        oss2 << " bytes";
        std::cout << oss2.str() << std::endl;
    }

    for(;;) {
        size_t len = main_socket_->read_some(boost::asio::buffer(buf));
        if (len>0)
            output_file->write(buf.c_array(), (std::streamsize)len);
        if (output_file->tellp() == (std::fstream::pos_type)(std::streamsize)file_size)
            break; // File was received
    }

    int bytes_received = output_file->tellp();

    if(DEBUG) {
        std::cout << "[DEBUG] Received " << bytes_received << " bytes." << std::endl;
    }

    if(bytes_received != file_size) {
        std::cout << "[ERROR] File size and bytes received DOES NOT correspond!" << std::endl;
        std::cout << "File size: " << file_size << std::endl;
        std::cout << "Bytes received: " << bytes_received << std::endl;
        throw std::runtime_error("File not received correctly");
    }
}

/******************* SERIALIZATION ************************************************************************************/

std::unordered_map<std::string, std::string> Connection::get_filesystem_status() {
    send_string("checkFilesystemStatus");
    std::cout << read_string(); // Read server's response

    std::string serialized_data = read_string();
    std::stringstream archive_stream(serialized_data);
    boost::archive::text_iarchive archive(archive_stream);

    std::unordered_map<std::string, std::string> filesystem_status;
    archive >> filesystem_status;
    return filesystem_status;
}
