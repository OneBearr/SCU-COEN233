#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

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
    // Create socket:
    int socket_desc;
    struct sockaddr_in server_addr, client_addr;
    int server_struct_length = sizeof(server_addr);
    // clean memory for addrs
    memset((char *) &server_addr, '\0', sizeof(server_addr));
    memset((char *) &client_addr, '\0', sizeof(client_addr));

    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(socket_desc < 0){
        printf("Client: Error while creating socket\n");
        return -1;
    }
    printf("Client: Socket created successfully. \n\n");
    
    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1000);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    // setup the timer;
    struct pollfd pfdsock;
    pfdsock.fd = socket_desc;
    pfdsock.events = POLLIN;
    
    // array for packets
    data_packet data_arr[PACKET_NUM];

    for (int i = 0; i < PACKET_NUM; i++) {
        
        data_arr[i].start_id = START_ID;
        data_arr[i].client_id = CLIENT_ID_MAX;
        data_arr[i].data = DATA;
        data_arr[i].seg_num = (char) i;
        data_arr[i].end_id = END_ID;
        data_arr[i].length = LENGTH;
        
        // Simulate the 4 error cases
        // Error case 1: Packets not in sequence
        if (i == 1) {
            data_arr[i].seg_num = (char) 2;
        }
        // Error case 2: length field does not match payload length
        else if (i == 2) {
            data_arr[i].length = 0x03;
        }
        // Error case 3: end of packet identifier invalid
        else if (i == 3) {
            data_arr[i].end_id = 0xFFF0;
        }
        // Error case 4: duplicate packets
        else if (i == 4) {
            data_arr[i].seg_num = (char) 3;
        }
        
        // Send the message to server
        printf("Client: Start sending packet %d. Attempt 1 out of 3\n", data_arr[i].seg_num);
        if(sendto(socket_desc, &data_arr[i], sizeof(data_packet), 0, (struct sockaddr*)&server_addr, server_struct_length) < 0){
            printf("Client: Unable to send message\n");
        } 
        else {
            printf("Client: Data packet %d has been sent.\n", data_arr[i].seg_num);
        }

        resp_packet response;
        // Ack timer implementation
        int send_attempt = 1;
        int poll_result;
        while (send_attempt <= 3) {
            // poll for 3 secs, and resend up to 2 times
            poll_result = poll(&pfdsock, 1, 3000);
            // run out of 3 attempts
            if (send_attempt == 3 && poll_result == 0) {
                printf("Client: Server does not response. Time-out Error\n");
                break;
            }
            if(poll_result == 0) {
                // time out, resend
                send_attempt++;
                printf("Client: Server no response. Retransmitting. Attempt %d out of 3\n", send_attempt);
                if(sendto(socket_desc, &data_arr[i], sizeof(data_packet), 0, (struct sockaddr*)&server_addr, server_struct_length) < 0){
                    printf("Client: Unable to send message\n");
                }
            } else { // Now receive a response from server
                if(recvfrom(socket_desc, &response, sizeof(resp_packet), 0,
                    (struct sockaddr*)&server_addr, &server_struct_length) < 0){
                    printf("Client: Error while receiving server's msg\n");
                    return -1;
                } else {
                    printf("Client: Packet %d Ack from the server has received\n\n", response.rcvd_seg_num);
                    // now check the response from the server if any errors
                    // Error case 1: Packets not in sequence
                    if (response.rej_sub_code  == (short) OUT_OF_SEQ) {
                        printf("Error: Packet %d out of sequence. Should be %d but %d.\n\n", response.rcvd_seg_num, i, response.rcvd_seg_num);
                    }
                    // Error case 2: length field does not match payload length
                    else if (response.rej_sub_code == (short) LENGTH_MISMATCH) {
                        printf("Error: Packet %d payload length mismatch.\n\n", response.rcvd_seg_num);
                    }
                    // Error case 3: end of packet identifier invalid
                    else if (response.rej_sub_code == (short) END_OF_PACKET_MISSING) {
                        printf("Error: Packet %d end of packet identifier missing.\n\n", response.rcvd_seg_num);
                    }
                    // Error case 4: duplicate packets
                    else if (response.rej_sub_code == (short) DUP_PACKET) {
                        printf("Error: Packet %d is duplicate. Should be %d but %d.\n\n", response.rcvd_seg_num, i, response.rcvd_seg_num);
                    }
                    break;
                }
            }
        }
        if (send_attempt == 3 && poll_result == 0) {
            break;
        }
        // timer and retransmission part finished
    }
    // Close the socket:
    close(socket_desc);
    return 0;
}
