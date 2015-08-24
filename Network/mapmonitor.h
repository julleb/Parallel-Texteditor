#ifndef MAPMONITOR_H
#define MAPMONITOR_H

#include <pthread.h>
#include <vector>
#include <map>
#include "clienthandler.h"
namespace ps {

    class Mapmonitor {
    private:
        std::map<int, ClientHandler*> the_map;
        pthread_mutex_t entry, mutex;
        pthread_cond_t cond;
        int writers, readers;
        void init();
    public: 
        Mapmonitor();
        void request_read();
        void request_write();
        void add_to_map(int id, ClientHandler *client);
        void remove_from_map(int id);
        ClientHandler* get_clienthandler(int id);
        void release_read();
        void release_write();
        std::vector<ClientHandler*> get_map_as_vector();
    };
}

#endif
