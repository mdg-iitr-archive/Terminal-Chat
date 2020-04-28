#include <unistd.h>
#include <string.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 

#include <iostream>
#include <string>
#include <fstream>

#define PORT 3001
#define BUF_SIZE 1024

//test command according to string
bool checkCommand(int length, std::string command, int valread, char* buffer){
    //this check is unnecessary, but will increase performance
    if(valread==length){
        //this should be already done, still doing again
        buffer[valread] = '\0';

        std::string s = buffer;
        if(s==command){
            return true;
        }else{
            return false;
        }
    }
    return false;
}

//return true if this command is stored in buffer: "/file sent"
bool isFileRecieved(int valread, char* buffer){
    return checkCommand(10, "/file sent", valread, buffer);
}

int main(void) {
    int client_sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE] = {0};

    if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form 
    if (inet_pton(AF_INET, "127.0.0.1", & serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    puts("Connecting to server...");

    if (connect(client_sock, (struct sockaddr * ) & serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    //set of file descriptor for reading
    fd_set rfds;

    //Wait for 0 secs to get input (so that messages can be sent and recieved without pausing the code (in realtime))
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    //activity returned by select function
    int activity;

    while (true) {

        FD_ZERO(&rfds);
        //file desc for stdin
        FD_SET(0, &rfds);
        //file desc for socket
        FD_SET(client_sock, &rfds);

        activity = select(client_sock+1, & rfds, NULL, NULL, & tv);

        if (activity == -1)
            perror("select()");
        
        //check if something happened in stdin (user input) file descriptor -> read stdin and send to server
        else if (FD_ISSET(0, &rfds)) {
            std::string s;
            getline(std::cin, s);

            //user wants to exit
            if (s == "exit") {
                std::cout << "Closing Socket..." << std::endl;
                close(client_sock);
                break;
            }

            //user wants to send file
            else if(s=="/send file"){
                //Inform server that you want to send a file
                char* request = (char * )(s.c_str());
                send(client_sock, request, strlen(request), 0);

                //Send file name to server
                std::cout<<"Enter file name."<<std::endl;
                getline(std::cin, s);
                request = (char * )(s.c_str());
                send(client_sock, request, strlen(request), 0);

                std::cout<<"Sending File...\n";

                //Start sending file to the server, request contains file name at tha moment
                std::ifstream in(request, std::ios_base::in | std::ios_base::binary);

                do{
                    in.read(&buffer[0], BUF_SIZE);      // Read at most n bytes into
                    send(client_sock, buffer, BUF_SIZE, 0);
                } while (in.gcount() > 0);  

                in.close();

                //Inform server that file has been sent successfully
                s = "/file sent";
                request = (char * )(s.c_str());
                send(client_sock, request, strlen(request), 0);

                std::cout<<"File Sent!\n\n";
            }

            //user wants to recieve file
            else if(s=="/request file"){
                //Inform server that you want to recieve a file
                char* request = (char * )(s.c_str());
                send(client_sock, request, strlen(request), 0);

                //Send file name to server that you want to recieve
                std::cout<<"Enter file name to get from server."<<std::endl;
                getline(std::cin, s);
                request = (char * )(s.c_str());
                send(client_sock, request, strlen(request), 0);

                std::cout<<"Requesting File...\n";

                //Start recieving file from the server, request contains file name at tha moment and the recieved file will be stored there
                std::ofstream out(request, std::ios_base::out | std::ios_base::binary);

                //start reading file data from socket and write in a new file
                while(true) {
                    int valread = read(client_sock, buffer, BUF_SIZE);
                    buffer[valread] = '\0';

                    //recieved command telling file recieved completely in previous iteration
                    if(isFileRecieved(valread, buffer)){
                        break;
                    }else{
                        out.write(&buffer[0], BUF_SIZE);
                    }
                }        

                out.close(); 

                std::cout<<"File Recieved!\n\n";
            }

            //user wants to send a normal text message
            else{
                char * request = (char * )(s.c_str());
                send(client_sock, request, strlen(request), 0);
            }
        }
        
        //check if something happened in socket file descriptor -> recieved a message
        else if(FD_ISSET(client_sock, &rfds))
        {
            int valread = read(client_sock, buffer, BUF_SIZE);

            //server disconnected
            if(valread==0){
                puts("Server Disconnected.\n");
                return 0;
            }

            //mark string termination in buffer
            buffer[valread] = '\0';
            
            printf("%s\n",buffer);

            // *buffer = {0};
        }
    }

    return 0;
}