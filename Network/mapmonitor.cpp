/*Mapmonitor is a monitor class for a std::map which
 *is implemented with Reader/Writers paradigm.
 *This class is used to store all the ClientHandlers on
 *the server.
 *
 */
#include "mapmonitor.h"

namespace ps {
    Mapmonitor::Mapmonitor() {
        init();
    }

    void Mapmonitor::init() {
        pthread_mutex_init(&mutex, NULL);
        pthread_mutex_init(&entry, NULL);
        pthread_cond_init(&cond, NULL);
        readers = writers = 0;
    }
    /*We request a read in the monitor
     */
    void Mapmonitor::request_read() {
        pthread_mutex_lock(&entry);

        pthread_mutex_lock(&mutex);
        while(writers != 0) { //there is a writer inside monitor, so lets wait
            pthread_cond_wait(&cond,&mutex);
        }
        readers++;
        pthread_mutex_unlock(&mutex);
        pthread_mutex_unlock(&entry);
    }
    /*Request a write in the monitor
     */
    void Mapmonitor::request_write() {
        pthread_mutex_lock(&entry);
    
        pthread_mutex_lock(&mutex);
        while(writers != 0 || readers != 0) { // there is a reader or a writer inside monitor. Lets wait!
            pthread_cond_wait(&cond, &mutex);
        }
        writers++;
        pthread_mutex_unlock(&mutex);
        pthread_mutex_unlock(&entry);
    }
    /*We are done reading, so lets release a read
     */
    void Mapmonitor::release_read() {
        pthread_mutex_lock(&mutex);
        readers--;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);

    }
    /*We are done writing, so lets release a write
     */
    void Mapmonitor::release_write() {
        pthread_mutex_lock(&mutex);
        writers--;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    ClientHandler* Mapmonitor::get_clienthandler(int id) {
        return the_map[id];

    }
    /*Removes the clienthandler with the specific id
     */
    void Mapmonitor::remove_from_map(int id) {
        ClientHandler *cp = the_map[id];
        the_map.erase(id);
        delete cp;

    }
    /*adds to the map a clienthandler, with a special id as key
     */
    void Mapmonitor::add_to_map(int id, ClientHandler *client) {
        the_map[id] = client;
    }

    /*Returns the map as a vector.
     *Easier to iterate through.
     */
    std::vector<ClientHandler*> Mapmonitor::get_map_as_vector() {
        std::vector<ClientHandler*> tmp_vec;
        for(std::map<int,ClientHandler*>::iterator it = the_map.begin(); it != the_map.end(); it++) {
            tmp_vec.push_back(it->second);
        } 
        return tmp_vec;
    }
}
