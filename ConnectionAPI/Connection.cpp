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
    boost::asio::io_context io_context;
    std::unique_ptr<tcp::socket> socket;

public:
    //TODO decide if the socket creation need some check and some exception handling
    Connection(const std::string& ip_address, int port_number): io_context() {
        try {
            socket = std::make_unique<tcp::socket>(io_context); //Prova per cercare di inizializzare il socket DOPO io_context
            socket->connect(tcp::endpoint(boost::asio::ip::address::from_string(ip_address), port_number));
            /*
             * Otherwise delete line 23, delete unique pointer at line 17 and
             * add socket(io_context) at line 18 after io_context()
             */
        }
        catch(std::exception& e) {
            if(DEBUG) {
                std::cerr << "[!] Entered in the catch" << std::endl;
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
        boost::asio::read_until( *socket, buf, "\n", error);
        if(!error) {
            if(DEBUG) {
                std::cout << "[+] Receive succeded " << std::endl;
            }
            std::string data = boost::asio::buffer_cast<const char*>(buf.data());
            return data;
        }
        else {
            if(DEBUG) {
                std::cerr << "[-] Receive failed: " << error.message() << std::endl;
            }
        }
        return nullptr;
    }

    void send_string(std::string const& message) {
        std::cout << "[+] SEND - " << message << std::endl;
        const std::string msg = message + "\n";
        boost::system::error_code error;
        boost::asio::write( *socket, boost::asio::buffer(msg), error);
        if(!error) {
            if(DEBUG) {
                std::cout << "[+] Client sent: " << message << std::endl;
            }
        }
        else {
            if(DEBUG) {
                std::cerr << "[-] Send failed: " << error.message() << std::endl;
            }
        }
    }

    //TODO Rendere asincrono
    void send_file(std::string const& file_path) {
        // !!!WATCH OUT this buffer's size determines the MTU, therefore how many byte at time get copied
        // from source file to such buffer which means how many bytes get sent everytime
        boost::array<char, 1024> buf{};
        boost::system::error_code error;
        std::ifstream source_file(file_path, std::ios_base::binary | std::ios_base::ate);

        if(!source_file) {
            if(DEBUG) {
                std::cout << "[!] Failed to open " << file_path << std::endl;
            }
        }
        size_t file_size = source_file.tellg();
        source_file.seekg(0);

        std::string file_size_readable = file_size_to_readable(file_size);

        // First send file name and file size in bytes to server
        boost::asio::streambuf request;
        std::ostream request_stream(&request);
        request_stream << file_path << "\n"
                       << file_size << "\n\n"; // Consider sending readable version, does it change anything?

        // Send the request
        boost::asio::write(*socket, request, error);
        if(error){
            std::cout << "[!] Send request error:" << error << std::endl;
            //TODO lanciare un'eccezione? Qua dovrò controllare se il server funziona o no
        }
        if(DEBUG) {
            std::cout << "[+] " << file_path << " size is: " << file_size_readable << std::endl;
            std::cout << "[+] Start sending file content" << std::endl;
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
                std::cout << "[+] Bytes read from file: " << bytes_read_from_file << std::endl;
            }*/

            if(bytes_read_from_file<=0) {
                std::cout << "[!] Read file error" << std::endl;
                break;
                //TODO gestire questo errore
            }

            percent = std::ceil((100.0 * bytes_sent) / file_size);
            print_percentage(percent);

            boost::asio::write(*socket, boost::asio::buffer(buf.c_array(), source_file.gcount()),
                               boost::asio::transfer_all(), error);
            if(error) {
                std::cout << "[!] Send file error:" << error << std::endl;
                //TODO lanciare un'eccezione?
            }

            bytes_sent += bytes_read_from_file;
        }

        std::cout << "\n" << "[+] File " << file_path << " sent successfully!" << std::endl;
    }

    //TODO Rifinire e rendere asincrono
    void read_file() {
        boost::array<char, 1024> buf;
        size_t file_size = 0;

        try {
            boost::system::error_code error;
            boost::asio::streambuf request_buf;

            // Read the request saying file name and file size
            boost::asio::read_until(*socket, request_buf, "\n\n");
            if(DEBUG) {
                std::cout << "[+] Request size:" << request_buf.size() << "\n";
            }
            std::istream request_stream(&request_buf);
            std::string file_path;
            request_stream >> file_path;
            request_stream >> file_size;
            request_stream.read(buf.c_array(), 2); // eat the "\n\n"

            if(DEBUG) {
                std::cout << "[+] " << file_path << " size is: " << file_size << std::endl;
            }
            size_t pos = file_path.find_last_of('/');
            if (pos!=std::string::npos) {
                std::string file_name = file_path.substr(pos + 1);
                file_path = "/Users/andreascopp/Desktop/Client-TestFiles/" + file_name;
            }
            std::ofstream output_file(file_path.c_str(), std::ios_base::binary);
            if (!output_file){
                std::cout << "[!] Failed to open " << file_path << std::endl;
            }

            // write extra bytes to file
            do {
                request_stream.read(buf.c_array(), (std::streamsize)buf.size());
                if(DEBUG) {
                    std::cout << "[+] " << __FUNCTION__ << " wrote " << request_stream.gcount() << " bytes" << std::endl;
                }
                output_file.write(buf.c_array(), request_stream.gcount());
            } while (request_stream.gcount()>0);

            for(;;) {
                size_t len = socket->read_some(boost::asio::buffer(buf), error);
                if (len>0)
                    output_file.write(buf.c_array(), (std::streamsize)len);
                if (output_file.tellp()== (std::fstream::pos_type)(std::streamsize)file_size)
                    break; // file was received
                if (error){
                    std::cout << error << std::endl;
                    break;
                }
            }
            if(DEBUG) {
                std::cout << "[+] Received " << output_file.tellp() << " bytes.\n";
            }
        }
        catch(std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    /*void prova(std::string ip_address, int port_number) {
        try {
            boost::asio::io_service io_service;
            //socket creation
            tcp::socket socket(io_service);
            //connection
            socket.connect(tcp::endpoint(boost::asio::ip::address::from_string(ip_address), port_number));
            // request/message from client
            const std::string msg = "Hello from Client!\n";
            boost::system::error_code error;
            boost::asio::write(socket, boost::asio::buffer(msg), error);
            if (!error) {
                std::cout << "Client sent hello message!" << std::endl;
            } else {
                std::cout << "send failed: " << error.message() << std::endl;
            }
            // getting response from server
            boost::asio::streambuf receive_buffer;
            boost::asio::read(socket, receive_buffer, boost::asio::transfer_all(), error);
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
