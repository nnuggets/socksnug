#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define _GNU_SOURCE
#define __USE_GNU

#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>

#include "socksnug.h"
#include "socksnug_util.h"
#include "socksnug_net.h"

/* Create a socket that listen on a given port
 */
int listen_on_port(int port) {
  int s   = 0;
  int ret = 0;
  struct sockaddr_in addrin = { 0 };

  s = socket(AF_INET, SOCK_STREAM, 0);
  SN_EXIT_IF_TRUE(s < 0, "The socket function failed");

  bzero((char*) &addrin, sizeof(addrin));
  addrin.sin_family = AF_INET;
  addrin.sin_port = htons(port);
  addrin.sin_addr.s_addr = INADDR_ANY;

  ret = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
  if ( ret < 0 )
    fprintf(stderr, "setsockopt(SO_REUSEADDR) failed");

  ret = bind(s, (struct sockaddr*)&addrin, sizeof(addrin));
  SN_EXIT_IF_TRUE(ret != 0, "The bind fucntion failed");

  ret = listen(s, 100);
  SN_EXIT_IF_TRUE( ret != 0, "The listen function failed");

  return s;
}

/* Read the content of all sockets into their buffers
 */
void* read_all_sockets(void* args) {
  int             i       = 0;
  int             ret     = 0;
  int             max     = -1;
  fd_set          readfs;
  struct timeval  tv      = { 0 };
  sn_socksclient* client  = NULL;

  SN_ASSERT (g_allclients != NULL);

  while ( 1 ) {
    /* reset the set of sockets to read
     */
    FD_ZERO(&readfs);
    /* reset the timeout
     */
    max = 0;
    tv.tv_sec = 0;
    tv.tv_usec = 1000 * 10; // 10 milliseconds

    /* List of sockets to listen
     */
    for ( i = 0; i < g_allclients->array_size; i++ ) {
      ret = pthread_mutex_trylock(&g_allclients->array[i].mutex);
      if ( ret !=  0 )
	continue; // The mutex is already acquired

      client = g_allclients->array[i].client;
      if ( client == NULL || client->state == SN_TCP_CLOSED ) {
	pthread_mutex_unlock(&g_allclients->array[i].mutex);
	continue;
      }

      /* Listen on s and rs
       */
      FD_SET(client->s, &readfs);
      if ( client->rs != -1 )
	FD_SET(client->rs, &readfs);

      /* Max descriptor
       */
      if ( client->s > max )
	max = client->s;

      if ( client->rs > max )
	max = client->rs;

      pthread_mutex_unlock(&g_allclients->array[i].mutex);
    }

    /* Listen to sockets */
    if ( (ret = select(max + 1, &readfs, NULL, NULL, &tv)) < 0 ) {
      perror("select");
      exit(errno);
    }

    if ( ret <= 0 )
      continue;

    /* Sockets in read state*/
    for ( i = 0; i < g_allclients->array_size; i++ ) {
      ret = pthread_mutex_trylock(&g_allclients->array[i].mutex);
      if ( ret != 0 )
	continue; // The mutex is already acquired

      client = g_allclients->array[i].client;

      if ( client == NULL ||
	   client->state == SN_TCP_CLOSED ) {
	ret = pthread_mutex_unlock(&g_allclients->array[i].mutex);
	SN_EXIT_IF_TRUE(ret != 0, "reading_thread: erreur de pthread_mutex_unlock");
	continue;
      }

      if ( FD_ISSET(client->s, &readfs) ) {
	int n = 0;

	/* Test if the socket is closed */
	ioctl(client->s, FIONREAD, &n);
	if ( n == 0 ) {
	  client->state = SN_TCP_CLOSED;
	  ret = pthread_mutex_unlock(&g_allclients->array[i].mutex);
	  SN_EXIT_IF_TRUE(ret != 0, "reading_thread: erreur de pthread_mutex_unlock");
	  continue;
	}

	if ( client->s_n < client->s_buf_size ) {
	  n = read(client->s,
		   &client->s_buffer[client->s_n],
		   client->s_buf_size - client->s_n);

	  if ( n == -1 ) {
	    fprintf(stderr, "Erreur dans la fonction read\n");
	    ret = pthread_mutex_unlock(&g_allclients->array[i].mutex);
	    SN_EXIT_IF_TRUE(ret != 0, "reading_thread: erreur de pthread_mutex_unlock");
	    continue;
	  }

	  printf("read : %d\n", n);
	  client->s_n += n;
	}
      }


      if ( client->rs != -1 ) {
	if ( FD_ISSET(client->rs, &readfs) ) {
	  int n = 0;

	  /* Test if the socket is closed */
	  ioctl(client->rs, FIONREAD, &n);
	  if ( n == 0 ) {
	    close(client->s);
	    close(client->rs);
	    client->rs = -1;
	    ret = pthread_mutex_unlock(&g_allclients->array[i].mutex);
	    SN_EXIT_IF_TRUE(ret != 0, "reading_thread: erreur de pthread_mutex_unlock");
	    continue;
	  }

	  if ( client->rs_n < client->rs_buf_size ) {
	    n = read(client->rs, &client->rs_buffer[client->rs_n],
		     client->rs_buf_size - client->rs_n);

	    if ( n == -1 ) {
	      fprintf(stderr, "Erreur dans la fonction read\n");
	      ret = pthread_mutex_unlock(&g_allclients->array[i].mutex);
	      SN_EXIT_IF_TRUE(ret != 0, "reading_thread: erreur de pthread_mutex_unlock");
	      continue;
	    }

	    client->rs_n += n;
	  }
	}
      }

      ret = pthread_mutex_unlock(&g_allclients->array[i].mutex);
      SN_EXIT_IF_TRUE(ret != 0, "reading_thread: erreur de pthread_mutex_unlock");
    }
  }
}

/* Accept connections coming from clients
 * Create a new client object for each new connection
 * Add this new client object to the global all_clients object
 */
void* accept_connections(void* arg) {
  int                s          = (int) arg;
  int                client_s   = 0;
  unsigned int       addrin_len = sizeof(struct sockaddr_in);
  struct sockaddr_in addrin     = { 0 };
  sn_socksclient*    sn_client  = NULL;

  while( (client_s = accept4(s, (struct sockaddr*)&addrin, &addrin_len, SOCK_NONBLOCK)) ) {
    sn_client = sn_new_socksclient();
    if( sn_client == NULL ) {
      fprintf(stderr, "connection non suivie: erreur d'allocation mémoire !");
      continue;
    }

    sn_client->s = client_s;
    sn_client->rs = -1;
    memcpy(&sn_client->s_addrin, &addrin, addrin_len);
    sn_client->state = SN_TCP_OPENED;

    printf("%s\n", sn_socksclient_str(sn_client));

    int slot = sn_add_client(g_allclients, sn_client);
    if ( slot == -1 ) {
      fprintf(stderr, "accept_connections: client non ajouté. erreur de sn_add_client.");
      close(client_s);
      free(sn_client);
      continue;
    }

    printf("%p - %p - slot: %d\n", sn_client, g_allclients->array[slot].client, slot);
    printf("\n%s\n", sn_socksclient_str(g_allclients->array[slot].client));

    addrin_len = sizeof(struct sockaddr_in);
  }

  return NULL;
}

/* Create a string from a sockaddr_in structure
 */
char* sockaddr_in_str(struct sockaddr_in* s_addrin) {
  char* str      = NULL;
  char* sin_addr = NULL;
  int   size     = 1024;

  if ( s_addrin == NULL )
    return NULL;

  str = calloc(1, size * sizeof(char));
  SN_RETURN_IF_TRUE(str == NULL, "sockaddr_in_str: erreur d'allocation mémoire", NULL);

  sin_addr = calloc(1, INET_ADDRSTRLEN * sizeof(char));
  SN_RETURN_IF_TRUE(sin_addr == NULL, "sockaddr_in_str: erreur d'allocation mémoire", NULL);

  inet_ntop(AF_INET, &s_addrin->sin_addr, sin_addr, INET_ADDRSTRLEN);
  snprintf(str, size, "{ sin_family: %d , sin_port: %uh, sin_addr: %s }",
	   s_addrin->sin_family, ntohs(s_addrin->sin_port), sin_addr);
  free(sin_addr);

  return str;
}

int turn_in_blocking(int sock) {
  int arg = fcntl(sock, F_GETFL, NULL);
  arg &= ~O_NONBLOCK;
  fcntl(sock, F_SETFL, arg);
  return 0;
}

int turn_in_nonblocking(int sock) {
  int arg = fcntl(sock, F_GETFL, NULL);
  arg |= O_NONBLOCK;
  fcntl(sock, F_SETFL, arg);
  return 0;
}

int wait_data_to_read(int sock, struct timeval* tv) {
  int    ret;
  fd_set fd_set;

  FD_ZERO(&fd_set);
  FD_SET(sock, &fd_set);

  ret = select(sock + 1, NULL, &fd_set, NULL, tv);
  if(ret == 1) {
    socklen_t len = sizeof(ret);
    // Y a t il eu une erreur ?
    int err = getsockopt(sock, SOL_SOCKET, SO_ERROR, &ret, &len);
    if(err == -1 || ret != 0) {
      ret = -1;
      perror("Erreur connect_socket: ");
    }
    else {
      return 0;
    }
  }

  return -1;
}

int connect_socket(int sock, struct sockaddr_in* sinserv, struct timeval* tv) {
  int ret;

  turn_in_nonblocking(sock);
  connect(sock, (struct sockaddr*)sinserv, sizeof(*sinserv));
  ret = wait_data_to_read(sock, tv);
  turn_in_blocking(sock);
  return ret;
}

/* Connect to a distant socket
 * and return a socket
 */
int sn_socks_connect(sn_socket* sn_sock, struct sockaddr_in* saddr_in, socklen_t* saddr_len) {
  int                 ret          = 0;
  char*               domain       = NULL;
  char*               port         = NULL;
  int                 s            = -1;
  struct timeval      tv           = { 0 };
  struct addrinfo     criterias    = { 0 };
  struct addrinfo*    ip_addresses = NULL;
  struct sockaddr_in* sin_serv     = NULL;
  struct sockaddr_in* tmp          = NULL;

  SN_RETURN_IF_TRUE(sn_sock->atyp == SN_ATYP_IPV6,
		    "sn_socks_connect: IPv6 is not yet implemented !\n", -1);

  if ( sn_sock->atyp == SN_ATYP_DOMAINNAME ) {
    memset(&criterias, 0, sizeof(criterias));
    criterias.ai_family   = AF_INET;
    criterias.ai_socktype = SOCK_STREAM;
    criterias.ai_flags    = AI_NUMERICSERV;


    domain = calloc(sn_sock->address.domain_name.strlen + 1, sizeof(char));
    SN_RETURN_IF_TRUE(domain == NULL, "sn_socks_connect: erreur d'allocation mémoire\n",
		      -1);

    port = calloc(7, sizeof(char));
    SN_RETURN_IF_TRUE(port == NULL, "sn_socks_connect: erreur d'allocation mémoire\n",
		      -1);

    memcpy(domain, sn_sock->address.domain_name.str, sn_sock->address.domain_name.strlen);
    snprintf(port, 6, "%d", SN_SOCKET_GETPORT(sn_sock));

    ret = getaddrinfo(domain, port, &criterias, &ip_addresses);
    SN_RETURN_IF_TRUE(ret != 0,
		      "sn_socks_connect: Impossible de résoudre le nom de domaine\n", -1);

    struct addrinfo* ip_tmp = ip_addresses;
    SN_RETURN_IF_TRUE(ip_tmp == NULL,
		      "sn_socks_connect: Aucune IP résolue\n", -1);

    sin_serv = (struct sockaddr_in*)ip_tmp->ai_addr;

    free(domain);
    free(port);
  }
  else if ( sn_sock->atyp == SN_ATYP_IPV4 ) {
    printf("bbbbbbbb \n");
    sin_serv = calloc(1, sizeof(struct sockaddr_in));
    SN_RETURN_IF_TRUE(sin_serv == NULL, "sn_socks_connect: erreur d'allocation mémoire\n",
		      -1);
    bzero(sin_serv, sizeof(struct sockaddr_in));
    sin_serv->sin_family = AF_INET;
    sin_serv->sin_port = SN_SOCKET_GETPORT(sn_sock);
    memcpy(&sin_serv->sin_addr, &sn_sock->address.ipv4_addr, sizeof(struct in_addr));

    tmp = sin_serv;

    printf("cccc: %s\n", sockaddr_in_str(tmp));
  }
  else {
    SN_RETURN_IF_TRUE(1 == 1, "sn_socks_connect: erreur de type de socket", -1);
  }

  s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  SN_RETURN_IF_TRUE(s == -1, "sn_socks_connect: erreur de la fonction socket", -1);

  tv.tv_sec = 3;
  tv.tv_usec = 0;

  ret = connect_socket(s, sin_serv, &tv);
  SN_RETURN_IF_TRUE(ret < 0, "sn_socks_connect: erreur de la fonction connect_socket", -1);

  ret = getsockname(s, saddr_in, saddr_len);
  SN_RETURN_IF_TRUE(ret == -1, "sn_socks_connect: erreur de la fonction getsockname", -1);

  free(tmp);

  return s;
}
