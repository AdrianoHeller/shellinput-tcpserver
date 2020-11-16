#include "network.h"

#if defined(_WIN32)
#include <conio.h>
#endif

int main(int argc, char* argv[]){
    // Printing number of args
    printf("%d\n",argc);
    // Checking Argument Array Size;
    printf("%ld\n", sizeof(argv[argc])/sizeof(argv[0]));
    // Checking Shell inputs
    if(argc > 1) {
        char** arg = &argv[1];
        // Parsing the IPv4 and IPv6 Loopback input addresses. Can be commented with no issues on compiling
        *arg = strcmp(*arg,"127.0.0.1") == 0 ? "IPv4 Loopback" : strcmp(*arg,"::1") == 0 ? "IPv6 Loopback" : *arg;
        printf("Remote Server Address: %s\n", *arg);
        arg++;
        printf("Port or Protocol: %s\n", *arg);
    }else{
        printf("You must specify 2 arguments: the server IP and the service PORT.\n");
    };
    //Starting the configuration
    printf("Configurating the remote address...\n");
    // Instancing the hints struct to zero the address memory
    struct addrinfo hints;
    memset(&hints,0,sizeof(hints));
    // Setting protocol to TCP
    hints.ai_socktype = SOCK_STREAM;
    // Creating pointer to receive the peer address
    struct addrinfo* peer_address;
    // Configuring the Remote Address
    if(getaddrinfo(argv[1],argv[2], &hints, &peer_address)){
        fprintf(stderr,"getaddrinfo() failed.(%d)\n",GETSOCKETERRNO());
        return 1;
    };
    // Print Remote Address to DEBUG purposes
    printf("Remote address is: ");
    // Creating 100 bytes of space to alloc address and service info
    char address_buffer[100];
    char service_buffer[100];
    // Recovering the address name to string for printing
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen, address_buffer, sizeof(address_buffer), service_buffer, sizeof(service_buffer), NI_NUMERICHOST);
    printf("%s %s\n", address_buffer, service_buffer);
    // Creating the communication socket
    SOCKET socket_peer;
    // Addressing the peer data into the socket
    socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype, peer_address->ai_protocol);
    // Checking if the socket is valid
    if(!ISVALIDSOCKET(socket_peer)){
        fprintf(stderr,"socket failed.(%d)\n",GETSOCKETERRNO());
        return 1;
    };
    // Calling connect to establish communication with the server
    printf("Connecting...\n");
    // Submitting connection to the address via socket
    if(connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)){
        fprintf(stderr,"connection failed.(%d)\n",GETSOCKETERRNO());
        return 1;
    };
    // Freeing address memory
    freeaddrinfo(peer_address);
    // Sending connection confirmation and instructions to the user
    printf("Connected...\n");
    printf("To send data, enter text and press \"ENTER\" on your keyboard...\n");
    // setting a boolean condition loop
    while(1) {
        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(socket_peer, &reads);
#if !defined(_WIN32)
        FD_SET(0, &reads);
#endif
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

        if (select(socket_peer + 1, &reads, 0, 0, &timeout) < 0) {
            fprintf(stderr, "select failed.(%d)\n", GETSOCKETERRNO());
            return 1;
        };
        // Checking if the socket is in reads
        if (FD_ISSET(socket_peer, &reads)) {
            char read[4096];
            int bytes_received = recv(socket_peer, read, 4096, 0);
            if (bytes_received < 1) {
                printf("Connection closed by peer.\n");
                break;
            };
            printf("Received (%d bytes): %.*s", bytes_received, bytes_received, read);
        };
#if defined(_WIN32)
        // Check in Windows for waiting console inputs with a proxy
        if(_kbhit()){
#else
        // Check in UNIX if an select recorded file descriptor 0
        if (FD_ISSET(0, &reads)) {
#endif
            char read[4096];
            // call fgets to read the next line of shell input
            if (!fgets(read, 4096, stdin)) break;
            printf("Sending: %s", read);
            int bytes_sent = send(socket_peer, read, strlen(read), 0);
            printf("Sent %d bytes.\n", bytes_sent);
        };
    };
    printf("Closing socket...\n");
    // Closing comm socket
    CLOSESOCKET(socket_peer);
#if defined(_WIN32)
    WSACleanup();
#endif
    printf("Program finished.\n");
    return 0;
};