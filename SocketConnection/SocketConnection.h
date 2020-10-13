//
// Created by Andrea Scoppetta on 07/10/2020.
//

#ifndef REMOTEBACKUP_CLIENT_SOCKETCONNECTION_H
#define REMOTEBACKUP_CLIENT_SOCKETCONNECTION_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>

using boost::asio::ip::tcp;

class SocketConnection {
public:
    void prova(){
        boost::asio::io_context io_context;

        try
        {
            boost::asio::io_context io_context;

            tcp::resolver resolver(io_context);
            tcp::resolver::results_type endpoints =
                    resolver.resolve("193.204.114.105", "daytime");

            tcp::socket socket(io_context);
            boost::asio::connect(socket, endpoints);

            for (;;)
            {
                boost::array<char, 128> buf;
                boost::system::error_code error;

                size_t len = socket.read_some(boost::asio::buffer(buf), error);

                if (error == boost::asio::error::eof)
                    break; // Connection closed cleanly by peer.
                else if (error)
                    throw boost::system::system_error(error); // Some other error.

                std::cout.write(buf.data(), len);
            }
        }
        catch (std::exception& e)
        {
            std::cerr << "Entered in the catch" << std::endl;
            std::cerr << e.what() << std::endl;
        }
    }
};


#endif //REMOTEBACKUP_CLIENT_SOCKETCONNECTION_H
