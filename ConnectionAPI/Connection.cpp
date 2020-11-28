//
// Created by Andrea Scoppetta on 07/10/2020.
//

#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/array.hpp>

static bool DEBUG = true;

using boost::asio::ip::tcp;

class Connection {
private:
    boost::asio::io_context io_context_;
    std::unique_ptr<tcp::socket> main_socket_;
    boost::asio::thread_pool pool_;

    std::string base_path_;
    std::string server_ip_address_;
    int server_port_number_;

public:
    //TODO decide if the socket_ creation need some check and some exception handling
    Connection(std::string  ip_address, int port_number, std::string  base_path):
            io_context_(), pool_(2), server_ip_address_(std::move(ip_address)), server_port_number_(port_number), base_path_(std::move(base_path)) {
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

    /******************* UTILITY METHODS ******************************************************************************/

    static void print_percentage(float percent){
        float hashes = percent / 5;
        float spaces = std::ceil(100.0 / 5 - percent / 5);
        std::cout << "\r" << "[" << std::string(hashes, '#') << std::string(spaces, ' ') << "]";
        std::cout << " " << percent << "%";
        std::cout.flush();
    }

    static std::string file_size_to_readable(int file_size){
        std::string measures[] = {"bytes", "KB", "MB", "GB", "TB"};
        int i = 0;
        int new_file_size = file_size;
        while (new_file_size > 1024){ //NOTE on Mac is 1000, it uses powers of 10
            new_file_size = new_file_size / 1024;
            i++;
        }
        return std::to_string(new_file_size) + " " + measures[i];
    }

    static void close_connection(const std::shared_ptr<tcp::socket>& socket){
        const std::string message = "\nclose";
        send_string(socket, message);
        socket->close();
    }

    /******************* STRING METHODS ******************************************************************************/

    std::string read_string() {
        boost::system::error_code error;
        boost::asio::streambuf buf;
        boost::asio::read_until(*main_socket_, buf, "\n", error);
        if(!error) {
            if(DEBUG) {
                std::cout << "[DEBUG] Receive succeded " << std::endl;
            }
            std::string data = boost::asio::buffer_cast<const char*>(buf.data());
            return data;
        }
        else {
            std::cout << "[ERROR] Receive failed: " << error.message() << std::endl;
        }
        return nullptr; //TODO questo va cambiato
    }

    static std::string read_string(const std::shared_ptr<tcp::socket>& socket) {
        boost::system::error_code error;
        boost::asio::streambuf buf;
        boost::asio::read_until(*socket, buf, "\n", error);
        if(!error) {
            if(DEBUG) {
                std::cout << "[DEBUG] Receive succeded " << std::endl;
            }
            std::string data = boost::asio::buffer_cast<const char*>(buf.data());
            return data;
        }
        else {
            std::cout << "[ERROR] Receive failed: " << error.message() << std::endl;
        }
        return nullptr; //TODO questo va cambiato
    }

    void send_string(std::string const& message) {
        if(DEBUG) {
            std::cout << "[DEBUG] Sending string: " << message << std::endl;
        }
        const std::string msg = message + "\n";
        boost::system::error_code error;
        boost::asio::write(*main_socket_, boost::asio::buffer(msg), error);
        if(!error) {
            if(DEBUG) {
                std::cout << "[DEBUG] Client sent: " << message << std::endl;
            }
        }
        else {
            std::cout << __FUNCTION__ << "[ERROR] Send failed: " << error.message() << std::endl;
        }
    }

    static void send_string(const std::shared_ptr<tcp::socket>& socket, std::string const& message) {
        if(DEBUG) {
            std::cout << "[DEBUG] Sending string: " << message << std::endl;
        }
        const std::string msg = message + "\n";
        boost::system::error_code error;
        boost::asio::write(*socket, boost::asio::buffer(msg), error);
        if(!error) {
            if(DEBUG) {
                std::cout << "[DEBUG] Client sent: " << message << std::endl;
            }
        }
        else {
            std::cout << "[ERROR] In function: " << __FUNCTION__ << " Send failed: " << error.message() << std::endl;
        }
    }

    /******************* FILES METHODS ******************************************************************************/

    void add_file(const std::string& file_path){
        boost::asio::post(pool_, [this, file_path] {
            // Everytime i send a file I open a new socket so that I can send multiple files asynchronously
            std::shared_ptr<tcp::socket> socket = std::make_shared<tcp::socket>(io_context_);
            socket->connect(tcp::endpoint(boost::asio::ip::address::from_string(server_ip_address_), server_port_number_));


            // I hope I'll be able to delete this
            std::string message = "login guido guido.poli";
            send_string(socket, message);

            // Open the file to send
            std::ifstream source_file(file_path, std::ios_base::binary | std::ios_base::ate);
            if(!source_file) {
                std::cout << "[ERROR] Failed to open " << file_path << std::endl;
                //TODO gestire errore
            }

            std::string cleaned_file_path = file_path.substr(base_path_.length(), file_path.length());
            size_t file_size = source_file.tellg();
            std::string file_size_readable = file_size_to_readable(file_size);

            if(DEBUG) {
                std::cout << "[DEBUG] " << file_path << " size is: " << file_size_readable << std::endl;
                std::cout << "[DEBUG] Cleaned file path: " << cleaned_file_path << std::endl;
            }

            std::ostringstream oss;
            oss << "addFile ";
            oss << cleaned_file_path;
            oss << " ";
            oss << file_size;
            oss << "\n";
            send_string(socket, oss.str()); //FIXME devo mandare questo attraverso il socket che creo qua

            handle_send_file(std::move(source_file), socket);

            close_connection(socket);

            std::cout << "\n" << "[INFO] File " << file_path << " sent successfully!" << std::endl;
        });
    }

    static void handle_send_file(std::ifstream source_file, const std::shared_ptr<tcp::socket>& socket) {
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

            boost::asio::write(*socket, boost::asio::buffer(buf.c_array(), source_file.gcount()),
                               boost::asio::transfer_all(), error);
            if(error) {
                std::cout << "[ERROR] Send file error: " << error << std::endl;
                //TODO lanciare un'eccezione?
            }

            bytes_sent += bytes_read_from_file;

            percent = std::ceil((100.0 * bytes_sent) / file_size);
            print_percentage(percent);
        }

        //TODO try to move this outside of this function
        source_file.close();
    }

    void read_file(){
        boost::asio::post(pool_, [this] {
            std::unique_ptr<tcp::socket> socket = std::make_unique<tcp::socket>(io_context_);

            handle_read_file(std::move(socket));
        });
    }

    //TODO Rifinire e rendere asincrono
    void handle_read_file(std::unique_ptr<tcp::socket> socket) {
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

            size_t pos = file_path.find_last_of('/'); ////FIXME character may depend on the OS
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
};
