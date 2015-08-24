#include "client.h"

int main(int argc, char* argv[]){

    if(argc!=2){
            std::cerr << "insert clientip" <<std::endl;
            return 1;
        }
   
     Client client(argv[1], "1234");
     
}