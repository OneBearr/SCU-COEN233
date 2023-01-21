#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

// primitives
#define PACKET_NUM 5        // number of packets to send
#define START_ID 0xFFFF      // start of packet identifier
#define END_ID 0xFFFF        // end of packet identifier
#define CLIENT_ID_MAX 0xFF    // max of client id (255)
#define LENGTH 0xFF         // max length(255)
// packet types
#define DATA 0xFFF1   // data packet
#define ACK 0xFFF2    // ack packet
#define REJECT 0xFFF3 // reject packet
// reject sub codes
#define OUT_OF_SEQ 0xFFF4     // out of sequence
#define LENGTH_MISMATCH 0xFFF5 // length mismatchh
#define END_OF_PACKET_MISSING 0xFFF6 // end of packet missing
#define DUP_PACKET 0xFFF7 // duplicate packet

// data packets, from client to server
typedef struct data_packet
{
    short start_id;     // short: 2 bytes
    char client_id;     // char: 1 byte
    short data;
    char seg_num;
    char length;
    char payload[255];
    short end_id;
} data_packet;

// response packets, from server to client
typedef struct resp_packet 
{
    short start_id;
    char client_id;
    short type;
    short rej_sub_code;
    char rcvd_seg_num;
    short end_id;
} resp_packet;

int main(void){
    // Create UDP socket:
    int socket_desc;
    struct sockaddr_in server_addr, client_addr;
    int client_struct_length = sizeof(client_addr);
    // clean memory for addrs
    memset((char *) &server_addr, '\0', sizeof(server_addr));
    memset((char *) &client_addr, '\0', sizeof(client_addr));
    
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
    
    int seq_num = 0;
    for (;;) {
        printf("Listening for incoming packet %d...\n\n", seq_num);
        struct data_packet data1;
        struct resp_packet resp1;

        // Receive client's message:
        int recvlen = recvfrom(socket_desc, &data1, sizeof(data1), 0, (struct sockaddr*)&client_addr, &client_struct_length);
        if ( recvlen < 0){
            printf("Couldn't receive\n");
            return -1;
        } else {
            printf("Server: Packet %d has received from client.\n", data1.seg_num);
        }
    
        // Respond to client:
        resp1.start_id = START_ID;
        resp1.client_id = CLIENT_ID_MAX;
        resp1.type = ACK;
        resp1.rej_sub_code = 0;
        resp1.rcvd_seg_num = data1.seg_num;
        resp1.end_id = END_ID;
        
        // Error case 1: Packets not in sequence
        if (seq_num < data1.seg_num) {
            resp1.type = REJECT;
            resp1.rej_sub_code = OUT_OF_SEQ;
            printf("Error: Packet out of sequence. Should be %d but %d\n", seq_num, data1.seg_num);
        }
        // Error case 2: length field does not match payload length
        else if (data1.length != (char) sizeof(data1.payload)) {
            resp1.type = REJECT;
            resp1.rej_sub_code = LENGTH_MISMATCH;
            printf("Error: Packet payload length mismatch.\n");
        }
        // Error case 3: end of packet identifier invalid
        else if (data1.end_id != (short) END_ID) {
            resp1.type = REJECT;
            resp1.rej_sub_code = END_OF_PACKET_MISSING;
            printf("Error: Packet end identifier missing.\n");
        }
        // Error case 4: duplicate packets
        else if (seq_num > data1.seg_num) {
            resp1.type = REJECT;
            resp1.rej_sub_code = DUP_PACKET;
            printf("Error: Duplicate packets. Should be %d but %d\n", seq_num, data1.seg_num);
        }
        
        if (sendto(socket_desc, &resp1, sizeof(resp1), 0,
            (struct sockaddr*)&client_addr, client_struct_length) < 0){
            printf("Can't send\n");
            return -1;
        } else {
            printf("Server: Packet %d Ack has sent to the client.\n", data1.seg_num);
            seq_num++;
        }
        if (seq_num == PACKET_NUM) {    // all packets ack had sent
            break;
        }
    }
    // Close the socket:
    close(socket_desc);
    return 0;
}
