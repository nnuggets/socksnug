#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "socksnug.h"
#include "socksnug_util.h"

//int is_unsigned_int(char *s);

sn_params* parse_parameters(int argc, char* argv[]) {
  sn_params* params = malloc(sizeof(sn_params));
  int        i      = 0;
  char*      value  = NULL;

  SN_ASSERT( argc > 0 );
  SN_ASSERT( params != NULL );

  params->listening_socks_port = DEFAULT_LISTENING_SOCKS_PORT;

  if ( argc == 1 )
    return params;

  for (i = 1; i < argc; i++ ) {
    if ( strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--listening-socks-port") == 0 ) {
      value = argv[++i];
      if ( is_unsigned_int(value) ) {
	params->listening_socks_port = atoi(value);
      } else {
	fprintf(stderr, "Invalid port number !\n");
	exit(EXIT_FAILURE);
      }
    }
  }

  return params;
}

int main(int argc, char* argv[]) {
  sn_params *params = NULL;

  params = parse_parameters(argc, argv);
  SN_ASSERT( params != NULL );

  printf("%d\n", params->listening_socks_port);

  free(params);
  return 0;
}
