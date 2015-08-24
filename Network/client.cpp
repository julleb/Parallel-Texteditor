#include "client.h"


Client::Client() { }



Client::Client(char *ip, char *port) {
    try {
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(ip, port);
        endpoint_iterator = resolver.resolve(query);
        socket = new tcp::socket(io_service);
        
        connect();
        
    }catch ( std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    
}

//sends a message to the server
void Client::sendMessage(const std::string message) {    
    boost::system::error_code ignored_error;
    boost::asio::write(*socket, boost::asio::buffer(message), ignored_error);
    
}


//connects to a specific server
void Client::connect() {
    boost::system::error_code error = boost::asio::error::host_not_found;
    while(error && endpoint_iterator != end) {
        socket->close();
        socket->connect(*endpoint_iterator++,error);
        
    }
    if(error) {
        throw boost::system::system_error(error);
    }
}


std::string Client::get_message() {
    boost::array<char, 1024> buf;
    std::string message;
    boost::system::error_code error;
    //read_some is blocking function
    size_t len = socket->read_some(boost::asio::buffer(buf), error);
    if (error == boost::asio::error::eof) {
        //break; // Connection closed cleanly by peer.
        return message;
     }else if (error) {
       //break;
       return message;
     }
     for(int i = 0; i < len; ++i) {
        message.push_back(buf[i]);
     }
     return message;
}

/*
void* Client::get_message_thread(void *arg) {
    return ((Client*)arg)->get_message();
}
*/








