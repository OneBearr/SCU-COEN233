#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#define DBSIZE 5           // database size
#define START_ID 0xFFFF      // start of packet identifier
#define END_ID 0xFFFF        // end of packet identifier
#define CLIENT_ID_MAX 0xFF    // max of client id (255)
#define LENGTH 0xFF         // max length(255)

#define ACCESS_PERM 0xFFF8
#define NOT_PAID 0xFFF9
#define NOT_EXIST 0xFFFA
#define ACCESS_OK 0xFFFB

#define G2 0x02
#define G3 0x03
#define G4 0x04
#define G5 0x05

// data packets, between client and server
typedef struct data_packet
{
    short start_id;     // short: 2 bytes
    char client_id;     // char: 1 byte
    short packet_type;  // access permission, not paid, not exist, access ok
    char seg_num;
    char length;
    char tech;
    unsigned long source_subscriber_num;
    short end_id;
} data_packet;

int main(void) {
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

    // Phone numbers for testing 
    // 1234567891   Exist,  Not-paid
    // 1234567892   Access Granted
    // 1234567893   Tech-mismatch, Not-Exist
    // 1234567894   Access Granted
    // 1234567895   Cannot find the number, Not-Exist   
    unsigned long nums[DBSIZE]= {1234567891, 1234567892, 1234567893, 1234567894, 1234567895};
    char techs[DBSIZE] = {G2, G3, G4, G5, G2};

    // Create data packets
    data_packet data_arr[DBSIZE];
    for (int i = 0; i < DBSIZE; i++) {
        data_arr[i].start_id = START_ID;
        data_arr[i].client_id = CLIENT_ID_MAX;
        data_arr[i].packet_type = ACCESS_PERM;
        data_arr[i].seg_num = i;
        data_arr[i].tech = techs[i];
        data_arr[i].source_subscriber_num = nums[i];
        data_arr[i].length = sizeof(data_arr[i].tech) + sizeof(data_arr[i].client_id);  // payload: tech + subsc num
        data_arr[i].end_id = END_ID;
    }

    data_packet data;
    data_packet response;

    for (int j = 0; j < DBSIZE; j++) {
        data = data_arr[j];
        printf("Client: Start sending packet %d. Attempt 1 out of 3\n", data.seg_num);
        // Send the message to server:
        if(sendto(socket_desc, &data, sizeof(data_packet), 0, (struct sockaddr*)&server_addr, server_struct_length) < 0){
            printf("Client: Unable to send message\n");
            // return -1;
        } 
        else {
            printf("Client: Data packet %d has been sent.\n", data.seg_num);
        }
        // Ack timer implementation
        int send_attempt = 1;
        int poll_result;
        while (send_attempt < 4) {
            // poll for 3 secs, and resend up to 2 times
            poll_result = poll(&pfdsock, 1, 3000);
            if (send_attempt == 3 && poll_result == 0) {
                printf("Client: Server does not response. Time-out Error\n");
                break;
            } else if(poll_result == 0) {
                // time out, resend
                send_attempt++;
                printf("Client: Server no response. Retransmitting. Attempt %d out of 3\n", send_attempt);
                if(sendto(socket_desc, &data, sizeof(data_packet), 0, (struct sockaddr*)&server_addr, server_struct_length) < 0){
                    printf("Client: Unable to send message\n");
                    // return -1;
                }
            } else { // Now receive a response from server
                if(recvfrom(socket_desc, &response, sizeof(response), 0,
                    (struct sockaddr*)&server_addr, &server_struct_length) < 0){
                    printf("Client: Error while receiving server's msg\n");
                    // return -1;
                } else {
                    printf("Client: Packet %d Ack from the server has received\n", response.seg_num);
                    // case 1 Access OK
                    if (response.packet_type == (short) ACCESS_OK) {
                        printf("Access Granted, %lu can access the network.\n\n", response.source_subscriber_num);
                    }
                    // case 2 Not Paid
                    else if (response.packet_type == (short) NOT_PAID) {
                        printf("Access Denied, %lu has not paid.\n\n", response.source_subscriber_num);
                    }
                    // case 3 Not Exist (number not found, or number found but tech not match)
                    else if (response.packet_type == (short) NOT_EXIST) {
                        printf("Access Denied, %lu does not exist.\n\n", response.source_subscriber_num);
                    } 
                    
                    break;
                }
            }
        }
        // No need to send the rest packets
        if (send_attempt == 3 && poll_result == 0) {
            break;
        }
        // timer and retransmission part finished
    }

    // Close the socket:
    close(socket_desc);
    return 0;
}