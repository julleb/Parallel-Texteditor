windows : client.o  
	g++ client.o termbox/hej/usr/local/bin/cygtermbox-1.dll note.cpp -o note -lboost_system -lpthread

linux : client.o  
	g++ client.o note.cpp -o note -lboost_system -lpthread -ltermbox

    
    
server.o : clienthandler.o mapmonitor.o Network/server.cpp
	g++ -c clienthandler.o mapmonitor.o Network/server.cpp -fopenmp -lboost_system -lpthread
    
    
test_server : mapmonitor.o server.o Network/test_server.cpp
	g++ clienthandler.o mapmonitor.o server.o Network/test_server.cpp -o test_server -fopenmp -lboost_system -lpthread
    
    
client.o : Network/client.cpp
	g++ -c Network/client.cpp -lboost_system


clienthandler.o : Network/clienthandler.cpp
	g++ -c Network/clienthandler.cpp -lboost_system

mapmonitor.o : clienthandler.o Network/mapmonitor.cpp
	g++ -c clienthandler.o Network/mapmonitor.cpp -lpthread
