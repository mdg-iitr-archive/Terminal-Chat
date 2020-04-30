#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 

#define PORT 3000

#include <iostream>
#include <thread> 
#include <string>
#include <map>
#include <vector>

using namespace std;

map<string, int> map_channel_port;
int next_port = 3001;

void startServerCmd(string ch_name){
    string cmd = "cd channels/"+ch_name+"/ && ./server.out " + to_string(map_channel_port[ch_name]);
    system(cmd.c_str());
}

void start_new_server(string ch_name){
    string cmd1 = "mkdir -p channels/"+ch_name+" && cp server.cpp channels/"+ch_name+"/";
    string cmd2 = "cd channels/"+ch_name+"/ && g++ server.cpp -o server.out";
    system(cmd1.c_str());
    system(cmd2.c_str());

    thread (startServerCmd, ch_name).detach();
}

int main(){
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0))<0){
        cout<<("\n Socket creation error \n")<<endl;
        return -1;
    }

    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	} 

    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind fail");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //inform server status
    printf("Listening on port %d.\nWaiting for connections...\n", PORT);

    int new_socket, valread, port;
    string ch_name;
    while (true)
    {
        if((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                    (socklen_t*)&addrlen))<0){
            perror("accept"); 
            exit(EXIT_FAILURE); 
        }

        valread = read(new_socket, buffer, 1024);
        ch_name = buffer;

        cout<<endl<<"Channel Requested: "+ch_name<<endl;

        if(map_channel_port.count(ch_name)){
            port = map_channel_port[ch_name];

            //in case a new server does not start, last digit of port should be 0
            port = (port*10) + 0;
        }else{
            cout<<"Starting new server for channel: "+ch_name<<endl; 
            port = next_port;
            next_port++;
            map_channel_port[ch_name]=port;
            start_new_server(ch_name);

            //in case a new server start, last digit of port should be 1
            port = (port*10) + 1;
        }

        cout<<"Port number: "+to_string(port)<<endl<<endl;
        send(new_socket, to_string(port).c_str(), strlen(to_string(port).c_str()), 0);
    }

    return 0;
}