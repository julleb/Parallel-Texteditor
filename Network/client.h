#ifndef CLIENT_H
#define CLIENT_H

#include <boost/asio.hpp>   
#include <boost/asio/detail/array.hpp>
#include <iostream>
#include <string>
#include <sstream>
using boost::asio::ip::tcp;

class Client {
    private:
      boost::asio::io_service io_service;
      tcp::resolver::iterator endpoint_iterator;
      tcp::resolver::iterator end;
      tcp::socket *socket;
    public:
        Client(char *ip, char *port);
        Client();
        void connect();
        void sendMessage(const std::string message);
        
        std::string get_message();
        //static void* get_message_thread(void *arg);
        
        
};   


#endif
