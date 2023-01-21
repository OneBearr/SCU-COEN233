Xiongsheng Yi Coen233 Computer Network Programming Assignment

/*      Programming Assignment One     */
Related Files: 
client1.c     server1.c 

To-run instruction code: 
1. Type   gcc -o filename filename.c   to compile in the terminal
2. Type   ./filename    to run

Part one:
Client sends 5 packets to the server, server sends ACK to client for each packet.
1. Comment error case 1-4 part of codes.
2. Run server1 first, start listening. Then run client1, start sending packets.

Part two:
Client sends 5 packets to the server, emulating one corrent packets and four packets with errors,
server sends ACK to client for each packet and with corresponding Reject sub codes.
1. Uncomment error case 1-4 part of codes.
2. Run server1 first, start listening. Then run client1, start sending packets.

Part three:
Use a ack_timer(3 secs), retransmite the packets if no response from server. Send the packet up to three times.
Show "server no response" if no ACK after three times transmission. 
1. Do not open(run) server1 (no listening), just run client1 (sending). 

/*      Programming Assignment Two     */
Related Files: 
client2.c     server2.c     VerificationDatabase.txt

To-run instruction code: 
1. Type   gcc -o filename filename.c   to compile in the terminal
2. Type   ./filename    to run

Part one:
The client requests access information from server, the server verifies the validity of the requiest and responses accordingly. 
1. Run server2 first, start listening. Then run client2, start sending packets.

Part two:
Use a ack_timer(3 secs), retransmite the packets if no response from server. Send the packet up to three times.
Show "server no response" if no ACK after three times transmission. 
1. Do not open(run) server2 (no listening), just run client2 (sending). 