#ifndef PARALLEL_SERVER
#define PARALLEL_SERVER

#include <ctime>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <vector>
#include <queue>
#include <pthread.h>
#include <semaphore.h>
#include <omp.h>
#include <map>
#include <iterator> 
#include <iostream>
#include <vector>
#include <stdlib.h> 
#include <stdio.h>
#include "mapmonitor.h"
namespace ps {
    using boost::asio::ip::tcp;
    

    class Server {    
        private:
            pthread_mutex_t queue_mutex;
            pthread_mutex_t client_mutex;
            pthread_mutex_t buffer_mutex;
            sem_t queue_empty;
            
            boost::asio::io_service io_service;
            tcp::acceptor *ap;
            //std::map<int, ClientHandler*> clients;
            Mapmonitor *clients_monitor;
            std::queue<std::string> queue;
            std::vector<std::vector<char> > buffer;
            
            int prev_row_length;
            int id_counter;
            void init();
            void remove_client(int id);
            void add_message_to_queue(std::vector<std::string> &messages);
            void add_message_to_buffer(int &y, int &x, std::string &action);
            
            void send_buffer_to_new_client(ClientHandler *client);
            void get_height_width(ClientHandler *client);
            void update_client_cursors(ClientHandler *c, int &y, int &x, std::string &action);
            void update_all_client_cursors(int &y, int &x, std::string &action);

            std::vector<std::string> create_individual_messages(std::string whole_message, int id);
            std::vector<std::string> split_protocol(std::string protocol, char delimiter);

        public:
            ~Server();
            Server();
            Server(int port);

            struct argz {
                Server *server_pointer;
                ClientHandler *clienthandler_pointer;   
            };
            
            std::string get_data(ClientHandler *client, boost::system::error_code &error);
            static void* client_datahandler_thread(void *arg);
           
            static void* message_execution_thread(void *arg);
            
            void* message_execution();
            
            void perform_action(std::string message);
            void perform_buffer_change_action(std::vector<std::string> &message_vec);
            void perform_move_cursor_action(std::vector<std::string> &message_vec);
            void perform_resize_action(std::vector<std::string> &message_vec);
            
            std::string create_protocol_message(int y, int x, std::string action, int client_id);
            
            std::string pop_from_queue();
            
            void start_connection_listener();
            void* client_datahandler(ClientHandler *client);
            void send_data_to_client(std::string message, int client_id);
            
            //help functions
            int string_to_int(std::string str);
            std::string int_to_string(int i);
    };
}

#endif
