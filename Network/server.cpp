#include "server.h"

namespace ps {
    Server::~Server() { 
        delete clients_monitor;
    }
    Server::Server() { }

    /* init
     * Initializes the servers buffer, mutexes and semaphores.
     */
    void Server::init() {
        id_counter = 0;
        pthread_mutex_init(&queue_mutex, NULL);
        pthread_mutex_init(&client_mutex, NULL);
        pthread_mutex_init(&buffer_mutex, NULL);
        int SHARED = 1; // semaphores are shared between processes
        //using semaphore to block the sending until the queue got some
        sem_init(&queue_empty, SHARED, 0);
        
        clients_monitor = new Mapmonitor();
        std::vector<char> temp;
        buffer.push_back(temp);
    }

    
    /* Constructor
     * @param port - Which port the server listens to.
     * Initializes the Server, sets up the port listening for incoming connections.
     */
    Server::Server(int port) {
        //start the "send to all" threads
        init();
        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
        ap = &acceptor;

        start_connection_listener();
    }

    /* start_connection_listener
     * Listens to incoming connections and assigns them to ClientHandlers with 
     * different IDs.
     */
    void Server::start_connection_listener() {
        pthread_t client_threads[20];
        
        pthread_create(&client_threads[4], NULL, &Server::message_execution_thread, this);
        
        try {
            while(true) {
                tcp::socket *sp = new tcp::socket(io_service); //we need to clear this
                ap->accept(*sp);
                ClientHandler *client = new ClientHandler(sp, id_counter);

                clients_monitor->request_write();
                clients_monitor->add_to_map(id_counter, client);
                clients_monitor->release_write();
                id_counter++;
                
                std::cout << "Received connection..." << std::endl;
                struct argz arg;
                arg.server_pointer = this;
                clients_monitor->request_read();
                arg.clienthandler_pointer = clients_monitor->get_clienthandler(id_counter-1);
                
                get_height_width(client);
                send_buffer_to_new_client(client); //clients[id_counter-1]
                clients_monitor->release_read();
                //start thread for getting message for a client.
                pthread_create(&(clients_monitor->get_clienthandler(id_counter-1)->thread), NULL, &Server::client_datahandler_thread, ((void*)&arg));
    
            }
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    /* remove_client
     * @param id - ID of the client that's being removed.
     * Removes a client that disconnects.
     */
    void Server::remove_client(int id) {
        clients_monitor->request_write();
        clients_monitor->remove_from_map(id);
        clients_monitor->release_write();
        
    }
    
    /* get_height_width
     * Updates the tb_height and tb_width for a specified client. Called at
     * initialization of the clients window.
     */
    void Server::get_height_width(ClientHandler *client) {
        boost::array<char, 1024> buf;
        boost::system::error_code error;
        size_t len = client->socket_pointer->read_some(boost::asio::buffer(buf), error);
        std::string message;
        for(int i = 0; i < len; ++i) {
            message.push_back(buf[i]);
        }
        std::vector<std::string> temp = split_protocol(message, ' ');
        client->tb_height = atoi(&temp[1][0]);
        client->tb_width = atoi(&temp[2][0]);
        
    }

    /* send_buffer_to_new_client
     * Sends the text buffer to a client. Called when a new client connects
     * and the buffer is not empty.
     */
    void Server::send_buffer_to_new_client(ClientHandler *client) {
        pthread_mutex_lock(&buffer_mutex);
        
        std::string res;
        if(buffer.size() == 1 && buffer[0].size() == 0) {
            res = "EMPTY";
            boost::system::error_code ignored_error;
            boost::asio::write(*(client->socket_pointer), boost::asio::buffer(res), ignored_error);
            pthread_mutex_unlock(&buffer_mutex);
            return;
        }
        
        for(int i = 0; i < buffer.size() ; ++i) {
            for(int j = 0; j < buffer[i].size(); ++j){
                
                res.push_back(buffer[i][j]);
            }
            //dont pushback if it is the last row in buffe
            if(i != buffer.size()-1)
                res.push_back('\n');
        }
        
        boost::system::error_code ignored_error;
        boost::asio::write(*(client->socket_pointer), boost::asio::buffer(res), ignored_error);
        pthread_mutex_unlock(&buffer_mutex);
        
    }
    
    
    /* client_datahandler_thread
     * Called when a new client thread is started and starts the listening loop
     * for the client.
     */
     void* Server::client_datahandler_thread(void *arg) { 
        struct argz *a = (struct argz*) arg;
        return (a->server_pointer->client_datahandler(a->clienthandler_pointer));
     }
     
    /* get_data
     * Reads the data sent from a specified client.
     */
    std::string Server::get_data(ClientHandler *client, boost::system::error_code &error) {
        boost::array<char, 1024> buf;
        std::string the_message ="";
        
        size_t len = client->socket_pointer->read_some(boost::asio::buffer(buf), error);
        
        std::cout<<"got message from " << client->id<<std::endl;
        std::cout<<error.message() <<" ERbROR " << error.value()<<std::endl;
        if (error == boost::asio::error::eof) {
            return the_message; 
        } else if (error) {
            return the_message; 
        }
        for(int i = 0; i < len; ++i) {
            the_message.push_back(buf[i]);
        }
        
        return the_message;    
    }
     
    /* client_data_handler
     * Handles the loop for a client, recieves data and adds it to the message queue.
     */
    void* Server::client_datahandler(ClientHandler *client) {
        std::string whole_message;
        boost::system::error_code error;
        while(true) {
            
            whole_message = get_data(client, error);
            
            if(error != 0 ) { // user dc
                remove_client(client->id);
                //thread should die!
                return NULL;
            }
            
            std::vector<std::string> messages = create_individual_messages(whole_message, client->id);
            add_message_to_queue(messages);
        }
    }
    
    /* message_execution
     * Pops a message from the message queue and executes the request.
     */
    void* Server::message_execution() {
        while(true) {
            sem_wait(&queue_empty); //blocks until sempahore is positive. To avoid busy waiting   
            
            std::string message = pop_from_queue();
            std::cout<<message<<std::endl;
            perform_action(message);
        }
    
    }
   
    /* perform_action
     * Will decompose the message and perform and action based on what action was requested.
     */
    void Server::perform_action(std::string message) {
        std::vector<std::string> temp_vec = split_protocol(message, ' ');
        
        if(temp_vec[0] == "RESIZE") {
            perform_resize_action(temp_vec);
        } else {
            if(temp_vec[0] == "AR_DOWN" || temp_vec[0] == "AR_LEFT" || 
               temp_vec[0] == "AR_RIGHT" || temp_vec[0] == "AR_UP") {
                perform_move_cursor_action(temp_vec);
            } else {
                perform_buffer_change_action(temp_vec);
            }
        }
    
    }
    
    /* pop_from_queue
     * Locks the message queue and pops and returns the message.
     */
    std::string Server::pop_from_queue() {
        pthread_mutex_lock(&queue_mutex);       
        std::string message = queue.front();
        queue.pop();
        pthread_mutex_unlock(&queue_mutex);
        return message;
    }
    
    /* perform_buffer_change_action
     * Performs an action that changes the buffer, such as adding a new character
     * or removing a character. Then send the changes to all clients connected.
     */
    void Server::perform_buffer_change_action(std::vector<std::string> &message_vec) {
        int client_id = string_to_int(message_vec[1]);
        clients_monitor->request_read();
        int y = clients_monitor->get_clienthandler(client_id)->buffer_cursor_y;
        int x = clients_monitor->get_clienthandler(client_id)->buffer_cursor_x;
        clients_monitor->release_read();
        std::string action = message_vec[0];
        
        add_message_to_buffer(y, x, action);
        update_all_client_cursors(y, x, action);
       
        clients_monitor->request_read();
        std::vector<ClientHandler*> tmp_vec = clients_monitor->get_map_as_vector();
        clients_monitor->release_read();
        //Sends messages to all clients with each clients own cursor_y and cursor_x
        //for(std::map<int,ClientHandler*>::iterator it = clients.begin(); it != clients.end(); it++) {
        for(int i = 0; i < tmp_vec.size(); ++i) {
            std::string message = create_protocol_message(y, x, action, tmp_vec[i]->id);
            send_data_to_client(message, tmp_vec[i]->id);
        }
    }
    
    /* perform_move_cursor_action
     * Performs an action that moves the buffer_cursor and terminal_cursor, such as
     * moving cursor using the arrow buttons. Then sends the new terminal_cursor
     * to the client that performed this action.
     */
    void Server::perform_move_cursor_action(std::vector<std::string> &message_vec) {
        int client_id = string_to_int(message_vec[1]);
        std::string action = message_vec[0];
        int x = 0;
        int y = 0;
        clients_monitor->request_read();
        ClientHandler *c = clients_monitor->get_clienthandler(client_id);
        update_client_cursors(c, y, x, action);
        clients_monitor->release_read();
        
        std::string message = create_protocol_message(y, x, "MOVE_CURSOR", client_id);
        send_data_to_client(message, client_id);
    }
    
    /* perform_resize_action
     * Performs an action that resizes the window. It will update a specified clients
     * tb_height and tb_width and then update the clients terminal_cursor and finally
     * send the updated data to the client that resized window.
     */
    void Server::perform_resize_action(std::vector<std::string> &message_vec) {
        int client_id = string_to_int(message_vec[3]);
        clients_monitor->request_read();
        clients_monitor->get_clienthandler(client_id)->tb_height = atoi(&message_vec[1][0]);
        clients_monitor->get_clienthandler(client_id)->tb_width = atoi(&message_vec[2][0]);
        clients_monitor->get_clienthandler(client_id)->sync_cursors(buffer);
        std::string message = create_protocol_message(0, 0, "MOVE_CURSOR", client_id);
        clients_monitor->release_read();
        send_data_to_client(message, client_id);
    }
    
    /* create_protocol_message
     * @param y - The row in buffer which the action is performed on.
     * @param x - The column in buffer which the action is performed on.
     * @param action - The action performed.
     * @param client_id - The client_id which the message is composed to.
     * Composes a message according to the protocol where the message will contain:
     * [TERM_CURS_Y, TERM_CURS_X, Y, X, ACTION]
     */
    std::string Server::create_protocol_message(int y, int x, std::string action, int client_id) {
        clients_monitor->request_read();
        ClientHandler *c = clients_monitor->get_clienthandler(client_id);
        clients_monitor->release_read();
        return int_to_string(c->cursor_y) + " " 
        + int_to_string(c->cursor_x) + " " 
        + int_to_string(y) + " " + int_to_string(x) + " " 
        + action + " ";
    }
    
    /* send_data_to_client
     * Sends data to a specified client.
     */
    void Server::send_data_to_client(std::string message, int client_id) {
        boost::system::error_code ignored_error;
        int counter = 0;
        clients_monitor->request_read();
        boost::asio::write(*((clients_monitor->get_clienthandler(client_id))->socket_pointer), boost::asio::buffer(message), ignored_error);
        
        while(ignored_error != 0 && counter < 5) {
            counter++;
            boost::asio::write(*((clients_monitor->get_clienthandler(client_id))->socket_pointer), boost::asio::buffer(message), ignored_error);
        }
        clients_monitor->release_read();
    }
    
    /* update_all_client_cursors
     * Updates all clients buffer_cursors and terminal_cursors.
     */
    void Server::update_all_client_cursors(int &y, int &x, std::string &action) {
        if(action == "BS") {
            if(y == 0 && x == 0) {
                y = -1;
                x = -1;
                return;
            }
        }
        
        clients_monitor->request_read();
        std::vector<ClientHandler*> tmp_vec = clients_monitor->get_map_as_vector();
        
#pragma omp parallel for num_threads(tmp_vec.size())
        for(int i = 0; i < tmp_vec.size(); ++i) {
            update_client_cursors(tmp_vec[i], y, x, action);
        }
        clients_monitor->release_read();
    }

    /* update_client_cursors
     * Updates a specified clients buffer_cursors and terminal_cursors depending on
     * the action and the coordinates in the buffer the action is performed.
     */
    void Server::update_client_cursors(ClientHandler *c, int &y, int &x, std::string &action) {
        if(action == "BS") {
            if(y == c->buffer_cursor_y && x <= c->buffer_cursor_x) {
                if(x == 0) {
                    c->buffer_cursor_x += prev_row_length;
                    c->buffer_cursor_y--;
                } else {
                    c->buffer_cursor_x--;
                }
            } else if(y < c->buffer_cursor_y) {
                if(x == 0) {
                    c->buffer_cursor_y--;
                }
            }
        } else if(action == "\n") {
            if(y < c->buffer_cursor_y ) {
                c->buffer_cursor_y++;
            } else if(y == c->buffer_cursor_y && x <= c->buffer_cursor_x) {
                c->buffer_cursor_y++;
                c->buffer_cursor_x = c->buffer_cursor_x - x;
            }
        } else if(action == "AR_DOWN") {
            if(c->cursor_y != c->tb_height) {
                if(c->buffer_cursor_y != buffer.size()-1) {
                    if(c->buffer_cursor_x + c->tb_width < buffer[c->buffer_cursor_y].size()) {
                        c->buffer_cursor_x += c->tb_width;
                    } else if(buffer.size()-1 != c->buffer_cursor_y) {
                        c->buffer_cursor_y++;
                       
                        if(c->buffer_cursor_x > buffer[c->buffer_cursor_y].size()) {
                            c->buffer_cursor_x = buffer[c->buffer_cursor_y].size();
                        }
                    }
                }
            }
        } else if(action == "AR_UP") {
            if(c->cursor_y != 0) {
                if(c->buffer_cursor_x ==  c->cursor_x) {
                    c->buffer_cursor_y--;
                    if(buffer[c->buffer_cursor_y].size() < c->buffer_cursor_x) {
                        c->buffer_cursor_x = buffer[c->buffer_cursor_y].size();
                    }
                }else {
                    c->buffer_cursor_x -= c->tb_width;
                }
            }
        } else if(action == "AR_LEFT") {
            if(c->cursor_y != 0 && c->cursor_x == 0) {
                if(c->buffer_cursor_x != c->cursor_x) {
                    c->buffer_cursor_x--;
                } else {
                    c->buffer_cursor_y--;
                    c->buffer_cursor_x = buffer[c->buffer_cursor_y].size();
                }
               
            } else if(c->cursor_x != 0) {
                c->buffer_cursor_x--;
            }
        } else if(action == "AR_RIGHT") {
            if(c->buffer_cursor_x < buffer[c->buffer_cursor_y].size()){
                c->buffer_cursor_x++;
            }
        } else {
            if(x <= c->buffer_cursor_x && y == c->buffer_cursor_y) {
                c->buffer_cursor_x++;
            }
        }
        
        c->sync_cursors(buffer);
    }
    
    
    /* split_protocol
     * Split the protocol on a delimiter variable, and returns an vector.
     */
    std::vector<std::string> Server::split_protocol(std::string protocol, char delimiter) {
        std::vector<std::string> vec;
        for(int i = 0; i < protocol.length(); ++i) {
            if(protocol[i] == delimiter) {
                vec.push_back(protocol.substr(0, i));
                protocol = protocol.substr(i+1,protocol.length());
                i=0;
            }
        }
        if(protocol.size() !=0)
            vec.push_back(protocol);    
        return vec;
    } 
    
    /* add_message_to_buffer
     * Performs the action on the buffer depending on the action.
     */
    void Server::add_message_to_buffer(int &y, int &x, std::string &action) {
        pthread_mutex_lock(&buffer_mutex);
        if(action == "\n") {
            std::vector<char> temp;
            if(x < buffer[y].size()) {
                while(buffer[y].size() > x ) {
                    //pushing everything to the right of the cursor to the new row
                    temp.push_back(buffer[y][x]);
                    buffer[y].erase(buffer[y].begin() + x);  
                }
            } 
            buffer.insert(buffer.begin() + y + 1, temp);            
        } else if(action == "BS") {
            if(y == 0 && x == 0) {
                pthread_mutex_unlock(&buffer_mutex);
                return;
            }
            if(y != 0) {
                prev_row_length =  buffer[y-1].size();    
            }
            
            if(x == 0) {
                for(int i = 0; i < buffer[y].size(); ++i) {
                    buffer[y-1].push_back(buffer[y][i]);
                }
                
                //remove the row
                buffer.erase(buffer.begin() + y);
            } else {    
                if(buffer[y].size() != 0) {
                    buffer[y].erase(buffer[y].begin()+x-1);
                } else {
                    buffer.erase(buffer.begin() + y);
                }
            }
        } else { //write character 
            buffer[y].insert(buffer[y].begin()+x, action[0]);  
        }
        
        pthread_mutex_unlock(&buffer_mutex);
    
    }
    
    //send data thread.
    void* Server::message_execution_thread(void *arg) {
        return ((Server*)arg)->message_execution();
    }
    
    /* create_individual_messages
     * Handles multiple messages received once from client
     * and returns them as a vector of strings
     */
    std::vector<std::string> Server::create_individual_messages(std::string whole_message, int id) {
        std::vector<std::string> individual_messages;
        std::vector<std::string> temp = split_protocol(whole_message, ' ');

        for(int i = 0; i < temp.size(); ++i) {
            std::string message;
            if(temp[i] == "RESIZE") {
                //the message is RESIZE, it will be : RESIZE height width
                message = temp[i] + " " + temp[i+1] + " " + temp[i+2] + " " + int_to_string(id);
                individual_messages.push_back(message);
                i+=2;
            } else {
                //a normal message, "ACTION"
                message = temp[i] + " " + int_to_string(id);
                individual_messages.push_back(message);
            }
        }
        return individual_messages;
    }
    
    /* string_to_int
     * Takes a string and returns it as an int.
     */
    int Server::string_to_int(std::string str) {
        return atoi(&str[0]);
    }
    
    /* int_to_string
     * Takes an int and returns it as a string.
     */
    std::string Server::int_to_string(int i) {
        char convert_int[32];
        sprintf(convert_int, "%d", i);
        std::string str(convert_int);
        return str;
    }
    
    /* add_message_to_queue
     * Add messages to the queue.
     */
    void Server::add_message_to_queue(std::vector<std::string> &messages) {
        pthread_mutex_lock(&queue_mutex);
       
        for(int i = 0; i < messages.size(); ++i) {
            queue.push(messages[i]);
            sem_post(&queue_empty); // increase the semaphore, so we can send
        }
        pthread_mutex_unlock(&queue_mutex);
    }
}
