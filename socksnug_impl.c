#include "socksnug.h"

in_port_t sn_socket_getport(sn_socket* sn_sock) {
  SN_ASSERT(sn_sock != NULL);

  int offset = sizeof(uint8_t);

  switch ( sn_sock->atyp ) {
  case SN_ATYP_IPV6 :
    offset += sizeof(struct in6_addr);
    break;
  case SN_ATYP_IPV4 :
    offset += sizeof(struct in_addr);
    break;
  case SN_ATYP_DOMAINNAME :
    offset += sizeof(uint8_t) + sn_sock->address.domain_name.strlen;
    break;
  }

  return *(in_port_t *)((uint8_t*)sn_sock + offset);
}

int sn_socket_sizeof(const sn_socket* sn_sock) {
  SN_ASSERT(sn_sock != NULL);

  int size = 0;

  switch ( sn_sock->atyp ) {
  case SN_ATYP_IPV6 :
    size = sizeof(uint8_t) + sizeof(struct in6_addr) + sizeof(in_port_t);
    break;
  case SN_ATYP_IPV4 :
    size = sizeof(uint8_t) + sizeof(struct in_addr) + sizeof(in_port_t);
    break;
  case SN_ATYP_DOMAINNAME :
    size = sizeof(uint8_t) + sizeof(uint8_t) + sn_sock->address.domain_name.strlen + \
      sizeof(in_port_t);
    break;
  }

  return size;
}
