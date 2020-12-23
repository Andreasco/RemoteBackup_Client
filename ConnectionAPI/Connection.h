//
// Created by Andrea Scoppetta on 07/10/2020.
//

#ifndef REMOTEBACKUP_CLIENT_CONNECTION_H
#define REMOTEBACKUP_CLIENT_CONNECTION_H

#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/string.hpp>
#include <boost/archive/text_iarchive.hpp>

static bool DEBUG = true;

using boost::asio::ip::tcp;

class Connection {
private:
    boost::asio::io_context io_context_;
    std::shared_ptr<tcp::socket> main_socket_;
    boost::asio::thread_pool pool_;

    std::string base_path_;
    std::string server_ip_address_;
    int server_port_number_;

    boost::asio::deadline_timer read_timer_;
    bool reading;
    bool closed;

    std::mutex mutex_;

public:
    /******************* CONSTRUCTOR **********************************************************************************/

    Connection(std::string  ip_address, int port_number, std::string  base_path);

    /******************* DESTRUCTOR **********************************************************************************/

    ~Connection();

    /******************* UTILITY METHODS ******************************************************************************/

    static void print_percentage(float percent);

    static std::string file_size_to_readable(int file_size);

    void close_connection(const std::shared_ptr<tcp::socket>& socket);

    void close_connection();

    void handle_close_connection(const std::shared_ptr<tcp::socket> &socket);

    void print_string(const std::string& message);

    /******************* STRINGS METHODS ******************************************************************************/

    std::string read_string();

    std::string read_string(const std::shared_ptr<tcp::socket>& socket);

    std::string handle_read_string(const std::shared_ptr<tcp::socket> &socket);

    std::string read_string_with_deadline(int deadline_seconds);

    void send_string(const std::string& message);

    void send_string(const std::shared_ptr<tcp::socket>& socket, const std::string& message);

    void handle_send_string(const std::shared_ptr<tcp::socket> &socket, const std::string &message);

    /******************* FILES METHODS ******************************************************************************/

    void remove_file(const std::string& file_path);

    void update_file(const std::string& file_path);

    void add_file(const std::string& file_path);

    void handle_send_file(const std::string& file_path, const std::string& command);

    void do_send_file(std::ifstream source_file);

    void get_file(const std::string &file_path);

    void handle_get_file(const std::string &file_path);

    /******************* SERIALIZATION ******************************************************************************/

    std::unordered_map<std::string, std::string> get_filesystem_status();
};

#endif
