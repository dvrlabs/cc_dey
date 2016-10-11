/*
 * ccimp_datapoints.c
 *
 * Copyright (C) 2016 Digi International Inc., All Rights Reserved
 *
 * This software contains proprietary and confidential information of Digi.
 * International Inc. By accepting transfer of this copy, Recipient agrees
 * to retain this software in confidence, to prevent disclosure to others,
 * and to make no use of this software other than that for which it was
 * delivered. This is an unpublished copyrighted work of Digi International
 * Inc. Except as permitted by federal law, 17 USC 117, copying is strictly
 * prohibited.
 *
 * Restricted Rights Legend
 *
 * Use, duplication, or disclosure by the Government is subject to restrictions
 * set forth in sub-paragraph (c)(1)(ii) of The Rights in Technical Data and
 * Computer Software clause at DFARS 252.227-7031 or subparagraphs (c)(1) and
 * (2) of the Commercial Computer Software - Restricted Rights at 48 CFR
 * 52.227-19, as applicable.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 *
 * Description: Cloud Connector data points implementation.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/sysinfo.h>
#include "ccapi/ccapi.h"
#include "utils.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#define LOOP_MS						100

#define SYSTEM_MONITOR_TAG			"SYSMON:"

#define DATA_STREAM_MEMORY			"system_monitor/free_memory"
#define DATA_STREAM_CPU_LOAD		"system_monitor/cpu_load"
#define DATA_STREAM_CPU_TEMP		"system_monitor/cpu_temperature"

#define DATA_STREAM_MEMORY_UNITS	"kB"
#define DATA_STREAM_CPU_LOAD_UNITS	"%"
#define DATA_STREAM_CPU_TEMP_UNITS	"C"

#define FILE_CPU_LOAD				"/proc/stat"
#define FILE_CPU_TEMP				"/sys/class/thermal/thermal_zone0/temp"

/*------------------------------------------------------------------------------
                    F U N C T I O N  D E C L A R A T I O N S
------------------------------------------------------------------------------*/
static void *system_monitor_threaded();
static void system_monitor_loop(const cc_cfg_t * const cc_cfg);
static ccapi_dp_error_t init_system_monitor(const cc_cfg_t * const cc_cfg);
static void add_system_samples(unsigned long memory, double load, double temp, const cc_cfg_t * const cc_cfg);
static ccapi_timestamp_t* get_timestamp(void);
static unsigned long get_free_memory(void);
static double get_cpu_load(void);
static double get_cpu_temp(void);
static uint32_t calculate_number_samples(const cc_cfg_t * const cc_cfg);
static long read_file(const char *path, char **buffer, long file_size);

/*------------------------------------------------------------------------------
                                  M A C R O S
------------------------------------------------------------------------------*/
/**
 * log_sm_debug() - Log the given message as debug
 *
 * @format:		Debug message to log.
 * @args:		Additional arguments.
 */
#define log_sm_debug(format, args...)									\
    log_debug("%s " format, SYSTEM_MONITOR_TAG, ##args)

/**
 * log_sm_error() - Log the given message as error
 *
 * @format:		Error message to log.
 * @args:		Additional arguments.
 */
#define log_sm_error(format, args...)									\
    log_error("%s " format, SYSTEM_MONITOR_TAG, ##args)

/*------------------------------------------------------------------------------
                         G L O B A L  V A R I A B L E S
------------------------------------------------------------------------------*/
extern ccapi_bool_t stop;
static unsigned long long last_idle = 0;
static unsigned long long last_total = 0;
static pthread_t dp_thread;
static ccapi_dp_collection_handle_t dp_collection;

/*------------------------------------------------------------------------------
                     F U N C T I O N  D E F I N I T I O N S
------------------------------------------------------------------------------*/
/*
 * start_system_monitor() - Start the monitoring of system variables
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) where the
 * 			settings parsed from the configuration file are stored.
 *
 * The variables being monitored are: CPU temperature, CPU load, and free
 * memory.
 *
 * Return: Error code after starting the monitoring.
 */
int start_system_monitor(const cc_cfg_t * const cc_cfg)
{
	pthread_attr_t attr;
	int any_sys_mon_enabled = (cc_cfg->sys_mon_parameters & SYS_MON_MEMORY)
			| (cc_cfg->sys_mon_parameters & SYS_MON_LOAD)
			| (cc_cfg->sys_mon_parameters & SYS_MON_TEMP);
	int error;

	if (!(cc_cfg->services & SYS_MONITOR_SERVICE)
			|| !any_sys_mon_enabled || cc_cfg->sys_mon_sample_rate <= 0)
		return CCAPI_DP_ERROR_NONE;

	error = pthread_attr_init(&attr);
	if (error != 0) {
		/* On Linux this function always succeeds. */
		log_sm_error("pthread_attr_init() error %d\n", error);
	}
	error = pthread_create(&dp_thread, &attr, system_monitor_threaded, (void *) cc_cfg);
	if (error != 0) {
		log_sm_error("pthread_create() error %d\n", error);
		pthread_attr_destroy(&attr);
		return CCIMP_STATUS_ERROR;
	}
	pthread_attr_destroy(&attr);
	return CCAPI_DP_ERROR_NONE;
}

/*
 * stop_system_monitor() - Stop the monitoring of system variables
 */
void stop_system_monitor(void)
{
	if (!pthread_equal(dp_thread, 0))
		pthread_join(dp_thread, NULL);
	ccapi_dp_destroy_collection(dp_collection);
}

/*
 * system_monitor_threaded() - Execute the system monitoring in a new thread
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) where the
 * 			settings parsed from the configuration file are stored.
 */
static void *system_monitor_threaded(void *cc_cfg)
{
	ccapi_dp_error_t dp_error = init_system_monitor(cc_cfg);
	if (dp_error != CCAPI_DP_ERROR_NONE)
		/* The data point collection could not be created. */
		return NULL;

	system_monitor_loop(cc_cfg);

	pthread_exit(NULL);
	return NULL;
}

/*
 * system_monitor_loop() - Start the system monitoring loop
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) where the
 * 			settings parsed from the configuration file are stored.
 *
 * This loop reads the values of the parameters to monitor every
 * 'cc_cfg->sys_mon_sample_rate' seconds and send them to Device Cloud when the
 * number of samples per parameter is at least
 * 'cc_cfg->sys_mon_num_samples_upload'.
 *
 * The monitored values are:
 *   - Free memory
 *   - CPU load
 *   - CPU temperature
 */
static void system_monitor_loop(const cc_cfg_t * const cc_cfg)
{
	uint32_t n_samples_to_send = calculate_number_samples(cc_cfg);
	long n_loops = cc_cfg->sys_mon_sample_rate * 1000 / LOOP_MS;
	uint32_t count = 0;

	while (stop != CCAPI_TRUE) {
		long loop;

		add_system_samples(get_free_memory(), get_cpu_load(), get_cpu_temp(), cc_cfg);

		ccapi_dp_get_collection_points_count(dp_collection, &count);
		if (count >= n_samples_to_send && stop != CCAPI_TRUE) {
			ccapi_dp_error_t dp_error;

			/*
			 * TODO: If the connection is lost, this thread blocks at this point
			 * and does not continue collecting data points.
			 *
			 * The expected behavior is an error after a timeout to keep the
			 * sampling process, so all the collected data is sent when the
			 * connection is restored.
			 * We tried to get this by using 'ccapi_dp_send_collection_with_reply'
			 * but it seems it does not work:
			 *
			 * unsigned long timeout = 2; // seconds
			 * dp_error = ccapi_dp_send_collection_with_reply(CCAPI_TRANSPORT_TCP, dp_collection, timeout, NULL);
			 */
			log_sm_debug("Sending Data Point collection\n");
			dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
			if (dp_error != CCAPI_DP_ERROR_NONE)
				log_sm_error("system_monitor_loop(): ccapi_dp_send_collection error %d", dp_error);
		}

		for (loop = 0; loop < n_loops; loop++) {
			struct timespec sleepValue = {0};

			if (stop == CCAPI_TRUE)
				break;

			sleepValue.tv_nsec = LOOP_MS * 1000 * 1000;
			nanosleep(&sleepValue, NULL);
		}
	}
}

/*
 * init_system_monitor() - Create and initialize the system monitor data point
 *                         collection
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) where the
 * 			settings parsed from the configuration file are stored.
 *
 * Return: Error code after the initialization of the system monitor collection.
 *
 * The return value will always be 'CCAPI_DP_ERROR_NONE' unless there is any
 * problem creating the collection.
 */
static ccapi_dp_error_t init_system_monitor(const cc_cfg_t * const cc_cfg)
{
	ccapi_dp_error_t dp_error = ccapi_dp_create_collection(&dp_collection);
	if (dp_error != CCAPI_DP_ERROR_NONE) {
		log_sm_error("init_system_monitor(): ccapi_dp_create_collection error %d", dp_error);
		return dp_error;
	}

	if (cc_cfg->sys_mon_parameters & SYS_MON_MEMORY)
		ccapi_dp_add_data_stream_to_collection_extra(dp_collection,
				DATA_STREAM_MEMORY, "int32 ts_iso", DATA_STREAM_MEMORY_UNITS, NULL);
	if (cc_cfg->sys_mon_parameters & SYS_MON_LOAD)
		ccapi_dp_add_data_stream_to_collection_extra(dp_collection,
				DATA_STREAM_CPU_LOAD, "double ts_iso", DATA_STREAM_CPU_LOAD_UNITS, NULL);
	if (cc_cfg->sys_mon_parameters & SYS_MON_TEMP)
		ccapi_dp_add_data_stream_to_collection_extra(dp_collection,
				DATA_STREAM_CPU_TEMP, "double ts_iso", DATA_STREAM_CPU_TEMP_UNITS, NULL);

	return CCAPI_DP_ERROR_NONE;
}

/*
 * add_system_samples() - Add memory, CPU load, and temp values to the data
 *                        point collection
 *
 * @memory:		Free memory available in kB.
 * @load:		CPU load in %.
 * @temp:		CPU temperature in C.
 * @cc_cfg:		Connector configuration struct (cc_cfg_t) where the
 * 				settings parsed from the configuration file are stored.
 */
static void add_system_samples(unsigned long memory, double load, double temp, const cc_cfg_t * const cc_cfg)
{
	ccapi_timestamp_t *timestamp = get_timestamp();

	if (cc_cfg->sys_mon_parameters & SYS_MON_MEMORY) {
		ccapi_dp_add(dp_collection, DATA_STREAM_MEMORY, memory, timestamp);
		log_sm_debug("Free memory = %lu %s\n", memory, DATA_STREAM_MEMORY_UNITS);
	}
	if (cc_cfg->sys_mon_parameters & SYS_MON_LOAD) {
		ccapi_dp_add(dp_collection, DATA_STREAM_CPU_LOAD, load, timestamp);
		log_sm_debug("CPU load = %f%s\n", load, DATA_STREAM_CPU_LOAD_UNITS);
	}
	if (cc_cfg->sys_mon_parameters & SYS_MON_TEMP) {
		ccapi_dp_add(dp_collection, DATA_STREAM_CPU_TEMP, temp, timestamp);
		log_sm_debug("Temperature = %f%s\n", temp, DATA_STREAM_CPU_TEMP_UNITS);
	}

	if (timestamp != NULL) {
		if (timestamp->iso8601 != NULL) {
			free((char *) timestamp->iso8601);
			timestamp->iso8601 = NULL;
		}
		free(timestamp);
		timestamp = NULL;
	}
}

/*
 * get_timestamp() - Get the current timestamp of the system
 *
 * Return: The timestamp of the system.
 */
static ccapi_timestamp_t* get_timestamp(void)
{
	ccapi_timestamp_t *timestamp = NULL;
	size_t len = strlen("2016-09-27T07:07:09.546Z") + 1;
	char *date = NULL;
	time_t now;

	timestamp = (ccapi_timestamp_t*) malloc(sizeof(ccapi_timestamp_t));
	if (timestamp == NULL)
		return NULL;

	date = (char*) malloc(sizeof(char) * len);
	if (date == NULL) {
		free(timestamp);
		return NULL;
	}

	time(&now);
	if (strftime(date, len, "%FT%TZ", gmtime(&now)) > 0) {
		timestamp->iso8601 = date;
	} else {
		free(date);
		timestamp->iso8601 = "";
	}

	return timestamp;
}

/*
 * get_free_memory() - Get the free memory of the system
 *
 * Return: The free memory of the system in kB.
 */
static unsigned long get_free_memory(void)
{
	struct sysinfo info;

	if (sysinfo(&info) != 0) {
		log_sm_error("get_free_memory(): sysinfo error");
		return -1;
	}

	return info.freeram / 1024;
}

/*
 * get_cpu_load() - Get the CPU load of the system
 *
 * Return: The CPU load in %.
 */
static double get_cpu_load(void)
{
	char *file_data = NULL;
	long file_size;
	unsigned long long int fields[10];
	unsigned long long idle, total = 0;
	unsigned long long diff_idle, diff_total;
	double usage;
	int result;
	int i;

	file_size = read_file(FILE_CPU_LOAD, &file_data, 4096);
	if (file_size <= 0) {
		log_sm_error("get_cpu_load(): error reading cpu load file");
		return -1;
	}

	result = sscanf(file_data, "cpu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
			&fields[0], &fields[1], &fields[2], &fields[3], &fields[4],
			&fields[5], &fields[6], &fields[7], &fields[8], &fields[9]);
	free(file_data);
	if (result < 4) {
		log_sm_error("get_cpu_load(): cpu load not enough fields error");
		return -1;
	}

	idle = fields[3];
	for (i = 0; i < 10; i++)
		total += fields[i];

	diff_idle = idle - last_idle;
	diff_total = total - last_total;

	usage = ((diff_total - diff_idle) / (double) diff_total) * 100;
	last_total = total;
	last_idle = idle;

	return usage;
}

/*
 * get_cpu_load() - Get the CPU temperature of the system
 *
 * Return: The CPU temperature in C.
 */
static double get_cpu_temp(void)
{
	char *file_data = NULL;
	long file_size;
	double temperature;
	int result;

	file_size = read_file(FILE_CPU_TEMP, &file_data, 1024);
	if (file_size <= 0) {
		log_sm_error("get_cpu_temp(): Error reading cpu temperature file");
		return -1;
	}

	result = sscanf(file_data, "%lf", &temperature);
	free(file_data);
	if (result < 1) {
		log_sm_error("get_cpu_temp(): cpu temp not enough fields error");
		return -1;
	}
	return temperature / 1000;
}

/*
 * calculate_number_samples() - Calculate the number of samples to be uploaded
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) where the
 * 			settings parsed from the configuration file are stored.
 *
 * Return: The number of samples in a collection to be uploaded to Device Cloud.
 */
static uint32_t calculate_number_samples(const cc_cfg_t * const cc_cfg)
{
	uint32_t channels = 0;
	if (cc_cfg->sys_mon_parameters & SYS_MON_MEMORY)
		channels++;
	if (cc_cfg->sys_mon_parameters & SYS_MON_LOAD)
		channels++;
	if (cc_cfg->sys_mon_parameters & SYS_MON_TEMP)
		channels++;

	return channels * cc_cfg->sys_mon_num_samples_upload;
}

/**
 * read_file() - Read the given file and returns its contents
 *
 * @path:		Absolute path of the file to read.
 * @buffer:		Buffer to store the contents of the file.
 * @file_size:	The number of bytes to read.
 *
 * Return: The number of read bytes.
 */
static long read_file(const char *path, char **buffer, long file_size)
{
	FILE *fd = NULL;
	long read_size;

	if ((fd = fopen(path, "rb")) == NULL) {
		log_sm_error("read_file(): fopen error: %s", path);
		return -1;
	}

	*buffer = (char*) malloc(sizeof(char) * file_size);
	if (*buffer == NULL) {
		log_sm_error("read_file(): malloc error: %s", path);
		fclose(fd);
		return -1;
	}

	read_size = fread(*buffer, sizeof(char), file_size, fd);
	if (ferror(fd)) {
		log_sm_error("read_file(): fread error: %s", path);
		read_size = -1;
	}

	fclose(fd);
	return read_size;
}
