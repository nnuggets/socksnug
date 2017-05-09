#ifndef SOCKSNUG_H
#define SOCKSNUG_H

#include <stdint.h>
#include <netinet/in.h>

#include "socksnug_util.h"

/* SOCKS5 Connection Message
 * ver    : version of the protocol (0x05)
 * nmeths : number of bytes in 'meths' field
 * meth   : array of bytes. each byte represent an authentication method
 */
#define SN_METH_NOAUTH   0x00
#define SN_METH_GSSAPI   0x01
#define SN_METH_USERPASS 0x02
#define SN_METH_NONE     0xFF
#define SN_IS_METH_IANA_ASSIGNED(meth) ( ((meth) >= 0x03) && ((meth) <= 0x7F) )
#define SN_IS_METH_PRIVATE(meth) ( ((meth) >= 0x80) && ((meth) <= 0xFE) )

typedef struct SN_PACKED _sn_connection_msg {
  uint8_t ver;
  uint8_t nmeths;
  uint8_t meths[];
} sn_connection_msg;

typedef sn_connection_msg* psn_connection_msg;
typedef sn_connection_msg  sn_method_selection_msg;

#define SN_CONNECTION_MSG_SIZEOF(conn_msg) ( sizeof(sn_connection_msg) + \
					     (conn_msg)->nmeths )


/* A SOCKS socket is a couple of address (IP or Domain Name) + port
 */
#define SN_ATYP_IPV4       0x01
#define SN_ATYP_DOMAINNAME 0x03
#define SN_ATYP_IPV6       0x06

typedef struct SN_PACKED _sn_socket {
  uint8_t atyp;
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

int sn_socket_sizeof(const sn_socket* sn_sock);
in_port_t sn_socket_getport(sn_socket* sn_sock);

#define SN_SOCKET_SIZEOF(sock) sn_socket_sizeof(sock)
#define SN_SOCKET_GETADDRESSTYPE(sock) ( (sock)->atyp )
#define SN_SOCKET_GETIPV4(sock) ( &(sock)->address.ipv4_addr )
#define SN_SOCKET_GETIPV6(sock) ( &(sock)->address.ipv6_addr )
#define SN_SOCKET_GETDOMAINNAME(sock) ( &(sock)->address.domain_name )

#define SN_SOCKET_GETPORT(sock) sn_socket_getport(sock)

typedef struct SN_PACKED _sn_request_msg {
  uint8_t ver;
  uint8_t cmd;
  uint8_t rsv;
  sn_socket socket;
} sn_request_msg;

typedef sn_request_msg sn_reply_msg;

typedef struct SN_PACKED _sn_params {
  int listening_socks_port; // -p port
} sn_params;

#define DEFAULT_LISTENING_SOCKS_PORT 1080


#endif /* SOCKSNUG_H */
