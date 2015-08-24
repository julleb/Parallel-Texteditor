

#include <iostream>
#include "termbox/hej/usr/local/include/termbox.h"
#include "note.h"
#include <string>


/*Destructor for Notepad
 *
 */
Notepad::~Notepad() {
    delete client;
}

/*Constructor for Notepad
 * Not in use
 */
Notepad::Notepad() {

    init();
    while(true) {
        key_event();
    }
    
}

/*Constructor for Notepad
 *Connect to a server, initializing variables, 
 *start a thread to recieve from server,
 *and waits for user input
 *@param ip - ip of the server
 *@param port - the port for the server
 */
Notepad::Notepad(char *ip, char *port) {
    
    //makes a connection to the server
    client = new Client(ip, port);
    
    init();
    
    //thread to get messages from server
    pthread_create(&threads[1], NULL, &Notepad::get_data_thread, this);
    while(true) {
        draw_screen();
        key_event();    
    }
}

/*Init
 *Initialize all variables, and mutexes,
 *creates our Termbox window,
 *sends height and width to the server,
 *gets the buffer from the server.
 */
void Notepad::init() {

    cursor_x = cursor_y = 0;
    message_length = 5;
    pthread_mutex_init(&mutex_reader, NULL);
    
    //init termbox window
    tb_init();
    tb_clear();
    tb_select_output_mode(TB_OUTPUT_NORMAL);
    
    //send height and width to server
    send_height_width();
    
    //get the buffer from the server
    get_full_buffer();
    
    tb_present();
}

/*send_height_width
 *Sends height and width to the server,
 *so the server can sync the cursors for the user
 */
void Notepad::send_height_width() {
    
    char test[100];
    //creates string a string with height and width
    sprintf(test, "INIT %d %d ", tb_height(), tb_width());
    std::string message(test);
    
    client->sendMessage(message);
}


/*get_full_buffer
 *gets the full buffer from the server
 *so a user can join whenever he wants 
 *and be able to see everything
*/
void Notepad::get_full_buffer() {

    std::string message = client->get_message();
        
    std::vector<char> first_line;
    buffer.push_back(first_line);
    
    //server send an empty message, so the buffer is empty
    if(message == "EMPTY") {  
        return;
    }
    
    //got the buffer and puts in clients buffer
    for(int i  = 0; i < message.length(); ++i) {
        if(message[i] == '\n') {
            std::vector<char> temp;
            buffer.push_back(temp);
            continue;
        }
        buffer[buffer.size()-1].push_back(message[i]);
    }

}


/*add_char_to_buffer
 *Adds a char to the clients buffer
 *@param c - the char that we will add
 *@param y - the Y position in the buffer
 *@param x - the X position in the buffer
*/
void Notepad::add_char_to_buffer(char c, int y, int x) {
    
    if(c == '\n') { 
        std::vector<char> temp;
        
        //add enter in middle of row
        if(x < buffer[y].size()) {
            while(buffer[y].size() > x ) {
                //pushing everything to the right of the cursor to the new row
                temp.push_back(buffer[y][x]);
                buffer[y].erase(buffer[y].begin() + x);  
            }
        }
        buffer.insert(buffer.begin() + y + 1, temp);
        

    } else { //add a normal char
        buffer[y].insert(buffer[y].begin()+x,c);  
    }
}


/*remove_char_from_buffer
 *Removes a char from a buffer
 */
void Notepad::remove_char_from_buffer(int y, int x) {
    
    //is this nessescary?
    if(y == -1 || x == -1) return;
    
    //move up the row to the other one if we backspace from 0,0
    if(x == 0) {
        for(int i = 0; i < buffer[y].size(); ++i) {
            buffer[y-1].push_back(buffer[y][i]);
        }
        
        //remove the row
        buffer.erase(buffer.begin() + y);
    } else {    
        if(buffer[y].size() != 0)
            buffer[y].erase(buffer[y].begin()+x-1);
        else {
            buffer.erase(buffer.begin() + y);
        }
    }
    
}

 
/*get_char
 *Gets the char that the user typed
 */
char Notepad::get_char() {
     if(keyevent.key == TB_KEY_ENTER){
        return '\n';
     }
     return keyevent.ch;
}


/*key_event
 *Sends message to the server depending on
 *what the user pressed on the keyboard
 */
void Notepad::key_event() {
    
    char typed_char;
    std::string protocol;
    std::string message;
    
    //waits for a keypress
    tb_poll_event(&keyevent);
    
    //keypress is resize
    if(keyevent.type == TB_EVENT_RESIZE) {
        char tmp[100];
        draw_screen();
        sprintf(tmp, "RESIZE %d %d ", tb_height(), tb_width());
        message = std::string(tmp);
        client->sendMessage(message);
        return;
    }
    
    switch(keyevent.key) {
        
        case TB_KEY_ESC:
            tb_shutdown();
            exit(1);
        case TB_KEY_ARROW_LEFT:
            client->sendMessage("AR_LEFT ");
            break;
            
        case TB_KEY_ARROW_RIGHT:
            client->sendMessage("AR_RIGHT ");
            break;
           
        case TB_KEY_ARROW_UP:
            client->sendMessage("AR_UP ");
            break;
            
        case TB_KEY_ARROW_DOWN:
            client->sendMessage("AR_DOWN ");
            break;
            
        case TB_KEY_BACKSPACE2:
            client->sendMessage("BS ");
            break;
        
        default:
            typed_char = get_char();
            message.push_back(typed_char);
            message.push_back(' ');
            client->sendMessage(message);
            break;
     }
}

/*write_char
 *Writes a char to a specific position in termbox
 */
void Notepad::write_char(char c,int y, int x) {
    tb_change_cell(x, y, c, TB_WHITE, 0);
    
}


/*draw_screen
 * Draws the buffer to the termbox screen
 */
void Notepad::draw_screen() {

    int cx ,cy;
    cx = cy = 0;
    tb_clear();
    
    //draws the matrix on the screen
    for(int i = 0; i < buffer.size(); ++i) {
        for(int j = 0; j < buffer[i].size(); ++j) {
            write_char(buffer[i][j], cy, cx);
            if(cx >= tb_width() - 1){  //cx> tb_width j != 0 && j % (tb_width() - 1) == 0
                cx = 0;
                cy++;   
            }else {
                cx++;  
            }   
        }
        cy++;
        cx = 0;  
    }
    tb_present();
}

/*set_cursor
 *set users cursor to a certain position
 *in the termbox window
  */
void Notepad::set_cursor(int y, int x) {
    
    tb_set_cursor(x, y);
}



/*get_data_thread
 *thread function to data from server
 */
void* Notepad::get_data_thread(void *arg) {   
    return ((Notepad*)arg)->get_data();
}

/*get_data
 *Get data from the server
 *gets a message : BY BX TY TX Y X "ACTION"
 *Where BY and BX is buffer cursors for the user,
 *Where TY and TX is the cursors on the termbox window
 *Where Y and X is where the ACTION Should happend.
 *
 *Depending on the message, it will perform an certain action
 */
void* Notepad::get_data(void) {
    
    while(true) {
        std::string whole_message = client->get_message();
        
        //std::cout<<"MESSAGE : " << whole_message << std::endl;
        //message will contain : BY BX TY TX Y X BS
        //but sometimes 2 messages are in one, so we need to split them
        std::vector<std::vector<std::string> > all_split_messages = get_all_split_messages(whole_message);
        
        //go through each message
        for(int i = 0 ; i < all_split_messages.size(); ++i) {
            
            std::string individual_messege;
            //create individual message
            for(int j = 0; j < message_length; ++j) {
                individual_messege = individual_messege + " " + all_split_messages[i][j];
            }
            
            //get the y, x position where the action should happen
            int y = atoi(&all_split_messages[i][2][0]);
            int x = atoi(&all_split_messages[i][3][0]);
            
            if(!is_handable_message(y,x)) {
                unhandled_messages.push_back(individual_messege);
                continue;
            }
    
            handle_message(all_split_messages[i]);
           
       }
       check_unhandled_messages();
       draw_screen();
      
    }
}

/*get_all_split_messages
 *Split a message from the server into several messages.
 *This is used because server might add 2 messages in 1 message,
 *so we need to split them up
 */
std::vector<std::vector<std::string> > Notepad::get_all_split_messages(std::string whole_message) {
    
    std::vector<std::vector<std::string> > all_split_messages;
    std::vector<std::string> protocol_vec = split_protocol(whole_message, ' ');
    
    for(int i = 0; i < protocol_vec.size(); i += message_length) {
        
        std::vector<std::string> tmp_vec;
        for(int j = 0; j < message_length; ++j)
            tmp_vec.push_back(protocol_vec[i+j]);
        all_split_messages.push_back(tmp_vec);
    }
    return all_split_messages;
}

/*handle_message
 *Gets a message as an vector and updating the cursors 
 *for the user and the buffer for the user, depending on action
 */
void Notepad::handle_message(std::vector<std::string> &protocol_vec) {
    
    //protocol vec : BY BX TY TX Y X ACTION
    
    int y = atoi(&protocol_vec[2][0]);
    int x = atoi(&protocol_vec[3][0]);  
    
    cursor_y = atoi(&protocol_vec[0][0]);
    cursor_x = atoi(&protocol_vec[1][0]);       
    std::string message = protocol_vec[4];
    
    
    set_cursor(cursor_y, cursor_x);
    
   //the action is no character, only an action of moving cursors, 
   //so we dont add anything new to the buffer
   /*if(message == "AR_DOWN" || message == "AR_LEFT"
        || message == "AR_RIGHT" || message == "AR_UP" || message == "RESIZE") {
        draw_screen();
        return;
    }*/
    if(message == "MOVE_CURSOR") {
        draw_screen();
        return;
    }
    //the action is backspace, lets remove a character from the buffer
    else if(message == "BS") {
        remove_char_from_buffer(y, x);
    } else { // the action is an character, add to buffer!
        add_char_to_buffer(message[0], y, x);
    }
}

/*is_handable_message
 *If a certain message is an action outside buffer, 
 *we store that message until it may proceed
 *Returns true if its handable
 */
bool Notepad::is_handable_message(int y, int x) {
    return (y < buffer.size() && x <= buffer[y].size());
}



/*check_unhandled_messages
 *Go through all the unhandled messages and check if
 *they can be procced to the buffer
 */
void Notepad::check_unhandled_messages() {
    for(int i = 0; i < unhandled_messages.size(); ++i) {
        std::vector<std::string> protocol_vec = split_protocol(unhandled_messages[i], ' ');
        int y = atoi(&protocol_vec[2][0]);
        int x = atoi(&protocol_vec[3][0]);
        
        //removes the unhandled message and adding it to the buffer
        if(is_handable_message(y, x)){
            handle_message(protocol_vec);
            unhandled_messages.erase(unhandled_messages.begin() + i);
            i--;
        }
        
    }

}

/*
 *split the protocol on a delimiter variable, and returns an vector
 *
 */
std::vector<std::string> Notepad::split_protocol(std::string protocol, char delimiter) {
    std::vector<std::string> vec;
    for(int i = 0; i < protocol.length(); ++i) {
        if(protocol[i] == delimiter) {
            vec.push_back(protocol.substr(0, i));
            protocol = protocol.substr(i+1,protocol.length());
            i=0;
        }
    }
    if(protocol.size() != 0)
        vec.push_back(protocol);    
    return vec;
} 


int main(int argc, char* argv[]) {
    if(argc!=2){
            Notepad note("127.0.0.1", "1234");
    }else {
       Notepad note(argv[1], "1234");
    }
}
