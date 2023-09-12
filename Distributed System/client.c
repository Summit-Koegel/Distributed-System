#include "client.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

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

// initializes the RPC connection to the server
struct rpc_connection RPC_init(int src_port, int dst_port, char dst_addr[]){
    struct rpc_connection rpc;
    struct sockaddr_storage addr;
    socklen_t addrlen;
    populate_sockaddr(AF_INET, dst_port, dst_addr, &addr, & addrlen);
    rpc.dst_addr = *((struct sockaddr *)(&addr));
    rpc.dst_len = addrlen;
    // try to establish connection
    rpc.recv_socket = init_socket(src_port);
    rpc.seq_number = 0;
    rpc.client_id = rand();
    return rpc;
}


// Sleeps the server thread for a few seconds
void RPC_idle(struct rpc_connection *rpc, int time){
    struct packet_info packet;
    struct message idle_msg;
    struct message receive_msg;
    idle_msg.request = "idle";
    idle_msg.time = time;
    idle_msg.client_id = rpc->client_id;
    idle_msg.seq_number = rpc->seq_number;
    idle_msg.socket = rpc->recv_socket;

    // retry up to 5 times
    for (int i = 0; i < RETRY_COUNT; i++){
        send_packet(rpc->recv_socket,rpc->dst_addr,rpc->dst_len,(char *)&idle_msg, sizeof(idle_msg));
        packet = receive_packet_timeout(rpc->recv_socket, 1);
        if (packet.recv_len != 0){
            receive_msg = *(struct message*)&packet.buf;
            // check if the right message
            if(receive_msg.seq_number != rpc->seq_number || receive_msg.client_id != rpc->client_id)
                continue;
            else if (receive_msg.ack == 1){
                rpc->seq_number += 1;
                break;
            }
        }
    }

    if(packet.recv_len == 0){
        perror("Socket Timeout!!");
    }

}

// gets the value of a key on the server store
int RPC_get(struct rpc_connection *rpc, int key){
    struct packet_info packet;
    struct message server_message;
    struct message get_msg;
    get_msg.request = "get";
    get_msg.key = key;
    get_msg.client_id = rpc->client_id;
    get_msg.seq_number = rpc->seq_number;
    get_msg.socket = rpc->recv_socket;

    // retry up to 5 times
    for (int i = 0; i < RETRY_COUNT; i++){
        send_packet(rpc->recv_socket,rpc->dst_addr,rpc->dst_len,(char *)&get_msg, sizeof(get_msg));
        packet = receive_packet_timeout(rpc->recv_socket, 3);
        if (packet.recv_len != 0){
            server_message = *(struct message*)&packet.buf;
            // check if the right message
            if(server_message.seq_number != rpc->seq_number || server_message.client_id != rpc->client_id)
                continue;
            else if (server_message.ack == 1){
                sleep(1);
                i = 0;
                continue;
            }
            else{
                rpc->seq_number += 1;
                break;
            }
        }
    }

    if(packet.recv_len == 0){
        perror("Socket Timeout!!");
    }

    return server_message.value;
}

// sets the value of a key on the server store
int RPC_put(struct rpc_connection *rpc, int key, int value){
    struct packet_info packet;
    struct message put_msg;
    struct message server_message;
    put_msg.request = "put";
    put_msg.key = key;
    put_msg.value = value;
    put_msg.client_id = rpc->client_id;
    put_msg.seq_number = rpc->seq_number;
    put_msg.socket = rpc->recv_socket;


    // retry up to 5 times
    for (int i = 0; i < RETRY_COUNT; i++){
        send_packet(rpc->recv_socket,rpc->dst_addr,rpc->dst_len,(char *)&put_msg, sizeof(put_msg));
        packet = receive_packet_timeout(rpc->recv_socket, 3);
        if (packet.recv_len != 0){
            server_message = *(struct message*)&packet.buf;
            // check if the right message
            if(server_message.seq_number != rpc->seq_number || server_message.client_id != rpc->client_id)
                continue;
            else if (server_message.ack == 1){
                sleep(1);
                i = 0;
                continue;
            }
            else{
                rpc->seq_number += 1;
                break;
            }
        }
    }

    if(packet.recv_len == 0){
        perror("Socket Timeout!!");
    }

    return 1;
}

// closes the RPC connection to the server
void RPC_close(struct rpc_connection *rpc){
    close_socket(rpc->recv_socket);
}