#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <errno.h>
#include <sstream>
using namespace std;
//Server side

void filePutContents(const string& name, const string& content, bool append);

int main(int argc, char *argv[])
{
    //for the server, we only need to specify a port number
    if(argc != 2)
    {
        cerr << "Usage: port" << endl;
        exit(0);
    }
    //grab the port number
    int port = atoi(argv[1]);
    //buffer to send and receive messages with
    char buffer[1025];
    char mess[2050];
    int SizeCheck=0;
    char mfcc[1500];
    //char *message = "ECHO Daemon v1.0 \r\n";
    fstream MyFile;
    MyFile.open("ChatBackup.txt", ios::in | ios::out | ios::trunc);
    MyFile << "\n";
    MyFile.close();

    int client_socket[30], activity, sd, max_sd, valread, new_socket, addrlen, i, sd1, sd2;
    int max_clients = 30;
    vector<string> names;

    for (int i = 0; i < max_clients; i++)   
    {   
        names.push_back("Anonymous");   
    }

    fd_set readfds;

    for (int i = 0; i < max_clients; i++)   
    {   
        client_socket[i] = 0;   
    }
     
    //setup a socket and connection tools
    sockaddr_in servAddr;
    bzero((char*)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);
 
    //open stream oriented socket with internet address
    //also keep track of the socket descriptor
    int serverSd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSd < 0)
    {
        cerr << "Error establishing the server socket" << endl;
        exit(0);
    }

    //set master socket to allow multiple connections
    if( setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, (char *)&port, sizeof(port)) < 0 )   
    {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    }

    //bind the socket to its local address
    int bindStatus = bind(serverSd, (struct sockaddr*) &servAddr, 
        sizeof(servAddr));
    if(bindStatus < 0)
    {
        cerr << "Error binding socket to local address" << endl;
        exit(0);
    }
    bool flag = false;

    if (listen(serverSd, 5) < 0)   
    {   
        perror("listen");
        exit(EXIT_FAILURE);   
    }
    addrlen = sizeof(servAddr); 
    cout << "Waiting for a client to connect..." << endl;

    while(1)
    {


        //clear the socket set  
        FD_ZERO(&readfds);   
     
        //add master socket to set  
        FD_SET(serverSd, &readfds);   
        max_sd = serverSd;   
             
        //add child sockets to set  
        for ( i = 0 ; i < max_clients ; i++)   
        {   
            //socket descriptor  
            sd = client_socket[i];   
                 
            //if valid socket descriptor then add to read list  
            if(sd > 0)   
                FD_SET( sd , &readfds);   
                 
            //highest file descriptor number, need it for the select function  
            if(sd > max_sd)   
                max_sd = sd;   
        }   
     
        //wait for an activity on one of the sockets , timeout is NULL ,  
        //so wait indefinitely  
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);   
       
        if ((activity < 0) && (errno!=EINTR))   
        {   
            printf("select error");   
        }   
             
        //If something happened on the master socket ,  
        //then its an incoming connection  
        if (FD_ISSET(serverSd, &readfds))   
        {   
            if ((new_socket = accept(serverSd,  
                    (struct sockaddr *)&servAddr, (socklen_t*)&addrlen))<0)   
            {   
                perror("accept");   
                exit(EXIT_FAILURE);   
            }   
             
            //inform user of socket number - used in send and receive commands  
            printf("New connection , socket fd is %d , ip is : %s , port : %d " , new_socket , inet_ntoa(servAddr.sin_addr) , ntohs(servAddr.sin_port));  
            printf("\n");
            memset(&buffer, 0, sizeof(buffer)); //clear the buffer
            recv(new_socket, (char*)&buffer, sizeof(buffer), 0);
            //cout<<buffer<<endl;
                 
            //add new socket to array of sockets  
            for (i = 0; i < max_clients; i++)   
            {   
                //if position is empty  
                if( client_socket[i] == 0 )   
                {   
                    client_socket[i] = new_socket;   
                    printf("Adding to list of sockets as %d\n" , i);   
                    names[i] = buffer;
                    //names.push_back(buffer);
                    //names[i].assign(buffer, find(buffer, buffer + 4, '\0'));
                         
                    break;   
                }   
            }   

            FILE *fp;
            fp = fopen("ChatBackup.txt","r");
            fseek(fp, 0, SEEK_END);
            long FileSize = ftell(fp);
            //cout<<FileSize<<endl;
            fclose(fp);
            FILE *fpn;
            fpn = fopen("ChatBackup.txt","r");
            memset(&buffer, 0, sizeof(buffer)); //clear the buffer
            sprintf(buffer, "%li", FileSize);
            //cout<<buffer<<endl;
            send(new_socket, (char*)&buffer, strlen(buffer), 0);
            while(SizeCheck<FileSize){
                memset(&mfcc, 0, sizeof(mfcc));//clear the buffer
                int Read = fread(mfcc, sizeof(char), 1500, fpn);
                //cout<<Read<<endl;
                send(new_socket, mfcc, Read, 0);
                SizeCheck = SizeCheck + Read;
                //cout<<SizeCheck<<endl;
            }
            fclose(fpn);
            //close(new_socket);
        } 
        //close(new_socket);


        //else its some IO operation on some other socket 
        for (i = 0; i < max_clients; i++)   
        {   
            sd1 = client_socket[i];   
                 
            if (FD_ISSET( sd1 , &readfds))   
            {   
                memset(&buffer, 0, sizeof(buffer)); //clear the buffer
                //Check if it was for closing , and also read the  
                //incoming message  
                if ((valread = recv( sd1 , buffer, 1024, 0)) == 0)   
                {   
                    //Somebody disconnected , get his details and print  
                    getpeername(sd1 , (struct sockaddr*)&servAddr , \ 
                        (socklen_t*)&addrlen);   
                    printf("Host disconnected , ip %s , port %d \n" ,  
                          inet_ntoa(servAddr.sin_addr) , ntohs(servAddr.sin_port));   
                         
                    //Close the socket and mark as 0 in list for reuse  
                    close( sd1 );   
                    client_socket[i] = 0;   
                }   
                     
                //Echo back the message that came in  
                else 
                {   
                    memset(&mess, 0, sizeof(mess)); //clear the buffer
                    // strcat(mess, "Client_");
                    // strcat(mess, to_string(i+1).c_str());
                    strcat(mess, names[i].c_str());
                    strcat(mess, " : ");
                    strcat(mess, buffer);
                    cout <<names[i]<<" : " << buffer << endl;
                    filePutContents("ChatBackup.txt",mess,true);
                    filePutContents("ChatBackup.txt","\n",true);
                    for (int j = 0; j < max_clients; j++)
                    {
                        sd2 = client_socket[j];
                        send(sd2 , mess , strlen(mess) , 0 );
                    }
                    //close(sd1);
                }   
            }
        }   
    } 
    close(sd2);
    close(sd);
    close(sd1);
    //close(new_socket);
    close(serverSd);
    return 0;   
}

void filePutContents(const string& name, const string& content, bool append = false) {
    ofstream outfile;
    if (append)
        outfile.open(name, ios_base::app);
    else
        outfile.open(name);
    outfile << content;
}