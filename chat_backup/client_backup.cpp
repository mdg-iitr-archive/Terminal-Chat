#include <iostream>
#include <string>
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
using namespace std;
//Client side
int main(int argc, char *argv[])
{
    //we need 2 things: ip address and port number, in that order
    if(argc != 4)
    {
        cerr << "Usage: ip_address port name" << endl; exit(0); 
    } //grab the IP address and port number 
    char *serverIp = argv[1]; int port = atoi(argv[2]); 
    char* name = argv[3];
    //cout<<name<<endl;
    //create a message buffer 
    char msg[1500];

    char GotFileSize[1024];
    long SizeCheck = 0;

    fd_set readfds;
    int max_sd, activity;
    //setup a socket and connection tools 
    struct hostent* host = gethostbyname(serverIp); 
    sockaddr_in sendSockAddr;   
    bzero((char*)&sendSockAddr, sizeof(sendSockAddr)); 
    sendSockAddr.sin_family = AF_INET; 
    sendSockAddr.sin_addr.s_addr = 
        inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
    sendSockAddr.sin_port = htons(port);
    int clientSd = socket(AF_INET, SOCK_STREAM, 0);
    //try to connect...
    int status = connect(clientSd,
                         (sockaddr*) &sendSockAddr, sizeof(sendSockAddr));
    if(status < 0)
    {
        cout<<"Error connecting to socket!"<<endl;
    }
    cout << "Connected to the server!" << endl;
    send(clientSd, name, strlen(name), 0);
    recv(clientSd, GotFileSize, 1024, 0);
    //cout<<GotFileSize<<endl;
    long FileSize = atoi(GotFileSize);
    char mfcc[1500];
    while(SizeCheck<FileSize){
        memset(&mfcc, 0, sizeof(mfcc));//clear the buffer
        int Received = recv(clientSd, mfcc, 1499, 0);
        //cout<<Received<<endl;
        cout<< mfcc;
        SizeCheck = SizeCheck + Received;
        //cout<<SizeCheck<<endl;
    }

    while(1)
    {

        FD_ZERO(&readfds);
        FD_SET(clientSd, &readfds);
        FD_SET(0, &readfds);
        max_sd = clientSd;

        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);   
       
        if ((activity < 0) && (errno!=EINTR))   
        {   
            printf("select error");   
        }   

        if (FD_ISSET(0, &readfds))   
        {   
            string data;
            getline(cin, data);
            memset(&msg, 0, sizeof(msg));//clear the buffer
            strcpy(msg, data.c_str());
            if(data == "exit")
            {
                send(clientSd, (char*)&msg, strlen(msg), 0);
                close(clientSd);
                break;
            }
            send(clientSd, (char*)&msg, strlen(msg), 0);
        } 
             
        //If something happened on the client socket ,  
        //then its an incoming message 
        else if (FD_ISSET(clientSd, &readfds))   
        {   
            memset(&msg, 0, sizeof(msg));//clear the buffer
            recv(clientSd, (char*)&msg, sizeof(msg), 0);
            cout << msg << endl;
        } 

    }
    close(clientSd);
    return 0;    
}