#ifndef SOCKSNUG_THREAD_H
#define SOCKSNUG_THREAD_H

#include <pthread.h>

extern pthread_mutex_t mutex_allclients;

typedef struct _sn_services_thread_params {
  int             n;
  sn_all_clients* g_allclients;
  sn_params*      g_params;
} sn_services_thread_params;

pthread_t* launch_services_thread(sn_all_clients* g_allclients, sn_params* g_params);

typedef struct _sn_connect_thread_params {
  int             s;
  sn_all_clients* g_allclients;
  sn_params*      g_params;
} sn_connect_thread_params;

pthread_t* launch_clients_connection_thread(int s, sn_all_clients* g_allclients, sn_params* g_params);

typedef struct _sn_read_thread_params {
  sn_all_clients* g_allclients;
  sn_params*      g_params;
} sn_read_thread_params;

pthread_t* launch_reading_thread(sn_all_clients* g_allclients, sn_params* g_params);

#endif /* SOCKSNUG_THREAD_H */
