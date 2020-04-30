#include <stdio.h>  
#include <string.h>   
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>     
#include <arpa/inet.h> 

#include<iostream>
#include <fstream>

#include <string>
#include <map>

#define BUF_SIZE 1024

using namespace std;

//data buffer of 1K  
char buffer[BUF_SIZE] = {0};

//logs file name
const string logs_file_name = "./chat_logs.txt";

//maps client socket with his username 
map<int, string> map_client_username;

//clean logs file
void cleanLogsFile(){
    ofstream logs_out(logs_file_name, ios_base::out | ios_base::binary);
    logs_out.write("", 0);
    logs_out.close();
}

//store in logs file
void addToLogs(char* m){
    ofstream logs_out(logs_file_name, ios_base::app | ios_base::binary);
    logs_out.write("\n", 1);
    logs_out.write(&m[0], strlen(m));
    logs_out.close();         
}

//send logs to a new client on connection
void sendLogsToClient(int sock_fd){
    cout<<"Sending logs to client_fd: "+to_string(sock_fd)<<endl;
    ifstream in_logs(logs_file_name, ios_base::in | ios_base::binary);
    
    in_logs.read(&buffer[0], BUF_SIZE);
    while (in_logs.gcount() > 0){
        send(sock_fd, buffer, in_logs.gcount(), 0);
        in_logs.read(&buffer[0], BUF_SIZE);
    }

    in_logs.close();
}

//test command according to string
bool checkCommand(int length, string command, int valread, char* buffer){
    //this check is unnecessary, but will increase performance
    if(valread==length){
        //this should be already done, still doing again
        buffer[valread] = '\0';

        string s = buffer;
        if(s==command){
            return true;
        }else{
            return false;
        }
    }
    return false;
}

//return true if this command is stored in buffer: "/send file"
bool isIncomingFile(int valread, char* buffer){
    return checkCommand(10, "/send file", valread, buffer);
}

//return true if this command is stored in buffer: "/file sent"
bool isFileRecieved(int valread, char* buffer){
    return checkCommand(10, "/file sent", valread, buffer);
}

//return true if this command is stored in buffer: "/request file"
bool isFileRequested(int valread, char* buffer){
    return checkCommand(13, "/request file", valread, buffer);
}

//NOTE: PORT must come as argv[1]
int main(int argc, char** argv){
    const int PORT = atoi(argv[1]);

    int opt = 1;   
    int master_socket, addrlen, new_socket, client_sockets[30] = {0}, max_clients = 30;
    int activity, valread;   
    int sd, max_sd;   //socket descriptor
    struct sockaddr_in address;   

    //clean logs
    cleanLogsFile();
              
    //create a master socket to accept new connections from clients
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)   
    {   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }   
     
    //set master socket options (allow multiple connections)
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )   
    {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    }   
     
    //setup address struct
    address.sin_family = AF_INET;   
    address.sin_addr.s_addr = INADDR_ANY;   
    address.sin_port = htons( PORT );   

    addrlen = sizeof(address);   
         
    //bind the socket to localhost port PORT  
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)   
    {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }
              
    //try to specify maximum of 3 pending connections for the master socket  
    if (listen(master_socket, 3) < 0)   
    {   
        perror("listen");   
        exit(EXIT_FAILURE);   
    }   
         
    //inform server status
    printf("Listening on port %d.\nWaiting for connections...\n", PORT);

    //set of socket descriptors used in select function (to handle simultaneous incoming requests)
    fd_set readfds;
    
    while(true)
    {   
        //clear set and add master_socket
        FD_ZERO(&readfds);
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;
             
        //add child sockets to set
        for ( int i = 0 ; i < max_clients ; i++)   
        {   
            sd = client_sockets[i];   
                 
            //check validity of socket desc.
            if(sd > 0)   
                FD_SET( sd , &readfds);   
                 
            //get highest file descriptor number for select function  
            if(sd > max_sd)   
                max_sd = sd;   
        }   
     
        //wait for an activity on one of the sockets , timeout is NULL (wait indefinitely)
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);   
       
        if ((activity < 0) && (errno!=EINTR))
            puts("select error"); 

            
        //This msg_string will be used later to inform all clients about the activity as required.
        string msg_string; 
             
        //request at master socket -> accept request
        if (FD_ISSET(master_socket, &readfds))   
        {   
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)   
            {   
                perror("accept");   
                exit(EXIT_FAILURE);   
            }   
             
            //send greeting and inform client of its socket_fd number (which serves as its name)
            char greeting[BUF_SIZE];
            sprintf(greeting, "Successfully connected with server.\nYour socket_fd number is %d.\n", new_socket);  
            send(new_socket, greeting, strlen(greeting), 0) != strlen(greeting);
                 
            //add new socket to array of sockets  
            for (int i = 0; i < max_clients; i++)   
            {   
                //if position is empty  
                if(client_sockets[i] == 0)   
                {   
                    client_sockets[i] = new_socket;   
                    break;   
                }   
            }   

            //get the new client username and inform all other clients
            int valread = read(new_socket, buffer, BUF_SIZE);
            buffer[valread] = '\0';

            //add new client to map
            map_client_username[new_socket] = buffer;

            //used for sending message to other clients
            sd = new_socket;
            msg_string = map_client_username[new_socket] + ": I have joined this chat room.";

            //print new connection details
            printf("New connection details: socket_fd is %d , ip is : %s , port : %d\n, username: %s.\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port), buffer);

            //send current logs to the new client
            sendLogsToClient(new_socket);
        }   

        else{ 
            //else its some IO operation on some other socket 
            for (int i = 0; i < max_clients; i++)   
            {                       
                //this will satisfy only once int the whole for loop
                if (FD_ISSET(client_sockets[i] , &readfds))   
                {   
                    //sd should be correct socket on which the IO operation has happened
                    sd = client_sockets[i];   

                    //Read the incoming message  
                    valread = read( sd , buffer, BUF_SIZE);

                    //mark string termination in buffer
                    buffer[valread] = '\0';

                    //Check for connection closing request 
                    if (valread == 0)   
                    {   
                        //Somebody disconnected , get his details and print  
                        printf("Client disconnected with socket_fd %d.", sd);   
                            
                        //Close the socket and mark as 0 in list for reuse  
                        close(sd);
                        client_sockets[i] = 0;  

                        //to tell all other clients
                        msg_string = map_client_username[sd] + ": Left chat room.";

                        //delete entry from map
                        map_client_username.erase(sd);
                    }
                        
                    else
                    {   
                        //print incoming message details
                        printf("Recieved message: %s, from socket_fd: %d\n", buffer, sd);

                        //A client is sending a file to server
                        if(isIncomingFile(valread, buffer)){
                            //firstly get the file name
                            char file_name[BUF_SIZE];
                            valread = read(sd, file_name, BUF_SIZE);
                            file_name[valread] = '\0';

                            //copy required as file_name is lost while writing file
                            string file_name_copy = file_name;

                            //checks for error on client side in which it may disconnect
                            if(valread!=0){
                                ofstream out(file_name, ios_base::out | ios_base::binary);

                                //start reading file data from socket and write in a new file
                                while(true) {
                                    valread = read(sd, buffer, BUF_SIZE);
                                    buffer[valread] = '\0';

                                    //recieved command telling file recieved completely in previous iteration
                                    if(isFileRecieved(valread, buffer)){
                                        break;
                                    }else{
                                        out.write(&buffer[0], BUF_SIZE);
                                    }
                                }        

                                out.close(); 
                            } 

                            msg_string = map_client_username[sd]+": sent file - " + file_name_copy;
                            cout<<"Recieved file " + file_name_copy + " from client: " + to_string(sd)<<endl;
                        }

                        //A client has requested a file from server
                        else if(isFileRequested(valread, buffer)){
                            //firstly get the file name of file being requested
                            char file_name[BUF_SIZE];
                            valread = read(sd, file_name, BUF_SIZE);
                            file_name[valread] = '\0';

                            //copy required as file_name is lost while writing file
                            string file_name_copy = file_name;

                            //for reading file
                            ifstream in(file_name, ios_base::in | ios_base::binary);

                            in.read(&buffer[0], BUF_SIZE);
                            while (in.gcount() > 0){
                                send(sd, buffer, in.gcount(), 0);
                                in.read(&buffer[0], BUF_SIZE);
                            }

                            in.close();

                            //Inform Client that file sent completely
                            string sent_file_command = "/file sent";
                            send(sd, (char *)sent_file_command.c_str(), strlen((char *)sent_file_command.c_str()), 0);

                            cout<<"Sent file " + file_name_copy + " to client: " + to_string(sd)<<endl;
                        }
                        
                        //A client has sent a text message
                        else{
                            msg_string = map_client_username[sd]+": "+buffer;
                        }
                    }   

                    //as the for loop should satisty FD_ISSET only once
                    break;
                }   
            }
        }

        //Inform all clients about the activity, i.e. send them msg_string (as required)
        char* msg = (char *)msg_string.c_str();

        //store this message in logs.txt
        addToLogs(msg);

        //send incoming message details to all clients
        //socket_fd (serves as client name) is added in string showing from which client the request has been recieved
        for(int j=0; j<max_clients; j++){
            int temp_sd = client_sockets[j];

            if(temp_sd != sd && temp_sd!=0){
                send(temp_sd, msg, strlen(msg), 0 );   
            }
        }  
    }   
         
    return 0;   
}   