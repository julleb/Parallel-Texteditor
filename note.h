#include <iostream>
#include "termbox/hej/usr/local/include/termbox.h"
#include <vector>
#include <stdlib.h> 
#include <stdio.h>

#include "Network/client.h"
#include <pthread.h>
#include <semaphore.h>
class Notepad {

    private: 
        std::vector<std::vector<char> > buffer;
        std::vector<std::string> unhandled_messages;
        int cursor_x, cursor_y;
        int message_length;
        
        tb_event keyevent;
        
        Client *client;
        pthread_t threads[20];
        
        void get_full_buffer();
        void send_height_width();
        void check_unhandled_messages();
        bool is_handable_message(int y, int x);
        void handle_message(std::vector<std::string> &protocol_vec);
        
        std::vector<std::vector<std::string> > get_all_split_messages(std::string whole_message);
        
        pthread_mutex_t mutex_reader;
        
        
    public:
        ~Notepad();
        Notepad();
        Notepad(char *ip, char *port);
        void init();
        void key_event();
        void draw_screen();
        void write_char(char c,int y, int x);
        char get_char();
        void set_cursor(int y, int x);
        void add_char_to_buffer(char c, int y, int x);
        void remove_char_from_buffer(int y, int x);
        
        std::vector<std::string> split_protocol(std::string protocol, char delimiter);
        
        static void* get_data_thread(void *arg);
        void* get_data(void);
};