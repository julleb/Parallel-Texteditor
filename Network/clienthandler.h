#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H
#include <vector>
#include <boost/asio.hpp> 


namespace ps {
    using boost::asio::ip::tcp;

    class ClientHandler {
        public :
            pthread_t thread;
            tcp::socket *socket_pointer;
            int id;
            int tb_width, tb_height;
            int buffer_cursor_x, buffer_cursor_y;
            int cursor_x, cursor_y;
            ClientHandler(tcp::socket *sp,int i);
            ClientHandler();
            void sync_cursors(std::vector<std::vector<char> > &buffer);
            ClientHandler& operator=(ClientHandler c);
    };

}
#endif