
#include <boost/asio.hpp>   
#include <boost/array.hpp>
#include <iostream>
#include <string>
#include <sstream>
using boost::asio::ip::tcp;

int main(int argc, char* argv[] ) {
    
    try {
        if(argc!=2){
            std::cerr << "insert clientip" <<std::endl;
            return 1;
        }
        
        boost::asio::io_service io_service;
       
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(argv[1], "1234"); //"daytime"
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        tcp::resolver::iterator end;
        tcp::socket socket(io_service);
        std::cout<<"CONNECTED"<<std::endl;
        
        boost::system::error_code error = boost::asio::error::host_not_found;
        while (error && endpoint_iterator != end) {
            socket.close();
            socket.connect(*endpoint_iterator++, error); //connects here.
            while(true) {
                std::string message ="yo";
                std::getline (std::cin, message);
                std::cout<<message<<std::endl;
                boost::system::error_code ignored_error;
                boost::asio::write(socket, boost::asio::buffer(message), ignored_error);
            }   
            
        }
        if (error) {
            throw boost::system::system_error(error);
        }
        
    }catch (std::exception& e) {
              std::cerr << e.what() << std::endl;
    }
    
    
    
}
