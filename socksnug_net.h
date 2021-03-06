#ifndef SOCKSNUG_NET_H
#define SOCKSNUG_NET_H

int listen_on_port(int port);
void* accept_connections(void* arg);
char* sockaddr_in_str(struct sockaddr_in* s_addrin);
void* read_all_sockets(void* args);
int sn_socks_connect(sn_socket* socket, struct sockaddr_in* saddr_in, socklen_t* saddr_len);
int sn_write_all(int socket, char* buffer, int n);
int sn_close_socks(sn_socksclient* client);

#endif /* SOCKSNUG_NET_H */
