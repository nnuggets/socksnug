#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "socksnug.h"
#include "socksnug_net.h"
#include "socksnug_util.h"
#include "socksnug_thread.h"

/* Global variables :
 *
 * g_allclients : all clients of the socks proxy
 * g_params     : command line arguments
 */

/* Get the port number of a sn_socket structure
 */
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

/* Get the size of a sn_socket structure
 */
unsigned int sn_socket_sizeof(const sn_socket* sn_sock) {
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

unsigned int sn_request_msg_sizeof(sn_request_msg* msg) {
  SN_ASSERT(msg != NULL);

  return (sizeof(uint8_t)*3 + sn_socket_sizeof(&msg->socket));
}

/* Function to parse command line arguments
 */
sn_params* parse_parameters(int argc, char* argv[]) {
  sn_params* params = calloc(1, sizeof(sn_params));
  int        i      = 0;
  char*      value  = NULL;

  SN_ASSERT( argc > 0 );
  SN_ASSERT( params != NULL );

  params->listening_socks_port = DEFAULT_LISTENING_SOCKS_PORT;
  params->nthreads = DEFAULT_NUMBER_OF_THREADS;

  if ( argc == 1 )
    return params;

  for ( i = 1; i < argc; i++ ) {

    if ( strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--listening-socks-port") == 0 ) {
      if ( i + 1 >= argc ) {
	fprintf(stderr, "Missing port number argument !\n");
	exit(EXIT_FAILURE);
      }

      value = argv[++i];
      if ( is_unsigned_int(value) ) {
	params->listening_socks_port = atoi(value);
      }
      else {
 	fprintf(stderr, "Invalid port number !\n");
	exit(EXIT_FAILURE);
      }
    }

    else if ( strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--threads") == 0 ) {
      if ( i + 1 >= argc ) {
	fprintf(stderr, "Missing threads number argument !\n");
	exit(EXIT_FAILURE);
      }

      value = argv[++i];
      if ( is_unsigned_int(value) ) {
	params->nthreads = atoi(value);
      }
      else {
	fprintf(stderr, "Invalid thread number !\n");
	exit(EXIT_FAILURE);
      }
    }

  }

  return params;
}

/* Print command line arguments
 */
void print_parameters(sn_params *params) {
  printf("\n------------------\n");
  printf("Listening socks port : %d\n", params->listening_socks_port);
  printf("Number of threads : %d\n", params->nthreads);
  printf("Verbose : %d\n", params->verbose);
  printf("------------------\n\n");
}


/* Add a slot number in the linked list of free slots of a sn_all_clients
 * variable
 */
int sn_add_freeslot(sn_all_clients* sac, int slot) {
  int          ret = 0;
  sn_int_list* sil = NULL;

  SN_ASSERT(sac == NULL);

  sil = calloc(1, sizeof(sn_int_list));
  SN_RETURN_IF_TRUE(sil == NULL, "sn_add_freeslot: erreur d'allocation mémoire",
		    -1);
  sil->n = slot;

  ret = pthread_mutex_lock(&sac->freelist_mutex);
  SN_EXIT_IF_TRUE(ret != 0, "sn_add_freeslot: erreur de pthread_mutex_lock");

  sil->next = sac->freelist;
  sac->freelist = sil;

  ret = pthread_mutex_unlock(&sac->freelist_mutex);
  SN_EXIT_IF_TRUE(ret != 0, "sn_add_freeslot: erreur de pthread_mutex_unlock");

  return 0;
}

/* Get and remove a slot number from the linked list of free slots of a sn_all_clients
 * variable
 */
int sn_get_freeslot(sn_all_clients* sac) {
  int ret       = 0;
  int ret_value = -1;

  SN_ASSERT(sac == NULL);

  ret = pthread_mutex_lock(&sac->freelist_mutex);
  SN_EXIT_IF_TRUE(ret != 0, "sn_get_freeslot: erreur de pthread_mutex_lock");

  if ( sac->freelist == NULL ) {
    ret_value = -1;
  }
  else {
    ret_value = sac->freelist->n;
    sac->freelist = sac->freelist->next;
  }

  ret = pthread_mutex_unlock(&sac->freelist_mutex);
  SN_EXIT_IF_TRUE(ret != 0, "sn_get_freeslot: erreur de pthread_mutex_unlock");

  return ret_value;
}

/* Initialize a all_clients array objet
 */
sn_all_clients* sn_new_all_clients_array() {
  int             i     = 0;
  int             ret   = 0;
  sn_all_clients* sac   = NULL;
  pthread_mutex_t tmp   = PTHREAD_MUTEX_INITIALIZER;

  sac = calloc(1, sizeof(sn_all_clients));
  SN_RETURN_IF_TRUE(sac == NULL, "sn_new_all_clients_sac: erreur d'allocation mémoire",
		    NULL);

  sac->array = calloc(SN_INITSIZE_ALLCLIENTS, sizeof(sn_all_clients_item));
  SN_RETURN_IF_TRUE(sac->array == NULL, "sn_new_all_clients_sac: erreur d'allocation mémoire",
		    NULL);

  sac->array_size = SN_INITSIZE_ALLCLIENTS;
  memcpy(&sac->freelist_mutex, &tmp, sizeof(pthread_mutex_t));

  for ( i = 0; i < SN_INITSIZE_ALLCLIENTS; i++) {
    ret = sn_add_freeslot(sac, i);
    SN_RETURN_IF_TRUE(ret == -1,
		    "sn_new_all_clients_array: erreur d'ajout d'un slot libre", NULL);

    memcpy(&sac->array[i].mutex, &tmp, sizeof(pthread_mutex_t));
  }

  return sac;
}


/* Create a new client
 */
sn_socksclient* sn_new_socksclient() {
  sn_socksclient* sn_client  = NULL;

  sn_client = calloc(1, sizeof(sn_socksclient));

  sn_client->s_buffer = calloc(SN_BUFFER_SIZE, sizeof(char));
  SN_RETURN_IF_TRUE(sn_client->s_buffer == NULL,
		    "sn_new_socksclient: erreur d'allocation mémoire", NULL);

  sn_client->rs_buffer = calloc(SN_BUFFER_SIZE, sizeof(char));
  SN_RETURN_IF_TRUE(sn_client->rs_buffer == NULL,
		    "sn_new_socksclient: erreur d'allocation mémoire", NULL);

  sn_client->s_buf_size = SN_BUFFER_SIZE;
  sn_client->rs_buf_size = SN_BUFFER_SIZE;

  pthread_mutex_t tmp = PTHREAD_MUTEX_INITIALIZER;
  memcpy(&sn_client->s_mutex, &tmp, sizeof(pthread_mutex_t));
  memcpy(&sn_client->rs_mutex, &tmp, sizeof(pthread_mutex_t));

  return sn_client;
}

/* Add a client to a sn_all_clients object
 */
int sn_add_client(sn_all_clients* sac, sn_socksclient* client) {
  int slot = 0;

  SN_ASSERT(sac != NULL);

  slot = sn_get_freeslot(sac);
  if ( slot == -1)
    return -1;

  sac->array[slot].client = client;
  sac->nclients++;

  return slot;
}


/* Delete a client from a sn_all_clients object
 */
int sn_del_client(sn_all_clients* sac, sn_all_clients_item* item) {
  int slot = -1;
  int ret  = 0;

  SN_ASSERT(sac != NULL);
  SN_ASSERT(item != NULL);

  ret = pthread_mutex_lock(&item->mutex);
  SN_EXIT_IF_TRUE(ret != 0, "sn_del_client: erreur de pthread_mutex_lock");

  SN_RETURN_IF_TRUE(item->client == NULL, "", 0);

  free(item->client->s_buffer);
  free(item->client->rs_buffer);
  free(item->client);

  item->client = NULL;

  ret = pthread_mutex_unlock(&item->mutex);
  SN_EXIT_IF_TRUE(ret != 0, "sn_del_client: erreur de pthread_mutex_unlock");

  slot = sn_add_freeslot(sac, item - sac->array);
  if ( slot == -1)
    return -1;

  return slot;
}

/* Converts a client object to a string
 */
char* sn_socksclient_str(sn_socksclient* client) {
  char* str       = NULL;
  char* s_addrin  = NULL;
  char* rs_addrin = NULL;
  int   size      = 1024;

  if ( client == NULL )
    return NULL;

  str = calloc(1, size * sizeof(char));
  SN_RETURN_IF_TRUE(str == NULL, "sn_sockslicent_str: erreur d'allocation mémoire", NULL);

  s_addrin = sockaddr_in_str(&client->s_addrin);
  rs_addrin = sockaddr_in_str(&client->rs_addrin);

  snprintf(str, size, "{ s: %d, rs: %d, s_addrin: %s, rs_addrin: %s, state: %d, s_buffer: %s, rs_buffer: %s, s_buf_size: %d, rs_buf_size: %d, s_i: %d, rs_i: %d, auth_method: %d }",
	  client->s, client->rs, s_addrin, rs_addrin, client->state, "", "", client->s_buf_size,
	  client->rs_buf_size, client->s_i, client->rs_i, client->auth_method);

  free(s_addrin);
  free(rs_addrin);

  return str;
}

int is_supported_auth(int auth_meth) {
  return auth_meth == 0;
}

int is_prefered_auth(int auth_meth, int auth_meth_chal) {
  return auth_meth_chal == 0;
}
