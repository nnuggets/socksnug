#include <stdio.h>
#include <stdlib.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "socksnug.h"
#include "socksnug_test.h"

sn_socket* sn_sock1 = (sn_socket*) "\x01\x7F\x00\x00\x01\x00\x50";
sn_socket* sn_sock2 = (sn_socket*) "\x03\x0awww.ugn.fr\x00\x51";
sn_socket* sn_sock3 = (sn_socket*) "\x06\xfe\x80\x0a\x00\x27\xff\xfe\x6b\x8a\x40\x00\x00\x00\x00\x00\x00\x00\x52";

sn_unit_test unit_tests_sn_socket[] = {
  { .unit_test_name = "test de sn_socket_sizeof", .unit_test_func = test_sn_socket_sizeof },
  { NULL, NULL }
};

sn_tests_suite all_tests_suites[] = {
  { "Suite de tests de sn_socket", init_suite_sn_socket, clean_suite_sn_socket,
    unit_tests_sn_socket },
  { NULL, NULL, NULL, NULL }
};

int init_suite_sn_socket(void) {
  return 0;
}

void test_sn_socket_sizeof(void) {
  CU_ASSERT( SN_SOCKET_SIZEOF(sn_sock1) == 7 );
  CU_ASSERT( SN_SOCKET_SIZEOF(sn_sock2) == 14 );
  CU_ASSERT( SN_SOCKET_SIZEOF(sn_sock3) == 19 );

  CU_ASSERT( SN_SOCKET_GETPORT(sn_sock1) == 0x5000 )
  CU_ASSERT( SN_SOCKET_GETPORT(sn_sock2) == 0x5100 )
  CU_ASSERT( SN_SOCKET_GETPORT(sn_sock3) == 0x5200 )
}

int clean_suite_sn_socket(void)
{
    return 0;
}

int main(int argc, char* argv[]) {
  sn_tests_suite *tmp_tests_suite = NULL;
  sn_unit_test   *tmp_unit_test   = NULL;
  CU_pSuite       pSuite          = NULL;

  /* Initialize the CUnit test registry */
  if ( CU_initialize_registry() != CUE_SUCCESS )
    return CU_get_error();

  SN_ASSERT( sizeof(all_tests_suites) >= sizeof(sn_test_suite) );

  tmp_tests_suite = all_tests_suites;
  while ( tmp_tests_suite->suite_name != NULL && tmp_tests_suite->suite_tests != NULL ) {
    /* add a suite to the registry */
    pSuite = CU_add_suite(tmp_tests_suite->suite_name, tmp_tests_suite->suite_init_func,
			  tmp_tests_suite->suite_cleanup_func);
    if ( pSuite == NULL ) {
      CU_cleanup_registry();
      return CU_get_error();
    }

    /* add the tests to the suite */
    tmp_unit_test = tmp_tests_suite->suite_tests;
    while ( tmp_unit_test->unit_test_func != NULL ) {
      if ( CU_add_test(pSuite, tmp_unit_test->unit_test_name, tmp_unit_test->unit_test_func) \
	   == NULL ) {
	CU_cleanup_registry();
	return CU_get_error();
      }
      tmp_unit_test += sizeof(sn_unit_test);
    }

    tmp_tests_suite += sizeof(sn_tests_suite);
  }

  /* Run all tests using the CUnit Basic interface */
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();

  return CU_get_error();
}
