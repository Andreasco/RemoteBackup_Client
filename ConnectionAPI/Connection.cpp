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
    std::unique_ptr<tcp::socket> socket_;
    boost::asio::thread_pool pool_;
    std::string base_path_;

public:
    //TODO decide if the socket_ creation need some check and some exception handling
    Connection(const std::string& ip_address, int port_number, const std::string& base_path): io_context_(), pool_(2) {
        try {
            socket_ = std::make_unique<tcp::socket>(io_context_); //Prova per cercare di inizializzare il socket_ DOPO io_context_
            socket_->connect(tcp::endpoint(boost::asio::ip::address::from_string(ip_address), port_number));

            base_path_ = base_path;
            /*
             * Otherwise delete line 23, delete unique pointer at line 17 and
             * add socket_(io_context_) at line 18 after io_context_()
             */
        }
        catch(std::exception& e) {
            if(DEBUG) {
                std::cerr << "[ERROR] Entered in the catch" << std::endl;
            }
            std::cerr << e.what() << std::endl;
            throw e;
        }
    }

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

    std::string read_string() {
        boost::system::error_code error;
        boost::asio::streambuf buf;
        boost::asio::read_until(*socket_, buf, "\n", error);
        if(!error) {
            if(DEBUG) {
                std::cout << "[DEBUG] Receive succeded " << std::endl;
            }
            std::string data = boost::asio::buffer_cast<const char*>(buf.data());
            return data;
        }
        else {
            std::cerr << "[ERROR] Receive failed: " << error.message() << std::endl;
        }
        return nullptr;
    }

    void send_string(std::string const& message) {
        if(DEBUG) {
            std::cout << "[DEBUG] Sending string: " << message << std::endl;
        }
        const std::string msg = message + "\n";
        boost::system::error_code error;
        boost::asio::write(*socket_, boost::asio::buffer(msg), error);
        if(!error) {
            if(DEBUG) {
                std::cout << "[DEBUG] Client sent: " << message << std::endl;
            }
        }
        else {
            std::cerr << "[ERROR] Send failed: " << error.message() << std::endl;
        }
    }

    void add_file(const std::string& file_path){
        boost::asio::post(pool_, [this, file_path] {
            std::string cleaned_file_path = file_path.substr(base_path_.length(), file_path.length());

            if(DEBUG)
                std::cout << "[DEBUG] Cleaned file path: " << cleaned_file_path << std::endl;

            // Forse da cambiare nel caso non serva al Server
            std::ifstream source_file(file_path, std::ios_base::binary | std::ios_base::ate);
            size_t file_size = source_file.tellg();
            source_file.close();

            std::ostringstream oss;
            oss << "addFile ";
            oss << cleaned_file_path;
            oss << " ";
            oss << file_size;
            oss << "\n";
            send_string(oss.str());

            handle_send_file(file_path);
        });
    }

    void handle_send_file(std::string const& file_path) {
        // !!!WATCH OUT this buffer's size determines the MTU, therefore how many byte at time get copied
        // from source file to such buffer, which means how many bytes get sent everytime
        boost::array<char, 1024> buf{};
        boost::system::error_code error;
        std::ifstream source_file(file_path, std::ios_base::binary | std::ios_base::ate);

        if(!source_file) {
            std::cout << "[ERROR] Failed to open " << file_path << std::endl;
            //TODO gestire errore
        }
        size_t file_size = source_file.tellg();
        source_file.seekg(0);

        std::string file_size_readable = file_size_to_readable(file_size);

        /*// First send file name and file size in bytes to server
        boost::asio::streambuf request;
        std::ostream request_stream(&request);
        request_stream << file_path << "\n"
                       << file_size << "\n\n"; // Consider sending readable version, does it change anything?

        // Send the request
        boost::asio::write(*socket_, request, error);
        if(error){
            std::cout << "[ERROR] Send request error:" << error << std::endl;
            //TODO lanciare un'eccezione? Qua dovrò controllare se il server funziona o no
        }*/

        if(DEBUG) {
            std::cout << "[DEBUG] " << file_path << " size is: " << file_size_readable << std::endl;
            std::cout << "[DEBUG] Start sending file content" << std::endl;
        }

        long bytes_sent = 0;
        float percent = 0;
        print_percentage(percent);

        while(!source_file.eof()) {
            source_file.read(buf.c_array(), (std::streamsize)buf.size());

            int bytes_read_from_file = source_file.gcount(); //int is fine because i read at most buf's size, 1024 in this case

            // If uncommented the progress is printed in multiple lines succeded from this line
            // Only use to debug file reading otherwise the progress get unreadable
            /*if(DEBUG){
                std::cout << "[DEBUG] Bytes read from file: " << bytes_read_from_file << std::endl;
            }*/

            if(bytes_read_from_file<=0) {
                std::cout << "[ERROR] Read file error" << std::endl;
                break;
                //TODO gestire questo errore
            }

            percent = std::ceil((100.0 * bytes_sent) / file_size);
            print_percentage(percent);

            boost::asio::write(*socket_, boost::asio::buffer(buf.c_array(), source_file.gcount()),
                               boost::asio::transfer_all(), error);
            if(error) {
                std::cout << "[ERROR] Send file error: " << error << std::endl;
                //TODO lanciare un'eccezione?
            }

            bytes_sent += bytes_read_from_file;
        }

        std::cout << "\n" << "[INFO] File " << file_path << " sent successfully!" << std::endl;
    }

    void read_file(){
        boost::asio::post(pool_, [this] {
            handle_read_file();
        });
    }

    //TODO Rifinire e rendere asincrono
    void handle_read_file() {
        boost::array<char, 1024> buf{};

        // Probabilmente il try non serve
        try {
            boost::system::error_code error;
            boost::asio::streambuf request_buf;

            // Read the request saying file name and file size
            boost::asio::read_until(*socket_, request_buf, "\n\n");
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
                size_t len = socket_->read_some(boost::asio::buffer(buf), error);
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
            std::cerr << e.what() << std::endl;
        }
    }

    /*void prova(std::string ip_address, int port_number) {
        try {
            boost::asio::io_service io_service;
            //socket_ creation
            tcp::socket_ socket_(io_service);
            //connection
            socket_.connect(tcp::endpoint(boost::asio::ip::address::from_string(ip_address), port_number));
            // request/message from client
            const std::string msg = "Hello from Client!\n";
            boost::system::error_code error;
            boost::asio::write(socket_, boost::asio::buffer(msg), error);
            if (!error) {
                std::cout << "Client sent hello message!" << std::endl;
            } else {
                std::cout << "send failed: " << error.message() << std::endl;
            }
            // getting response from server
            boost::asio::streambuf receive_buffer;
            boost::asio::read(socket_, receive_buffer, boost::asio::transfer_all(), error);
            if (error && error != boost::asio::error::eof) {
                std::cout << "receive failed: " << error.message() << std::endl;
            } else {
                const char *data = boost::asio::buffer_cast<const char *>(receive_buffer.data());
                std::cout << data << std::endl;
            }
        }
        catch (std::exception &e) {
            std::cerr << "Entered in the catch" << std::endl;
            std::cerr << e.what() << std::endl;
        }
    }*/
};
