#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "socksnug.h"
#include "socksnug_thread.h"
#include "socksnug_net.h"

pthread_t* launch_connection_thread(int s) {
  pthread_t* t   = NULL;
  int        ret = 0;

  t = calloc(1, sizeof(pthread_t));
  SN_EXIT_IF_TRUE(t == NULL, "launch_connection_thread: erreur d'allocation mémoire\n");

  ret = pthread_create(t, NULL, accept_connections, (void *)s);
  SN_EXIT_IF_TRUE(ret != 0, "Erreur lors de la création d'un thread de lancement !\n");

  return t;
}

pthread_t* launch_reading_thread() {
  pthread_t* t   = NULL;
  int        ret = 0;

  t = calloc(1, sizeof(pthread_t));
  SN_EXIT_IF_TRUE(t == NULL, "launch_connection_thread: erreur d'allocation mémoire\n");

  ret = pthread_create(t, NULL, read_all_sockets, NULL);
  SN_EXIT_IF_TRUE(ret != 0, "Erreur lors de la création d'un thread de lancement !\n");

  return t;
}

int are_there_enough_bytes_s(sn_socksclient* client, int needed_bytes) {
  int available_bytes = client->s_n - client->s_i;
  int available_space = client->s_buf_size - client->s_n;

  //printf("av bytes: %d, av space: %d needed bytes: %d\n", available_bytes, available_space, needed_bytes);

  if ( needed_bytes > available_bytes ) {
    int remaining_bytes = needed_bytes - available_bytes;

    if ( needed_bytes > client->s_buf_size ) {
      pthread_mutex_lock(&client->s_mutex);
      char* newbuf = realloc(client->s_buffer, (needed_bytes - available_space) + client->s_buf_size);
      if ( newbuf == NULL ) {
	fprintf(stderr, "Erreur d'allocation mémoire !\n");
	return 0;
      }
      client->s_buffer = newbuf;
      client->s_buf_size += (needed_bytes - available_space);
      pthread_mutex_unlock(&client->s_mutex);
    }
    else if ( remaining_bytes > available_space ) {
      /* Not enough space in the buffer */
      pthread_mutex_lock(&client->s_mutex);
      memcpy(client->s_buffer, &(client->s_buffer[client->s_i]), available_bytes);
      client->s_n = available_bytes;
      client->s_i = 0;
      pthread_mutex_unlock(&client->s_mutex);
    }

    return 0;
  }

  return available_bytes;
}

int are_there_enough_bytes_rs(sn_socksclient* client, int needed_bytes) {
  int available_bytes = client->rs_n - client->rs_i;
  int available_space = client->rs_buf_size - client->rs_n;

  //printf("av bytes: %d, av space: %d needed bytes: %d\n", available_bytes, available_space, needed_bytes);

  if ( needed_bytes > available_bytes ) {
    int remaining_bytes = needed_bytes - available_bytes;

    if ( needed_bytes > client->rs_buf_size ) {
      pthread_mutex_lock(&client->rs_mutex);
      char* newbuf = realloc(client->rs_buffer, (needed_bytes - available_space) + client->rs_buf_size);
      if ( newbuf == NULL ) {
	fprintf(stderr, "Erreur d'allocation mémoire !\n");
	return 0;
      }
      client->rs_buffer = newbuf;
      client->rs_buf_size += (needed_bytes - available_space);
      pthread_mutex_unlock(&client->rs_mutex);
    }
    else if ( remaining_bytes > available_space ) {
      /* Not enough space in the buffer */
      pthread_mutex_lock(&client->rs_mutex);
      memcpy(client->rs_buffer, &(client->rs_buffer[client->rs_i]), available_bytes);
      client->rs_n = available_bytes;
      client->rs_i = 0;
      pthread_mutex_unlock(&client->rs_mutex);
    }

    return 0;
  }

  return available_bytes;
}

void* service_thread(void* arg) {
  int                i        = 0;
  int n                       = (int)arg;
  sn_socksclient*    client   = NULL;
  sn_connection_msg* conn_msg = NULL;

  i = n;

  while ( 1 ) {
    client = g_allclients->array[i].client;
    /* if ( i == 127 )
      printf("%d\n", i); */
    if ( client == NULL ) {      //printf("aaaaaaaaa %d\n", i);
      goto next_iter; }

    if ( client->state == SN_TCP_CLOSED ) {

      if ( client->rs != -1 )
	close(client->rs);
      sn_del_client(g_allclients, &g_allclients->array[i]);

      goto next_iter;
    }

    int ret = 0;
    sn_method_selection_msg auth_sel_msg = { .ver = 0x05, .nmeths = 0x00 };

    /* If there are bytes to read in the buffer */
    switch ( client->state ) {
    case SN_TCP_OPENED :
      conn_msg = (sn_connection_msg*)&(client->s_buffer[client->s_i]);

      /*    if ( i == 127 )
	    printf("bbbbbbbb %d\n", i);*/
      if ( are_there_enough_bytes_s(client, 2) &&
	   are_there_enough_bytes_s(client, SN_CONNECTION_MSG_SIZEOF(conn_msg)) ) {
	if ( conn_msg->ver != 0x05 ) {
	  fprintf(stderr, "Seule la version 5 du protocole est gérée !\n");
	  close(client->s);
	  sn_del_client(g_allclients, &g_allclients->array[i]);
	  break;
	}

	int j = 0;
	int prefered_auth = -1;
	for ( j = 0; j < conn_msg->nmeths; j++ ) {
	  printf("meth: %d\n", conn_msg->meths[j]);
	  if ( is_supported_auth(conn_msg->meths[j]) ) {
	    if ( is_prefered_auth(prefered_auth, conn_msg->meths[j]) ) {
	      prefered_auth = conn_msg->meths[j];
	    }
	  }
	}

	if ( prefered_auth == -1 ) {
	  fprintf(stderr, "Méthodes d'authentification proposées non gérées !\n");
	  close(client->s);
	  sn_del_client(g_allclients, &g_allclients->array[i]);
	  break;
	}

	client->auth_method = prefered_auth;
	client->s_i += SN_CONNECTION_MSG_SIZEOF(conn_msg);
	client->state = SN_SOCKS_UNAUTH;
      }
      break;

    case SN_SOCKS_UNAUTH :
      switch ( client->auth_method ) {
      case SN_METH_NOAUTH :

	ret = sn_write_all(client->s, (char*)&auth_sel_msg, 2);
	if ( ret == -1 ) {
	  fprintf(stderr, "service_thread: erreur de sn_write_all 1");
	  sn_close_socks(client);
	  break;
	}

	client->state = SN_SOCKS_AUTHENTICATED;
	break;

      default :
	fprintf(stderr, "Méthode d'authentification non gérée !\n");
	close(client->s);
	sn_del_client(g_allclients, &g_allclients->array[i]);
	break;
      }

      break;

    case SN_SOCKS_AUTHENTICATED :
      printf("sn_socks_authenticated\n");
      socklen_t          bind_addr_len = sizeof(struct sockaddr_in);
      sn_request_msg*    msg           = (sn_request_msg*)&(client->s_buffer[client->s_i]);
      struct sockaddr_in bind_addr_in;

      printf("sn_socks_authenticated\n");

      if ( are_there_enough_bytes_s(client, 6) &&
	       are_there_enough_bytes_s(client, sn_request_msg_sizeof(msg)) ) {
	if ( msg->ver != 0x05 ) {
	  fprintf(stderr, "Seule la version 5 du protocole est gérée !\n");
	  close(client->s);
	  sn_del_client(g_allclients, &g_allclients->array[i]);
	  break;
	}

	if ( msg->cmd != 0x01 ) {
	  fprintf(stderr, "Need a CONNECT request !\n");
	  close(client->s);
	  sn_del_client(g_allclients, &g_allclients->array[i]);
	  break;
	}

	if ( msg->rsv != 0x00 ) {
	  fprintf(stderr, "The reserved byte must be \\x00 \n");
	  close(client->s);
	  sn_del_client(g_allclients, &g_allclients->array[i]);
	  break;
	}

	if ( msg->socket.atyp == SN_ATYP_IPV6 ) {
	  fprintf(stderr, "IPv6 is not yet implemented !\n");
	  close(client->s);
	  sn_del_client(g_allclients, &g_allclients->array[i]);
	  break;
	}

	if ( msg->ver != 0x05 ) {
	  fprintf(stderr, "Seule la version 5 du protocole est gérée !\n");
	  close(client->s);
	  sn_del_client(g_allclients, &g_allclients->array[i]);
	  break;
	}
	printf("aaaaaa sn_socks_authenticated\n");

	int rs = sn_socks_connect(&msg->socket, &bind_addr_in,
						   &bind_addr_len);
	if ( rs == -1 ) {
	  char* buf = calloc(10, sizeof(char));
	  if ( buf == NULL ) {
	    fprintf(stderr, "erreur d'allocation mémoire\n");
	    break;
	  }

	  sn_reply_msg* reply_msg = (sn_reply_msg*) buf;

	  fprintf(stderr, "service_thread: erreur de sn_socks_connect");
	  client->s_i += sn_request_msg_sizeof(msg);

	  reply_msg->ver = 0x05;
	  reply_msg->cmd = 0x05;
	  reply_msg->rsv = 0x00;
	  reply_msg->socket.atyp = 0x01;

	  ret = sn_write_all(client->s, (char*)reply_msg, 10);
	  if ( ret == -1 ) {
	    fprintf(stderr, "service_thread: erreur de sn_write_all 2");
	    break;
	  }

	  close(client->s);
	  sn_del_client(g_allclients, &g_allclients->array[i]);
	  break;
	}

	client->rs = rs;

	char* buf = calloc(10, sizeof(char));
	if ( buf == NULL ) {
	  fprintf(stderr, "erreur d'allocation mémoire\n");
	  break;
	}

	sn_reply_msg* reply_msg = (sn_reply_msg*) buf;

	reply_msg->ver = 0x05;
	reply_msg->cmd = 0x00;
	reply_msg->rsv = 0x00;
	reply_msg->socket.atyp = 0x01;
	memcpy(&reply_msg->socket.address.ipv4_addr, &bind_addr_in.sin_addr, sizeof(struct in_addr));
	memcpy((char*)&reply_msg->socket.address.ipv4_addr + sizeof(struct in_addr),
	       &bind_addr_in.sin_port, sizeof(unsigned short));

	ret = sn_write_all(client->s, (char*)reply_msg, 10);
	if ( ret == -1 ) {
	  fprintf(stderr, "service_thread: erreur de sn_write_all 3");
	  sn_close_socks(client);
	  break;
	}

	client->s_i += sn_request_msg_sizeof(msg);

	free(buf);
	client->state = SN_SOCKS_RELAY;
      }

      break;

    case SN_SOCKS_RELAY:
      //printf("sn_socks_relay %d %d\n", client->s, client->rs);
      while(0);

      int av_bytes_s = are_there_enough_bytes_s(client, 1);
      if ( av_bytes_s != 0 && client->rs != -1 ) {
	ret = sn_write_all(client->rs, &client->s_buffer[client->s_i], av_bytes_s);
	if ( ret == -1 ) {
	  fprintf(stderr, "service_thread: erreur de sn_write_all 4");
	  sn_close_socks(client);
	  break;
	}

	client->s_i += ret;
      }

      int av_bytes_rs = are_there_enough_bytes_rs(client, 1);
      if ( av_bytes_rs != 0 && client->s != -1 ) {
	ret = sn_write_all(client->s, &client->rs_buffer[client->rs_i], av_bytes_rs);
	if ( ret == -1 ) {
	  fprintf(stderr, "service_thread: erreur de sn_write_all 5");
	  sn_close_socks(client);

	  break;
	}

	client->rs_i += ret;
      }

      if ( client->s == -1 ) {
	close(client->rs);
	sn_del_client(g_allclients, &g_allclients->array[i]);
      } else if ( client->rs == -1 ) {
	close(client->s);
	sn_del_client(g_allclients, &g_allclients->array[i]);
      }


      break;

    default :
      printf("cas non encore géré!\n");
      break;
    }

  next_iter:
    i += g_params->nthreads;
    if ( i >= g_allclients->array_size )
      i = n;

    usleep(1000);
  }

  return NULL;
}

pthread_t* launch_services_thread() {
  pthread_t* t   = NULL;
  int        ret = 0;

  int i = 0;

  for ( i = 0; i < g_params->nthreads; i++ ) {
    t = calloc(1, sizeof(pthread_t));
    if ( t == NULL ) {
      fprintf(stderr, "Erreur d'allocation mémoire\n");
      return NULL;
    }

    ret = pthread_create(t, NULL, service_thread, (void *)i);
    if ( ret != 0 ) {
      fprintf(stderr, "Erreur lors de la création d'un thread de lancement !\n");
      return NULL;
    }
  }

  return t;
}
