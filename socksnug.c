#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include "socksnug.h"
#include "socksnug_util.h"
#include "socksnug_net.h"
#include "socksnug_thread.h"

int main(int argc, char* argv[]) {
  pthread_t* connection_thread = NULL;
  pthread_t* reading_thread    = NULL;

  /* Parsing on console parameters
   */
  g_params = parse_parameters(argc, argv);
  SN_EXIT_IF_TRUE(g_params == NULL, "main: erreur en sortie de parse_parameters");
  print_parameters(g_params);

  /* Listen on the default port or the port given in parameters
   */
  int s = listen_on_port(g_params->listening_socks_port);
  printf("socket: %d\n", s);

  /* Create the global "sn_all_clients" structure
   */
  g_allclients = sn_new_all_clients_array();
  SN_EXIT_IF_TRUE(g_allclients == NULL, "main: erreur en sortie de sn_new_all_parameters");

  /* Launch all threads :
   * connection thread : accept connections, create a new client, and add this client
   *                     to the global client structure
   * reading thread    : read from the sockets of all clients and fill the buffers
   * services thread   : state machine dynamic
   */
  connection_thread = launch_connection_thread(s);
  reading_thread = launch_reading_thread();
  launch_services_thread();

  pthread_join(*connection_thread, NULL);

  free(g_params);
  return 0;
}
