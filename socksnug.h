#ifndef SOCKSNUG_H
#define SOCKSNUG_H

#include <stdint.h>
#include <netinet/in.h>

#include "socksnug_util.h"

#define SN_METH_NOAUTH   0x00
#define SN_METH_GSSAPI   0x01
#define SN_METH_USERPASS 0x02
#define SN_METH_NONE     0xFF
#define SN_IS_METH_IANA_ASSIGNED(meth) ( ((meth) >= 0x03) && ((meth) <= 0x7F) )
#define SN_IS_METH_PRIVATE(meth) ( ((meth) >= 0x80) && ((meth) <= 0xFE) )

/* SOCKS5 Connection Message
 * ver    : version of the protocol (0x05)
 * nmeths : number of bytes in 'meths' field
 * meth   : array of bytes. each byte represent an authentication method
 */
typedef struct SN_PACKED _sn_connection_msg {
  uint8_t ver;
  uint8_t nmeths;
  uint8_t meths[];
} sn_connection_msg;

typedef sn_connection_msg* psn_connection_msg;
typedef sn_connection_msg  sn_method_selection_msg;

int is_supported_auth(int auth_meth);
int is_prefered_auth(int auth_meth, int auth_meth_chal);

#define SN_CONNECTION_MSG_SIZEOF(conn_msg) ( sizeof(sn_connection_msg) + \
					     (conn_msg)->nmeths )


#define SN_ATYP_IPV4       0x01
#define SN_ATYP_DOMAINNAME 0x03
#define SN_ATYP_IPV6       0x06

/* A SOCKS socket is a couple form by an address (IP or Domain Name) and a port
 */
typedef struct SN_PACKED _sn_socket {
  uint8_t           atyp;
  union {
    struct {
      uint8_t       strlen;
      char          str[];
    } domain_name;
    struct in_addr  ipv4_addr;
    struct in6_addr ipv6_addr;
  } address;
  //in_port_t         port[];
} sn_socket;

unsigned int sn_socket_sizeof(const sn_socket* sn_sock);
in_port_t sn_socket_getport(sn_socket* sn_sock);

#define SN_SOCKET_SIZEOF(sock) sn_socket_sizeof(sock)
#define SN_SOCKET_GETADDRESSTYPE(sock) ( (sock)->atyp )
#define SN_SOCKET_GETIPV4(sock) ( &(sock)->address.ipv4_addr )
#define SN_SOCKET_GETIPV6(sock) ( &(sock)->address.ipv6_addr )
#define SN_SOCKET_GETDOMAINNAME(sock) ( &(sock)->address.domain_name )

#define SN_SOCKET_GETPORT(sock) sn_socket_getport(sock)

/* SOCKS5 Request Message
 * ver    : version of the protocol (0x05)
 * cmd    : Command
 * rsv    : Reserved bytes
 * socket : address + port
 */
typedef struct SN_PACKED _sn_request_msg {
  uint8_t   ver;
  uint8_t   cmd;
  uint8_t   rsv;
  sn_socket socket;
} sn_request_msg;

typedef sn_request_msg sn_reply_msg;

#define SN_SOCKS_CMD_CONNECT       0x01
#define SN_SOCKS_CMD_BIND          0x02
#define SN_SOCKS_CMD_UDP_ASSOCIATE 0x03

unsigned int sn_request_msg_sizeof(sn_request_msg* msg);

#define SN_REQUEST_MSG_SIZEOF(msg) sn_request_msg_sizeof(msg)

/* Command line parameters
 */
typedef struct SN_PACKED _sn_params {
  int listening_socks_port; // -p port - Port which listen on socks clients
  int listening_proxy_port; // -P port - Port which listen on connetions made by
                            // distant proxies
  int nthreads;             // -t T
  int verbose;              // -v
} sn_params;

#define DEFAULT_LISTENING_SOCKS_PORT 1080
#define DEFAULT_LISTENING_PROXY_PORT 0
#define DEFAULT_NUMBER_OF_THREADS 10

sn_params* parse_parameters(int argc, char* argv[]);
void print_parameters(sn_params *params);


/* States of the socks protocol
 */
enum sn_socksclient_state {
  SN_TCP_OPENED = 0,
  SN_TCP_CLOSED,
  SN_SOCKS_UNAUTH,
  SN_SOCKS_AUTHENTICATED,
  SN_SOCKS_RELAY,
};

#define SN_BUFFER_SIZE 8192

typedef struct sn_socksclient {
  int    s;                               // socket resulted from the accept syscall
  int    rs;                              // socket to the remote computer

  struct sockaddr_in          s_addrin;   // remote address and port of the client
  struct sockaddr_in          rs_addrin;  // remote address and port of the remote computer

  enum   sn_socksclient_state state;      // state of the client in socks protocol

  char  *s_buffer;                        // buffer for read/write in s socket
  char  *rs_buffer;                       // buffer for read/write in rs socket

  unsigned int    s_buf_size;             // size of s_buffer
  unsigned int    rs_buf_size;            // size of rs_buffer

  unsigned int    s_i;                    // read cursor in s_buffer
  unsigned int    rs_i;                   // read cursor in rs_buffer

  unsigned int    s_n;                    // number of bytes in the s buffer
  unsigned int    rs_n;                   // number of bytes in the rs buffer

  pthread_mutex_t s_mutex;
  pthread_mutex_t rs_mutex;

  int   auth_method;                      // authentication method
} sn_socksclient;

/* Default size of the the global allclients variable
 */
#define SN_INITSIZE_ALLCLIENTS 128

/* Integer linked list
 */
typedef struct _sn_int_list {
  int n;
  struct _sn_int_list* next;
} sn_int_list;

typedef struct _sn_all_clients_item {
  pthread_mutex_t mutex;
  sn_socksclient* client;
} sn_all_clients_item;

/* Structure to store all clients and theirs states
 */
typedef struct _sn_all_clients {
  unsigned int         nclients;               // number of clients
  unsigned int         array_size;             // size (capacity) of the array
  sn_all_clients_item* array;
  sn_int_list*         freelist;               // linked list of free slot in the array
  pthread_mutex_t      freelist_mutex;
} sn_all_clients;

sn_all_clients* sn_new_all_clients_array();
sn_socksclient* sn_new_socksclient();

void sn_init_allclients_array();
int sn_add_client(sn_all_clients* sal, sn_socksclient* client);
int sn_del_client(sn_all_clients* sal, sn_all_clients_item* item);




/* States of the inter socks server protocol
 */
enum sn_socksserver_state {
  SN_SOCKSSERVER_TCP_OPENED = 0,
  SN_SOCKSSERVER_TCP_CLOSED,
  SN_SOCKSSERVER_UNAUTH,
  SN_SOCKSSERVER_AUTHENTICATED,
  SN_SOCKSSERVER_INSERVICE,
};

typedef struct sn_socksserver {
  int    s;                               // socket resulted from the accept syscall
  struct sockaddr_in          s_addrin;   // remote address and port of the proxy server

  enum   sn_socksserver_state state;      // state of the client in socks protocol

  char  *s_buffer;                        // buffer for read/write in s socket
  unsigned int    s_buf_size;             // size of s_buffer
  unsigned int    s_i;                    // read cursor in s_buffer
  unsigned int    s_n;                    // number of bytes in the s buffer
  pthread_mutex_t s_mutex;

  int   auth_method;                      // authentication method
} sn_socksserver;

typedef struct _sn_all_proxies_item {
  pthread_mutex_t mutex;
  sn_socksserver* client;
} sn_all_proxies_item;

/* Structure to store all proxies and theirs states
 */
typedef struct _sn_all_proxies {
  unsigned int         nproxies;               // number of clients
  unsigned int         array_size;             // size (capacity) of the array
  sn_all_proxies_item* array;
  sn_int_list*         freelist;               // linked list of free slot in the array
  pthread_mutex_t      freelist_mutex;
} sn_all_proxies;

sn_all_clients* sn_new_all_proxies_array();
sn_socksclient* sn_new_socksproxy();

void sn_init_allproxies_array();
int sn_add_proxy(sn_all_proxies* sal, sn_socksproxy* client);
int sn_del_proxy(sn_all_proxies* sal, sn_all_proxies_item* item);

char* sn_socksclient_str(sn_socksclient* client);
char* sn_socksproxy_str(sn_socksproxy* client);

#endif /* SOCKSNUG_H */
