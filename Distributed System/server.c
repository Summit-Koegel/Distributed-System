#include<unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "server_functions.h"
#include "udp.h"
#include <string.h>


struct message{
    int                key;
    int              value;
    int               time;
    int                ack;
    int         seq_number;
    int          client_id;
    char*          request;
    struct socket   socket;
};

int main(int argc, char const *argv[])
{
    if( argc != 2){
        printf("Incorrect number of arguments\n");
        return 1;
    }
    if (argv[1] == NULL){
        printf("Must give socket for server\n");
        return 1;
    }

    struct socket server_sock = init_socket(atoi(argv[1]));

    
    while(1){
        struct packet_info packet = receive_packet(server_sock);
        struct message client_message = *(struct message *)&packet.buf;
        int last = client_message.seq_number;
        struct message server_message;

        // received packet
        if (packet.recv_len != 0){
            printf("%s\n",client_message.request);
            if(strcmp(client_message.request, "get") == 0){
                if(client_message.seq_number > last){
                    server_message.value = get(client_message.key);
                    server_message.client_id = client_message.client_id;
                    server_message.seq_number = client_message.seq_number;
                    send_packet(client_message.socket, packet.sock, packet.slen, (char *)&server_message, sizeof(server_message));
                    continue;
                }
                else if(client_message.seq_number == last){
                    send_packet(client_message.socket, packet.sock, packet.slen, (char *)&server_message, sizeof(server_message));
                    continue;
                }
                else{
                    continue;
                }
            }

            else if(strcmp(client_message.request, "put") == 0){
                if(client_message.seq_number > last){
                    put(client_message.key, client_message.value);
                    server_message.client_id = client_message.client_id;
                    server_message.seq_number = client_message.seq_number;
                    send_packet(client_message.socket, packet.sock, packet.slen, (char *)&server_message, sizeof(server_message));
                    continue;
                }
                else if(client_message.seq_number == last){
                    send_packet(client_message.socket, packet.sock, packet.slen, (char *)&server_message, sizeof(server_message));
                    continue;
                }
                else{
                    continue;
                }
            }
            
            else if(strcmp(client_message.request, "idle") == 0){
                if(client_message.seq_number > last){
                    server_message.client_id = client_message.client_id;
                    server_message.seq_number = client_message.seq_number;
                    send_packet(client_message.socket, packet.sock, packet.slen, (char *)&server_message, sizeof(server_message));
                    idle(client_message.time);
                    continue;
                }
                else if(client_message.seq_number == last){
                    send_packet(client_message.socket, packet.sock, packet.slen, (char *)&server_message, sizeof(server_message));
                    continue;
                }
                else{
                    continue;
                }
            }

            else{
                perror("This is not a valid request");
            }
        }

    }
    return 0;
}