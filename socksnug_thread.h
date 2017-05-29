#ifndef SOCKSNUG_THREAD_H
#define SOCKSNUG_THREAD_H

#include <pthread.h>

extern pthread_mutex_t mutex_allclients;

pthread_t* launch_services_thread();

pthread_t* launch_connection_thread(int s);

pthread_t* launch_reading_thread();

int associate_a_service_thread(int s);

#endif /* SOCKSNUG_THREAD_H */
