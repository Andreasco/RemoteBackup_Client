//
// Created by Andrea Scoppetta on 09/12/20.
//

#include "Connection.h"

/******************* CONSTRUCTOR **********************************************************************************/

//TODO Gestire gli errori della creazione del main_socket_
Connection::Connection(std::string ip_address, int port_number, std::string base_path):
    io_context_(), read_timer_(io_context_), pool_(2), server_ip_address_(std::move(ip_address)), server_port_number_(port_number), base_path_(std::move(base_path)), reading(false), closed(false) {
    try {
        main_socket_ = std::make_unique<tcp::socket>(io_context_); //Prova per cercare di inizializzare il socket_ DOPO io_context_
        main_socket_->connect(tcp::endpoint(boost::asio::ip::address::from_string(server_ip_address_), server_port_number_));
        /*
         * Otherwise delete line 23, delete unique pointer at line 17 and
         * add socket_(io_context_) at line 18 after io_context_()
         */
    }
    catch(std::exception& e) {
        if(DEBUG) {
            std::cout << "[ERROR] Entered in the catch" << std::endl;
        }
        std::cout << e.what() << std::endl;
        throw e;
    }
}

/******************* DESTRUCTOR **********************************************************************************/

Connection::~Connection() {
    if(!closed) {
        close_connection();
    }
}

/******************* UTILITY METHODS ******************************************************************************/

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
    while (new_file_size > 1024){ //NOTE on Mac is 1000, it uses powers of 10
        new_file_size = new_file_size / 1024;
        i++;
    }
    return std::to_string(new_file_size) + " " + measures[i];
}

void Connection::close_connection(const std::shared_ptr<tcp::socket> &socket) {
    handle_close_connection(socket);
}

void Connection::close_connection() {
    handle_close_connection(main_socket_);
}

void Connection::handle_close_connection(const std::shared_ptr<tcp::socket> &socket){
    const std::string message = "close";
    send_string(message);
    socket->close();
    closed = true;
}

void Connection::print_string(const std::string &message) {
    std::lock_guard lg (mutex_);
    std::cout << message << std::endl;
}

/******************* STRINGS METHODS ******************************************************************************/

std::string Connection::read_string() {
    return handle_read_string(main_socket_);
}

std::string Connection::read_string(const std::shared_ptr<tcp::socket> &socket) {
    return handle_read_string(socket);
}

std::string Connection::handle_read_string(const std::shared_ptr<tcp::socket> &socket) {
    boost::system::error_code error;
    boost::asio::streambuf buf;
    boost::asio::read_until(*socket, buf, "\n", error);
    if(!error) {
        if(DEBUG) {
            //std::cout << "[DEBUG] Receive succeded" << std::endl;
            print_string("[DEBUG] Receive succeded");
        }
        std::string data = boost::asio::buffer_cast<const char*>(buf.data());
        return data;
    }
    else {
        //std::cout << "[ERROR] Receive failed: " << error.message() << std::endl;
        print_string("[ERROR] Receive failed: " + error.message());
    }
    return "-1";
}

std::string Connection::read_string_with_deadline(int deadline_seconds) {
    boost::asio::streambuf buf;
    boost::asio::async_read_until(*main_socket_, buf, "\n", [this](boost::system::error_code error, std::size_t bytes_read) -> std::string{
        reading = true;
        read_timer_.cancel(); // Cancel the timer because I started reading

        boost::asio::streambuf buf;
        if(!error) {
            if(DEBUG) {
                //std::cout << "[DEBUG] Receive succeded" << std::endl;
                print_string("[DEBUG] Receive succeded");
            }

            std::string data = boost::asio::buffer_cast<const char*>(buf.data());
            return data;
        }
        else {
            //std::cout << "[ERROR] Receive failed: " << error.message() << std::endl;
            print_string("[ERROR] Receive failed: " + error.message());
        }
        return "-1";
    });

    boost::system::error_code err;
    read_timer_.expires_from_now(boost::posix_time::seconds(deadline_seconds));
    read_timer_.wait(err);
    if(!err) {
        if(!reading) {
            if (DEBUG) {
                print_string("[DEBUG] Read timeout occurred");
            }

            main_socket_->cancel();
            return "-1";
        }
    }
    else {
        //std::cout << "[ERROR] Receive failed: " << error.message() << std::endl;
        print_string("[ERROR] Timer error: " + err.message());
    }

    reading = false; // In this way I don't risk that the main thread get stuck for some reason on line 135 when reading = true and resume his workflow after the end of the reading thread where I should set reading = false;
}

void Connection::send_string(const std::string &message) {
    handle_send_string(main_socket_, message);
}

void Connection::send_string(const std::shared_ptr<tcp::socket> &socket, const std::string &message) {
    handle_send_string(socket, message);
}

void Connection::handle_send_string(const std::shared_ptr<tcp::socket> &socket, const std::string &message) {
    if(DEBUG) {
        //std::cout << "[DEBUG] Sending string: " << message << std::endl;
        print_string("[DEBUG] Sending string: " + message);
    }
    const std::string msg = message + "\n";
    boost::system::error_code error;
    boost::asio::write(*socket, boost::asio::buffer(msg), error);
    if(!error) {
        if(DEBUG) {
            //std::cout << "[DEBUG] Client sent: " << message << std::endl;
            print_string("[DEBUG] Client sent: " + message);
        }
    }
    else {
        //std::cout << "[ERROR] In function: " << __FUNCTION__ << " Send failed: " << error.message() << std::endl;
        print_string("[ERROR] In function: " + std::string(__FUNCTION__) + " Send failed: " + error.message());
    }
}

/******************* FILES METHODS ******************************************************************************/

void Connection::remove_file(const std::string &file_path) {
    std::string cleaned_file_path = file_path.substr(base_path_.length(), file_path.length());

    std::ostringstream oss;
    oss << "removeFile ";
    oss << cleaned_file_path;
    oss << "\n";
    send_string(oss.str());

    //FIXME si blocca nel caso in cui il server crashi e non mandi nulla, cercare soluzione su SO
    std::string response = read_string();
    if(response == "-1" || response.find("[SERVER SUCCESS]") == std::string::npos){ // If server response doesn't contain [SERVER SUCCESS] or the deadline is passed
        std::this_thread::sleep_for(std::chrono::seconds(5));
        remove_file(file_path);
    }

    // Read the confirm of receipt from the server
    print_string(read_string());
}

void Connection::update_file(const std::string &file_path) {
    handle_send_file(file_path, "updateFile");
}

void Connection::add_file(const std::string &file_path) {
    handle_send_file(file_path, "addFile");
}

void Connection::handle_send_file(const std::string &file_path, const std::string &command) {
    // Open the file to send
    std::ifstream source_file(file_path, std::ios_base::binary | std::ios_base::ate);
    if(!source_file) {
        //std::cout << "[ERROR] Failed to open " << file_path << std::endl;
        print_string("[ERROR] Failed to open " + file_path);
        //TODO gestire errore
    }

    std::string cleaned_file_path = file_path.substr(base_path_.length(), file_path.length());
    size_t file_size = source_file.tellg();
    std::string file_size_readable = file_size_to_readable(file_size);

    if(DEBUG) {
        //std::cout << "[DEBUG] " << file_path << " size is: " << file_size_readable << std::endl;
        print_string("[DEBUG] " + file_path + " size is: " + file_size_readable);
        //std::cout << "[DEBUG] Cleaned file path: " << cleaned_file_path << std::endl;
        print_string("[DEBUG] Cleaned file path: " + cleaned_file_path);
    }

    std::ostringstream oss;
    oss << command + " ";
    oss << cleaned_file_path;
    oss << " ";
    oss << file_size;
    send_string(oss.str());

    /*//IF THE NEXT BLOCK DOESN'T WORK, COMMENT IT AND UNCOMMENT THIS
    // Read the confirm of command receipt from the server
    print_string(read_string());*/

    std::string response = read_string_with_deadline(3);
    if(response == "-1" || response.find("[SERVER SUCCESS]") == std::string::npos){ // If server response doesn't contain [SERVER SUCCESS] or the deadline is passed
        //std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "TIMERRRRRRR" << std::endl;
        //handle_send_file(file_path, command);
    }

    do_send_file(std::move(source_file));

    // Read the confirm of file receipt from the server
    print_string(read_string());

    //std::cout << "\n" << "[INFO] File " << file_path << " sent successfully!" << std::endl;
    print_string(std::string("\n") + "[INFO] File " + file_path + " sent successfully!");
}

void Connection::do_send_file(std::ifstream source_file) {
    // This buffer's size determines the MTU, therefore how many bytes at time get copied
    // from source file to such buffer, which means how many bytes get sent everytime
    boost::array<char, 1024> buf{};
    boost::system::error_code error;

    size_t file_size = source_file.tellg();
    source_file.seekg(0);

    if(DEBUG) {
        std::cout << "[DEBUG] Start sending file content" << std::endl;
    }

    long bytes_sent = 0;
    float percent = 0;
    print_percentage(percent);

    while(!source_file.eof()) {
        source_file.read(buf.c_array(), (std::streamsize)buf.size());

        int bytes_read_from_file = source_file.gcount(); //int is fine because i read at most buf's size, 1024 in this case

        // If uncommented the progress is printed in multiple lines succeded from this line
        // Only use to debug file reading otherwise the progress gets unreadable
        /*if(DEBUG){
            std::cout << "[DEBUG] Bytes read from file: " << bytes_read_from_file << std::endl;
        }*/

        if(bytes_read_from_file<=0) {
            std::cout << "[ERROR] Read file error" << std::endl;
            break;
            //TODO gestire questo errore
        }

        boost::asio::write(*main_socket_, boost::asio::buffer(buf.c_array(), source_file.gcount()),
                           boost::asio::transfer_all(), error);
        if(error) {
            std::cout << "[ERROR] Send file error: " << error << std::endl;
            //TODO lanciare un'eccezione?
        }

        bytes_sent += bytes_read_from_file;

        percent = std::ceil((100.0 * bytes_sent) / file_size);
        print_percentage(percent);
    }

    //TODO cercare di sportarlo fuori dalla funzione
    source_file.close();
}

//TODO Rifinire e rendere asincrono
void Connection::read_file() {
    boost::asio::post(pool_, [this] {
        std::unique_ptr<tcp::socket> socket = std::make_unique<tcp::socket>(io_context_);

        handle_read_file(std::move(socket));
    });
}

void Connection::handle_read_file(std::unique_ptr<tcp::socket> socket) {
    boost::array<char, 1024> buf{};

    // Probabilmente il try non serve
    try {
        boost::system::error_code error;
        boost::asio::streambuf request_buf;

        // Read the request saying file name and file size
        boost::asio::read_until(*socket, request_buf, "\n\n");
        if(DEBUG) {
            std::cout << "[DEBUG] Request size:" << request_buf.size() << "\n";
        }
        std::istream request_stream(&request_buf);
        std::string file_path;
        size_t file_size = 0;

        request_stream >> file_path;
        request_stream >> file_size;
        request_stream.read(buf.c_array(), 2); // eat the "\n\n" //TODO capire a cosa serva

        if(DEBUG) {
            std::cout << "[DEBUG] " << file_path << " size is: " << file_size << std::endl;
        }

        size_t pos = file_path.find_last_of('/'); //FIXME character may depend on the OS
        if (pos!=std::string::npos) {
            std::string file_name = file_path.substr(pos + 1);

            file_path = base_path_ + file_name;
        }
        std::ofstream output_file(file_path.c_str(), std::ios_base::binary);
        if (!output_file){
            std::cout << "[ERROR] Failed to open " << file_path << std::endl;
            //TODO gestire errore
        }

        // write extra bytes to file
        //TODO why?
        do {
            request_stream.read(buf.c_array(), (std::streamsize)buf.size());
            if(DEBUG) {
                std::cout << "[DEBUG] " << __FUNCTION__ << " wrote " << request_stream.gcount() << " bytes" << std::endl;
            }
            output_file.write(buf.c_array(), request_stream.gcount());
        } while (request_stream.gcount()>0);

        for(;;) {
            size_t len = socket->read_some(boost::asio::buffer(buf), error);
            if (len>0)
                output_file.write(buf.c_array(), (std::streamsize)len);
            if (output_file.tellp() == (std::fstream::pos_type)(std::streamsize)file_size)
                break; // File was received
            if (error){
                std::cout << "[ERROR] Save file error: " << error << std::endl;
                break;
            }
        }
        if(DEBUG) {
            std::cout << "[DEBUG] Received " << output_file.tellp() << " bytes.\n";
        }
    }
    catch(std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

/******************* SERIALIZATION ******************************************************************************/

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
