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
using namespace std;
//Client side
int main(int argc, char *argv[])
{
    //we need 2 things: ip address and port number, in that order
    if(argc != 3)
    {
        cerr << "Usage: ip_address port" << endl; exit(0); 
    } //grab the IP address and port number 
    char *serverIp = argv[1]; int port = atoi(argv[2]); 
    //create a message buffer 
    char msg[1500]; 

    char GotFileSize[1024];
    long SizeCheck = 0;
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

    
    cout << ">";
    string data;
    getline(cin, data);
    memset(&msg, 0, sizeof(msg));//clear the buffer
    strcpy(msg, data.c_str());
    send(clientSd, (char*)&msg, strlen(msg), 0);
    //cout << "Awaiting file size..." << endl;
    recv(clientSd, GotFileSize, 1024, 0);
    cout<<GotFileSize<<endl;
    
    long FileSize = atoi(GotFileSize);
    //cout<<"Got file size : "<<FileSize<<endl;
    FILE *fp = fopen("MyFile.txt", "w");
    char mfcc[1500];
    while(SizeCheck<FileSize){
        int Received = recv(clientSd, mfcc, 1499, 0);
        fwrite(mfcc, 1, Received, fp);
        SizeCheck = SizeCheck + Received;
        printf("Filesize: %li\nSizecheck: %li\nReceived: %d\n\n", FileSize, SizeCheck, Received);
    }
    fclose(fp);

    close(clientSd);
    cout << "********Session********" << endl;
    cout << "Connection closed" << endl;
    return 0;    
}