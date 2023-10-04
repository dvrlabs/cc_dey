/*
* Copyright (c) 2017 Digi International Inc.
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
* REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
* AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
* INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
* LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
* OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
* PERFORMANCE OF THIS SOFTWARE.
*
* Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
* =======================================================================
*/

#include "test_helper_functions.h"

TEST_GROUP(test_ccapi_logging)
{
    void setup()
    {
        /* We need to start at least one instance of the connector so lock object is created */
        ccapi_start_t start = {0};
        ccapi_start_error_t error;

        Mock_create_all();

        logging_lock_users = 0;
        logging_lock = NULL;

        th_fill_start_structure_with_good_parameters(&start);
        error = ccapi_start(&start);
        CHECK(error == CCAPI_START_ERROR_NONE);
    }

    void teardown()
    {
        ccapi_stop_error_t stop_error;

        Mock_connector_initiate_action_expectAndReturn(ccapi_data_single_instance->connector_handle, connector_initiate_terminate, NULL, connector_success);

        stop_error = ccapi_stop(CCAPI_STOP_IMMEDIATELY);
        CHECK(stop_error == CCAPI_STOP_ERROR_NONE);
        CHECK(ccapi_data_single_instance == NULL);

        CHECK_EQUAL(0, logging_lock_users);
        CHECK(logging_lock == NULL);

        Mock_destroy_all();
    }
};

#if (defined CCIMP_DEBUG_ENABLED)

TEST(test_ccapi_logging, testCcapiNoArguments)
{
	debug_t debug;

    debug = debug_all;

	Mock_ccimp_hal_logging_vprintf_expect(debug, "layer2_args0");

    ccapi_logging_line("layer2_args0");
}

TEST(test_ccapi_logging, testCcapiOneArgument)
{
	debug_t debug;

    debug = debug_all;

	Mock_ccimp_hal_logging_vprintf_expect(debug, "layer2_args1 33");

    ccapi_logging_line("layer2_args1 %d", 33);
}

TEST(test_ccapi_logging, testCcapiTwoArguments)
{
	debug_t debug;

    debug = debug_all;

	Mock_ccimp_hal_logging_vprintf_expect(debug, "layer2_args2 34 0.25");

    ccapi_logging_line("layer2_args2 %d %.2f", 34, 0.25);
}

static void * thread_logging(void * argument)
{
    #define BUF_SIZE 256*4
    int buffer[BUF_SIZE];
    int i;

    for (i=0; i < BUF_SIZE; i++)
        buffer[i]= i;
    
    ccapi_logging_print_buffer((char*)argument, buffer, BUF_SIZE);

    return NULL;
}

pthread_t create_thread_logging(void * argument)
{
    pthread_t pthread;
    int ccode = pthread_create(&pthread, NULL, thread_logging, argument);

    if (ccode != 0)
    {
        printf("create_thread_logging() error %d\n", ccode);
    }

    return pthread;
}


TEST(test_ccapi_logging, testCcapiLoggingSyncr)
{
    #define NUM_THREADS 10
    pthread_t thread_h[NUM_THREADS];
    char* thread_str[NUM_THREADS];
    char thread_str_pattern[] = "thread %d:";
    int i;

    for (i=0; i < NUM_THREADS; i++)
    {
        thread_str[i]= (char*)malloc(sizeof(thread_str_pattern));
        sprintf(thread_str[i], thread_str_pattern, i);

        thread_h[i] = create_thread_logging(thread_str[i]);
    }

    for (i=0; i < NUM_THREADS; i++)
    {
        pthread_join(thread_h[i], NULL);
    }
}

#define CALL_DEBUG_VPRINTF(type, format) \
    do \
    { \
        va_list args; \
 \
        va_start(args, (format)); \
        connector_debug_vprintf((type), (format), args); \
        va_end(args); \
    } \
    while (0)

static void connector_debug_line(char const * const format, ...)
{
    CALL_DEBUG_VPRINTF(debug_all, format);
}

TEST(test_ccapi_logging, testCcfsmNoArguments)
{
	debug_t debug;

    debug = debug_all;

	Mock_ccimp_hal_logging_vprintf_expect(debug, "layer1_args0");

    connector_debug_line("layer1_args0");
}

TEST(test_ccapi_logging, testCcfsmOneArgument)
{
	debug_t debug;

    debug = debug_all;

	Mock_ccimp_hal_logging_vprintf_expect(debug, "layer1_args1 33");

    connector_debug_line("layer1_args1 %d", 33);
}

TEST(test_ccapi_logging, testCcfsmTwoArguments)
{
	debug_t debug;

    debug = debug_all;

	Mock_ccimp_hal_logging_vprintf_expect(debug, "layer1_args2 34 0.25");

    connector_debug_line("layer1_args2 %d %.2f", 34, 0.25);
}

#else
/* CCAPI_LOGGING is not defined. Check that nothing happens */
TEST(test_ccapi_logging, testDebugDefineDisabled)
{
    ccapi_start_error_t error;
    ccapi_start_t start = {0};
    th_fill_start_structure_with_good_parameters(&start);

    /* Manually enable the mock to check that nothing is called */
    mock("ccimp_hal_logging_vprintf").setData("behavior", CCIMP_LOGGING_PRINTF_DOESNT_EXPECT_A_CALL);

    error = ccapi_start(&start);
    CHECK(error == CCAPI_START_ERROR_NONE);
}
#endif
