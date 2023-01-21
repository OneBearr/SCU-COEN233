#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#define DBSIZE 5              // database size
#define START_ID 0xFFFF      // start of packet identifier
#define END_ID 0xFFFF        // end of packet identifier
#define CLIENT_ID_MAX 0xFF    // max of client id (255)
#define LENGTH 0xFF         // max length(255)

#define ACCESS_PERM 0xFFF8
#define NOT_PAID 0xFFF9
#define NOT_EXIST 0xFFFA
#define ACCESS_OK 0xFFFB

// data packets, between client and server
typedef struct data_packet
{
    short start_id;     // short: 2 bytes
    char client_id;     // char: 1 byte
    short packet_type;  // access permission, not paid, not exist, or access ok
    char seg_num;
    char length;
    char tech;
    unsigned long source_subscriber_num;
    short end_id;
} data_packet;

unsigned long subscribers[DBSIZE];
int techs[DBSIZE];
int ifPaids[DBSIZE];

int readfile(char *);

int main(void) {
    // load the subscribers database
    char* filename = "VerificationDatabase.txt";
   if (readfile(filename) < 0) {
       printf("Unable to read file.\n");
       return -1;
   }

    // Create the socket
    int socket_desc;
    struct sockaddr_in server_addr, client_addr;
    int client_struct_length = sizeof(client_addr);
    // clean memory for addrs
    memset((char *) &server_addr, '\0', sizeof(server_addr));
    memset((char *) &client_addr, '\0', sizeof(client_addr));

    // Create UDP socket:
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");
    
    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1000);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind to the set port and IP:
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Couldn't bind to the port\n");
        return -1;
    }
    printf("Done with binding\n");

    int req_num = 0;
    for (;;) {
        printf("Listening for incoming request...\n\n");
        struct data_packet data;
        struct data_packet response;
        // Receive client's message:
        int recvlen = recvfrom(socket_desc, &data, sizeof(data), 0, (struct sockaddr*)&client_addr, &client_struct_length);
        if ( recvlen < 0){
            printf("Couldn't receive\n");
            return -1;
        } else {
            printf("Server: Received message from client: \"%lu\" \n", data.source_subscriber_num);
        }

        // Prepare the response packet
        response.start_id = START_ID;
        response.client_id = CLIENT_ID_MAX;
        response.seg_num = data.seg_num;
        response.length = data.length;
        response.source_subscriber_num = data.source_subscriber_num;
        response.end_id = data.end_id;
        response.tech = data.tech;
        response.packet_type = NOT_EXIST;       // default type

        // search, find the number if the access is ok 
        for (int i = 0; i < DBSIZE; i++) {
            if (subscribers[i] == data.source_subscriber_num && techs[i] == (int) data.tech) {
                response.packet_type = NOT_PAID;
                if (ifPaids[i]) {
                    response.packet_type = ACCESS_OK;
                }
            }
        }

        // Send the response packet back to client
        if (sendto(socket_desc, &response, sizeof(data_packet), 0,
            (struct sockaddr*)&client_addr, client_struct_length) < 0){
            printf("Can't send\n");
            return -1;
        } else {
            printf("Server: Client %lu Ack has sent to the client.\n", data.source_subscriber_num);
            req_num++;
        }
        if (req_num == DBSIZE) {    // all packets ack had sent
            break;
        }
    }
    // Close the socket:
    close(socket_desc);
    return 0;
}

int readfile(char *x) {
    FILE *fptr;
    unsigned long subs_num;
    int tech;
    int ifPaid;
    if ((fptr = fopen(x, "r")) == NULL) {
        printf("Unable to open file.\n");
        return -1;
    }
    int row = 0;
    while (fscanf(fptr, "%lu %i %i", &subs_num, &tech, &ifPaid) > 0) {
        subscribers[row] = subs_num;
        techs[row] = tech;
        ifPaids[row] = ifPaid;
        row++;
    }
    fclose(fptr);
    return 1;
}

