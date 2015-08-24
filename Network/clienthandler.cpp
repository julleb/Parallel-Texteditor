#include "clienthandler.h"

namespace ps {

ClientHandler::ClientHandler(tcp::socket *sp,int i) {
    socket_pointer = sp;
    id = i;
    buffer_cursor_x = buffer_cursor_y = 0;
    cursor_x = cursor_y = 0;
    tb_height = tb_width = 0;
}


ClientHandler::ClientHandler() {

}

void ClientHandler::sync_cursors(std::vector<std::vector<char> > &buffer) {
    cursor_x = 0;
    cursor_y = 0;
    int tmp_buff_x = 0;
    for(int i = 0; i < buffer_cursor_y; ++i) {
        tmp_buff_x = buffer[i].size();
        while(tmp_buff_x >= 0) {
            tmp_buff_x -= tb_width;
            cursor_y++;
        }   
    }
    
    tmp_buff_x = buffer_cursor_x;
    while(tmp_buff_x >= tb_width) {
        tmp_buff_x -= tb_width;
        cursor_y++;
    }
    cursor_x = buffer_cursor_x % tb_width;
}

ClientHandler& ClientHandler::operator=(ClientHandler c) {
    socket_pointer = c.socket_pointer;
    id = c.id;
    thread = c.thread;
    buffer_cursor_x = c.buffer_cursor_x;
    buffer_cursor_y = c.buffer_cursor_y;
    tb_height = c.tb_height;
    tb_width = c.tb_width;
    return *this;
}


}