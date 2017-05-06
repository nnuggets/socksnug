#ifndef SOCKSNUG_TEST_H
#define SOCKSNUG_TEST_H

#include <CUnit/CUnit.h>

typedef struct _sn_unit_test {
  char       *unit_test_name;
  CU_TestFunc unit_test_func;
} sn_unit_test;

typedef struct _sn_tests_suite {
  char             *suite_name;
  CU_InitializeFunc suite_init_func;
  CU_CleanupFunc    suite_cleanup_func;
  sn_unit_test     *suite_tests;
} sn_tests_suite;

int init_suite_sn_socket(void);
int clean_suite_sn_socket(void);
void test_sn_socket_sizeof(void);

#endif /* SOCKSNUG_TEST_H */
