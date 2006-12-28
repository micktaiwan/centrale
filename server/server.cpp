#include <winsock2.h>
#include <iostream>
using namespace std;

int Server() {
    cout << "Starting up TCP server\n";

    //A SOCKET is simply a typedef for an unsigned int.
    //In Unix, socket handles were just about same as file
    //handles which were again unsigned ints.
    //Since this cannot be entirely true under Windows
    //a new data type called SOCKET was defined.
    SOCKET server;

    //WSADATA is a struct that is filled up by the call
    //to WSAStartup
    WSADATA wsaData;

    //The sockaddr_in specifies the address of the socket
    //for TCP/IP sockets. Other protocols use similar structures.
    sockaddr_in local;

    //WSAStartup initializes the program for calling WinSock.
    //The first parameter specifies the highest version of the
    //WinSock specification, the program is allowed to use.
    int wsaret=WSAStartup(0x101,&wsaData);

    //WSAStartup returns zero on success.
    //If it fails we exit.
    if(wsaret!=0)
    {
        return 0;
    }

    //Now we populate the sockaddr_in structure
    local.sin_family=AF_INET; //Address family
    local.sin_addr.s_addr=INADDR_ANY; //Wild card IP address
    local.sin_port=htons((u_short)20248); //port to use

    //the socket function creates our SOCKET
    server=socket(AF_INET,SOCK_STREAM,0);

    //If the socket() function fails we exit
    if(server==INVALID_SOCKET)
    {
        return 0;
    }

    //bind links the socket we just created with the sockaddr_in
    //structure. Basically it connects the socket with
    //the local address and a specified port.
    //If it returns non-zero quit, as this indicates error
    if(bind(server,(sockaddr*)&local,sizeof(local))!=0)
    {
        return 0;
    }

    //listen instructs the socket to listen for incoming
    //connections from clients. The second arg is the backlog
    if(listen(server,10)!=0)
    {
        return 0;
    }

    //we will need variables to hold the client socket.
    //thus we declare them here.
    SOCKET client;
    sockaddr_in from;
    int fromlen=sizeof(from);

    while(true)//we are looping endlessly
    {
        char temp[512];

        //accept() will accept an incoming
        //client connection
        client=accept(server,
            (struct sockaddr*)&from,&fromlen);

        sprintf(temp,"Your IP is %s\r\n",inet_ntoa(from.sin_addr));

        //we simply send this string to the client
        send(client,temp,strlen(temp),0);
        cout << "Connection from " << inet_ntoa(from.sin_addr) <<"\r\n";

        //close the client socket
        closesocket(client);

    }

    //closesocket() closes the socket and releases the socket descriptor
    closesocket(server);

    //originally this function probably had some use
    //currently this is just for backward compatibility
    //but it is safer to call it as I still believe some
    //implementations use this to terminate use of WS2_32.DLL
    WSACleanup();

    }
