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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include "ccapi/ccapi.h"
#include "ccapi/ccapi_datapoints.h"
#include "device_request.h"

/* Hack */
//#include "source/ccapi_definitions.h"
#include "connector_api.h"

typedef enum {
    CCAPI_DP_ARG_DATA_INT32,
    CCAPI_DP_ARG_DATA_INT64,
    CCAPI_DP_ARG_DATA_FLOAT,
    CCAPI_DP_ARG_DATA_DOUBLE,
    CCAPI_DP_ARG_DATA_STRING,
    CCAPI_DP_ARG_DATA_JSON,
    CCAPI_DP_ARG_DATA_GEOJSON,
    CCAPI_DP_ARG_TS_EPOCH,
    CCAPI_DP_ARG_TS_EPOCH_MS,
    CCAPI_DP_ARG_TS_ISO8601,
    CCAPI_DP_ARG_LOCATION,
    CCAPI_DP_ARG_QUALITY,
    CCAPI_DP_ARG_INVALID
} ccapi_dp_argument_t;

typedef struct ccapi_dp_data_stream {
    connector_data_stream_t * ccfsm_data_stream;
    struct {
        ccapi_dp_argument_t * list;
        unsigned int count;
    } arguments;
    struct ccapi_dp_data_stream * next;
} ccapi_dp_data_stream_t;

typedef struct ccapi_dp_collection {
    ccapi_dp_data_stream_t * ccapi_data_stream_list;
    uint32_t dp_count;
    void * lock;
} ccapi_dp_collection_t;

#define APP_DEBUG printf
/************************************************************/
/******** FUNCTIONS TO PRINT THE DATAPOINT STRUCTURE ********/
/************************************************************/

char* getTimestamp(void)
{
//     time_t ltime;
//     struct tm result;
//     static char stime[32];
// 
//     ltime = time(NULL);
//     localtime_r(&ltime, &result);
// //     asctime_r(&result, stime);
// 
// 
// //     time_t ltime; /* calendar time */
// //     ltime=time(NULL); /* get current cal time */
// 
// //     "2015-03-30T16:31:53Z"
// 
//     sprintf(stime, "%d-%d-%dT%d:%d:%dZ\n", result.tm_year+1900, result.tm_mon+1, result.tm_mday, result.tm_hour, result.tm_min, result.tm_sec);
// 
    time_t rawtime;
    struct tm * timeinfo;
    static char buffer [80];

    rawtime = time(NULL);
    timeinfo = localtime(&rawtime);


    strftime (buffer,80,"%FT%TZ.",timeinfo);

    return buffer;

//     printf("%s\n",buffer);

//     return stime;

}


/* Function to print by console the DataStream/DataPoint structure */
// void app_show_datastreams(connector_data_stream_t * ccfsm_data_stream)
void app_show_datastreams(/*ccapi_dp_collection_t * p*/ ccapi_dp_collection_handle_t datapoin_collection)
{

    ccapi_dp_collection_t * p = (ccapi_dp_collection_t *)datapoin_collection;

    /* Get pointer to the first DataStream */
    ccapi_dp_data_stream_t * data_stream_list = p->ccapi_data_stream_list;
    //uint32_t numberOfStreams = p->dp_count;
    unsigned int stream_index = 0;


    /* Detect how many DataStreams we have in this structure */
    unsigned int numberOfStreams = 1;
    connector_data_stream_t * pointer_to_stream = data_stream_list->ccfsm_data_stream;
    while ( pointer_to_stream->next != NULL )
    {
        numberOfStreams++;
        pointer_to_stream = pointer_to_stream->next;
    }


    /* For each DataStream we show the info */
    for (stream_index = 0; stream_index < numberOfStreams; stream_index++)
    {
        /* Get each DataStream */
        connector_data_stream_t * stream = data_stream_list->ccfsm_data_stream;



        APP_DEBUG("[Datastream] ID: '%s' -- UNITS: '%s'\n", stream->stream_id, stream->unit);

        if (stream->point != NULL)
        {
            /* Detect how many DataPoints we have in this DataStream */
            unsigned int numberOfPoints = 1;
            connector_data_point_t * pointer_to_point = stream->point;
            while ( pointer_to_point->next != NULL )
            {
                numberOfPoints++;
                pointer_to_point = pointer_to_point->next;
            }


            size_t point_index;


            /* Get first DataPoint from the list */
            connector_data_point_t * point = stream->point;

            /* For each DataPoint we show the info */
            for (point_index = 0; point_index < numberOfPoints; point_index++)
            {

                /* Print the DataPoint info */
                switch ( stream->type )
                {
                    case connector_data_point_type_integer:
                        APP_DEBUG("\n[DataPoint] Location: '%.1f,%.1f,%.1f' -- Quality: '%d' -- Description: '%s' -- Data: '%d'\n",
                            point->location.value.native.latitude,
                            point->location.value.native.longitude,
                            point->location.value.native.elevation,
                            (int)point->quality.value,
                            point->description,
                            point->data.element.native.int_value);
                        break;

                    case connector_data_point_type_float:
                        #if (defined CONNECTOR_SUPPORTS_FLOATING_POINT)
                            APP_DEBUG("\n[DataPoint] Location: '%.1f,%.1f,%.1f' -- Quality: '%d' -- Description: '%s' -- Data: '%f'\n",
                                point->location.value.native.latitude,
                                point->location.value.native.longitude,
                                point->location.value.native.elevation,
                                (int)point->quality.value,
                                point->description,
                                point->data.element.native.float_value);
                        #else
                            APP_DEBUG("\n[DataPoint] Location: '%s,%s,%s' -- Quality: '%d' -- Description: '%s' -- Data: 's'\n",
                                point->location.value.text.latitude,
                                point->location.value.text.longitude,
                                point->location.value.text.elevation,
                                (int)point->quality.value,
                                point->description,
                                point->data.element.text);
                        #endif
                        break;
                    case connector_data_point_type_double:
                        #if (defined CONNECTOR_SUPPORTS_FLOATING_POINT)
                            APP_DEBUG("\n[DataPoint] Location: '%.1f,%.1f,%.1f' -- Quality: '%d' -- Description: '%s' -- Data: '%f'\n",
                                point->location.value.native.latitude,
                                point->location.value.native.longitude,
                                point->location.value.native.elevation,
                                (int)point->quality.value,
                                point->description,
                                point->data.element.native.double_value);
                        #else
                            APP_DEBUG("\n[DataPoint] Location: '%s,%s,%s' -- Quality: '%d' -- Description: '%s' -- Data: '%s' \n",
                                point->location.value.text.latitude,
                                point->location.value.text.longitude,
                                point->location.value.text.elevation,
                                (int)point->quality.value,
                                point->description,
                                point->data.element.text);
                        #endif
                        break;
                    case connector_data_point_type_long:
                        #if (defined CONNECTOR_SUPPORTS_64_BIT_INTEGERS)
                            APP_DEBUG("\n[DataPoint] Location: '%.1f,%.1f,%.1f' -- Quality: '%d' -- Description: '%s' -- Data: '%" PRId64 "' \n",
                                point->location.value.native.latitude,
                                point->location.value.native.longitude,
                                point->location.value.native.elevation,
                                (int)point->quality.value,
                                point->description,
                                point->data.element.native.long_value);
                        #else
                            APP_DEBUG("\n[DataPoint] Location: '%s,%s,%s' -- Quality: '%d' -- Description: '%s' -- Data: '%s' \n",
                                point->location.value.text.latitude,
                                point->location.value.text.longitude,
                                point->location.value.text.elevation,
                                (int)point->quality.value,
                                point->description,
                                point->data.element.text);
                        #endif
                        break;
                    case connector_data_point_type_string:
                    case connector_data_point_type_json:
                    case connector_data_point_type_geojson:
                        APP_DEBUG("\n[DataPoint] Location: '%.1f,%.1f,%.1f' -- Quality: '%d' -- Description: '%s' -- Data: '%s' \n",
                            point->location.value.native.latitude,
                            point->location.value.native.longitude,
                            point->location.value.native.elevation,
                            (int)point->quality.value,
                            point->description,
                            point->data.element.native.string_value);
                        break;
                    default:
                        APP_DEBUG("\n[DataPoint] Location: '%s,%s,%s' -- Quality: '%d' -- Description: '%s' -- Data: 'XX'\n",
                            point->location.value.text.latitude,
                            point->location.value.text.longitude,
                            point->location.value.text.elevation,
                            (int)point->quality.value,
                            point->description);
                        break;

                }



                /* Go to the next point */
                if ( point->next != NULL )
                {
                    point = point->next;
                }



            }
        }


        /* Go to the next stream */
        if ( data_stream_list->next != NULL )
        {
            data_stream_list = data_stream_list->next;
        }


    }

}



/*****************************************************/
/**************** TEST_CASES FUNCTIONS ***************/
/*****************************************************/
void * app_start_test_case_datapoints_loop(void * args){
    printf("\napp_start_test_case_datapoints_loop: start function....\n");

    /* Parse the passed arguments structure */
    test_thread_arguments_t * test_arguments = (test_thread_arguments_t *) args;

    printf("%s\n",test_arguments->valueType);

    ccapi_dp_collection_handle_t datapoin_collection;
    ccapi_dp_error_t error;


    /* Generate DataPoint collection */
    error = ccapi_dp_create_collection(&datapoin_collection);
    if (datapoin_collection == NULL)
    {
        printf("ccapi_dp_create_collection() failed with error %d\n", error);
    }


    /* Create a string null terminated to process the payload */
    char * config_elements = malloc( sizeof(char) * 100 );
    sprintf(config_elements, "%s loc qual", test_arguments->valueType);


    error = ccapi_dp_add_data_stream_to_collection_extra(datapoin_collection, test_arguments->streamIdentifier, config_elements, "Counts", NULL);

    if (datapoin_collection == NULL)
    {
        printf("ccapi_dp_create_stream() failed with error %d\n", error);
    }

    /* Initialize vars */
    uint32_t int32_value = 99;
    uint64_t int64_value = 55555;
    float float_value = 25.5 ;
    double double_value = 5000;
    int quality = 0;


    for(unsigned int i=1;i<=test_arguments->numberOfLoops;i++)
    {

        unsigned int j;
        ccapi_location_t location;
        /* Generate all DataPoints */
        for(j = 0; j < test_arguments->numberPointsPerStream; j++)
        {

//             get_gps_location(&location.latitude, &location.longitude);
            location.latitude = 42;
            location.longitude = 2;
            location.elevation = 90;

            /* Obtain a new Quality */
            quality++;

            /* Send each kind of Datapoint */
            if ( strcmp(test_arguments->valueType, "int32")==0 )
            {
                int32_value++;
                error = ccapi_dp_add(datapoin_collection, test_arguments->streamIdentifier, int32_value, &location, quality);
            }
            else if ( strcmp(test_arguments->valueType, "int64")==0 )
            {
                int64_value++;
                error = ccapi_dp_add(datapoin_collection, test_arguments->streamIdentifier, int64_value, &location, quality);
            }
            else if ( strcmp(test_arguments->valueType, "float")==0 )
            {
                float_value++;
                error = ccapi_dp_add(datapoin_collection, test_arguments->streamIdentifier, float_value, &location, quality, getTimestamp());
            }
            else if ( strcmp(test_arguments->valueType, "double")==0 )
            {
                double_value++;
                error = ccapi_dp_add(datapoin_collection, test_arguments->streamIdentifier, double_value, &location, quality);
            }
            else if ( strcmp(test_arguments->valueType, "string")==0 )
            {
                #define TEST_STRING_PATTERNS 8

                static int index_string = 0;
                static char * string_value[TEST_STRING_PATTERNS] = {
                        "long string 300 chars...............................................................................1...................................................................................................2...................................................................................................3...................................................................................................",
                        "Hello World!",
                        "c,o,m,m,a",
                        /* Removed due to it is not posible verify it in test harness, move to other test case */
                        /* "line\nfeed",*/
                        "t\ta\tb",
                        "\"quote\"",
                        "s p a c e",
                        "long string 574 chars..............................................................................1..............................................................................2..............................................................................3..............................................................................4..............................................................................5..............................................................................6..............................................................................7",
                        "long string 1611 chars..............................................................................1..............................................................................2..............................................................................3..............................................................................4..............................................................................5..............................................................................6..............................................................................7..............................................................................8..............................................................................9..............................................................................10..............................................................................11..............................................................................12..............................................................................13..............................................................................14..............................................................................15..............................................................................16..............................................................................17..............................................................................18..............................................................................19.............................................................................."};

                error = ccapi_dp_add(datapoin_collection, test_arguments->streamIdentifier, string_value[index_string % TEST_STRING_PATTERNS], &location, quality);
                index_string++;
            }
            else if ( strcmp(test_arguments->valueType, "json")==0 )
            {
                #define TEST_JSON_PATTERNS 5

                static int index_json = 0;
                static char * json_value[TEST_JSON_PATTERNS] = {
                    "{\"address\":{\"streetAddress\": \"21 2nd Street\",\"city\":\"New York\"},\"phoneNumber\":[{\"type\":\"home\",\"number\":\"212 555-1234\"}]}",
                    "{\"array\":[1,2,3],\"boolean\":true,\"null\":null,\"number\":123,\"object\":{\"a\":\"b\",\"c\":\"d\",\"e\":\"f\"},\"string\":\"Hello World\"}",
                    "{\"index\":\"test\"}",
                    "{ long string 574 chars..............................................................................1..............................................................................2..............................................................................3..............................................................................4..............................................................................5..............................................................................6..............................................................................7}",
                    "long string 1611 chars..............................................................................1..............................................................................2..............................................................................3..............................................................................4..............................................................................5..............................................................................6..............................................................................7..............................................................................8..............................................................................9..............................................................................10..............................................................................11..............................................................................12..............................................................................13..............................................................................14..............................................................................15..............................................................................16..............................................................................17..............................................................................18..............................................................................19.............................................................................."};


                error = ccapi_dp_add(datapoin_collection, test_arguments->streamIdentifier, json_value[index_json % TEST_JSON_PATTERNS], &location, quality);
                index_json++;

            }
            else if ( strcmp(test_arguments->valueType, "geojson")==0 )
            {
                #define TEST_GEOJSON_PATTERNS 6

                static int index_geojson = 0;
                static char * geojson_value[TEST_GEOJSON_PATTERNS] = {
                        "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\",\"coordinates\":[102.0, 0.6]},\"properties\": {\"prop0\":\"value0\"}},{\"type\":\"Feature\",\"geometry\": {\"type\": \"LineString\",\"coordinates\": [[102.0, 0.0], [103.0, 1.0], [104.0, 0.0], [105.0, 1.0]]},\"properties\": {\"prop1\": 0.0,\"prop0\": \"value0\"}},{\"type\": \"Feature\",\"geometry\": {\"type\": \"Polygon\",\"coordinates\": [[[100.0, 0.0], [101.0, 0.0], [101.0, 1.0], [100.0, 1.0],[100.0, 0.0]]]},\"properties\": {\"prop1\": {\"this\": \"that\"},\"prop0\": \"value0\"}}]}",
                        "{\"coordinates\": [102.0, 0.6]}",
                        "{ \"type\": \"Point\", \"coordinates\": [30, 10]}",
                        "{\"type\": \"MultiPolygon\",\"coordinates\": [[ [[40, 40], [20, 45], [45, 30], [40, 40]]],[[[20, 35], [10, 30], [10, 10], [30, 5], [45, 20], [20, 35]],[[30, 20], [20, 15], [20, 25], [30, 20]]]]}",
                        "{ long string 574 chars..............................................................................1..............................................................................2..............................................................................3..............................................................................4..............................................................................5..............................................................................6..............................................................................7 }",
                        "long string 1611 chars..............................................................................1..............................................................................2..............................................................................3..............................................................................4..............................................................................5..............................................................................6..............................................................................7..............................................................................8..............................................................................9..............................................................................10..............................................................................11..............................................................................12..............................................................................13..............................................................................14..............................................................................15..............................................................................16..............................................................................17..............................................................................18..............................................................................19.............................................................................."};


                error = ccapi_dp_add(datapoin_collection, test_arguments->streamIdentifier, geojson_value[index_geojson % TEST_GEOJSON_PATTERNS], &location, quality);
                index_geojson++;

            }


            /* Verify if we have errors */
            if (error != CCAPI_DP_ERROR_NONE)
            {
                printf("ccapi_dp_add failed with error: %d\n", error);
            }

        }

        /* First we should print the DataPoint collection */
        app_show_datastreams(datapoin_collection);



        /* Send the DataPoint collection */
        error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, datapoin_collection);
        if (error != CCAPI_DP_ERROR_NONE)
        {
            printf("ccapi_dp_send_collection failed over TCP: %d\n", error);
        }
        {
            printf("ccapi_dp_send_collection Success!!!\n");
        }


        APP_DEBUG("END LOOP ----------------------------------------\n\n");

        sleep(5);
    }


    /* Destroy Datapoint collection */
    ccapi_dp_destroy_collection(datapoin_collection);


    goto done;

done:
    printf("Finished the Thread for app_start_test_case_datapoints_loop()\n");
    return NULL;

}
