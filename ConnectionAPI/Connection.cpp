//
// Created by Andrea Scoppetta on 07/10/2020.
//

#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>

using boost::asio::ip::tcp;

class Connection {
private:
    boost::asio::io_context io_context;
    tcp::socket* socket;

public:
    //TODO decide if the socket creation need some check and some exception handling
    Connection(const std::string& ip_address, int port_number): io_context() {
        try {
            socket = new tcp::socket(io_context); //Prova per cercare di inizializzare il socket DOPO io_context
            socket->connect( tcp::endpoint( boost::asio::ip::address::from_string(ip_address), port_number ));
            //Altrimenti cancellare la prima riga del try, togliere l'asterisco alla riga 14 e aggiungere socket(io_context)
            //alla riga 18 dopo io_context()
        }
        catch (std::exception& e)
        {
            std::cerr << "Entered in the catch" << std::endl;
            std::cerr << e.what() << std::endl;
            throw e;
        }
    }

    std::string read(){
        boost::system::error_code error;
        boost::asio::streambuf buf;
        boost::asio::read_until( *socket, buf, "\n", error);
        if(!error){
            std::cout << "[+] Receive succeded " << std::endl;
            std::string data = boost::asio::buffer_cast<const char*>(buf.data());
            return data;
        }
        else{
            std::cout << "[-] Receive failed: " << error.message() << std::endl;
        }
        return nullptr;
    }

    void send(std::string const& message){
        std::cout << "[+] SEND - " << message << std::endl;
        const std::string msg = message + "\n";
        boost::system::error_code error;
        boost::asio::write( *socket, boost::asio::buffer(msg), error);
        if(!error) {
            std::cout << "[+] Client sent: " << message << std::endl;
        }
        else {
            std::cout << "[-] Send failed: " << error.message() << std::endl;
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
