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
  pthread_t*      connection_thread = NULL;
  pthread_t*      reading_thread    = NULL;
  sn_params*      g_params          = NULL;
  sn_all_clients* g_allclients      = NULL;
  sn_all_proxies* g_allproxies      = NULL;

  /* Parsing console parameters
   */
  g_params = parse_parameters(argc, argv);
  SN_EXIT_IF_TRUE(g_params == NULL, "main: erreur en sortie de parse_parameters");
  print_parameters(g_params);

  /* Listen on the socks server ports
   */
  int servers_listen_sock = -1;
  if ( g_params->listening_proxy_port != 0 ) {
    servers_listen_sock = listen_on_port(g_params->listening_proxy_port);

    /* Launch remote proxies management threads
     */
    g_allproxies = sn_new_all_proxies_array();
    launch_proxies_connection_thread(servers_listen_sock, g_allproxies, g_params);
  }

  /* Listen on the socks clients port
   */
  int clients_listen_sock = listen_on_port(g_params->listening_socks_port);

  /* Create the global "sn_all_clients" structure
   */
  g_allclients = sn_new_all_clients_array();
  SN_EXIT_IF_TRUE(g_allclients == NULL, "main: erreur en sortie de sn_new_all_parameters");

  /* Launch all clients management threads
   */

  /* connection thread : accept client connections, create a new socksclient 
   * per connection, and add this client to the global client structure
   */
  connection_thread = launch_clients_connection_thread(clients_listen_sock, g_allclients, g_params);

  /* reading thread    : read from the sockets of all clients and fill the buffers
   */
  reading_thread = launch_reading_thread(g_allclients, g_params);

  /* services thread   : state machine dynamic
   */
  launch_services_thread(g_allclients, g_params);

  pthread_join(*connection_thread, NULL);

  free(g_params);
  free(g_allclients);

  return 0;
}
