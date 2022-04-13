/*
 * Copyright (c) 2017-2022 Digi International Inc.
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
 * Digi International Inc., 9350 Excelsior Blvd., Suite 700, Hopkins, MN 55343
 * ===========================================================================
 */

#include <net/if.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <unistd.h>

#include "ccapi/ccapi.h"
#include "cc_config.h"
#include "cc_init.h"
#include "cc_logging.h"
#include "cc_system_monitor.h"
#include "network_utils.h"
#include "file_utils.h"

/*------------------------------------------------------------------------------
                             D E F I N I T I O N S
------------------------------------------------------------------------------*/
#define LOOP_MS						100

#define MAX_LENGTH					256

#define SYSTEM_MONITOR_TAG			"SYSMON:"

#define DATA_STREAM_FREE_MEMORY		"system_monitor/free_memory"
#define DATA_STREAM_USED_MEMORY		"system_monitor/used_memory"
#define DATA_STREAM_CPU_LOAD		"system_monitor/cpu_load"
#define DATA_STREAM_CPU_TEMP		"system_monitor/cpu_temperature"
#define DATA_STREAM_FREQ			"system_monitor/frequency"
#define DATA_STREAM_UPTIME			"system_monitor/uptime"

#define DATA_STREAM_NET_STATE		"system_monitor/%s/state"
#define DATA_STREAM_NET_TRAFFIC_RX	"system_monitor/%s/rx_bytes"
#define DATA_STREAM_NET_TRAFFIC_TX	"system_monitor/%s/tx_bytes"

#define DATA_STREAM_MEMORY_UNITS	"kB"
#define DATA_STREAM_CPU_LOAD_UNITS	"%"
#define DATA_STREAM_CPU_TEMP_UNITS	"C"
#define DATA_STREAM_FREQ_UNITS		"kHz"
#define DATA_STREAM_UPTIME_UNITS	"s"
#define DATA_STREAM_STATE_UNITS		"state"
#define DATA_STREAM_BYTES_UNITS		"bytes"

#define FILE_CPU_LOAD				"/proc/stat"
#define FILE_CPU_TEMP				"/sys/class/thermal/thermal_zone0/temp"
#define FILE_CPU_FREQ				"/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq"

/*------------------------------------------------------------------------------
                 D A T A    T Y P E S    D E F I N I T I O N S
------------------------------------------------------------------------------*/
typedef enum {
	STREAM_FREE_MEM,
	STREAM_USED_MEM,
	STREAM_CPU_LOAD,
	STREAM_CPU_TEMP,
	STREAM_FREQ,
	STREAM_UPTIME,
	STREAM_STATE,
	STREAM_RX_BYTES,
	STREAM_TX_BYTES,
} stream_type_t;

typedef struct {
	char *name;
	char *path;
	const char *units;
	const char *format;
	stream_type_t type;
} stream_t;

typedef struct {
	stream_t *streams;
	int n_streams;
} net_stream_list_t;

/*------------------------------------------------------------------------------
                    F U N C T I O N  D E C L A R A T I O N S
------------------------------------------------------------------------------*/
static void *system_monitor_threaded(void *cc_cfg);
static void system_monitor_loop(const cc_cfg_t *const cc_cfg);
static ccapi_dp_error_t init_system_monitor(void);
static ccapi_dp_error_t init_iface_streams(const char *const iface_name, net_stream_list_t *stream_list);
static ccapi_dp_error_t init_net_streams(void);
static void add_system_samples(void);
static void add_net_samples(ccapi_timestamp_t timestamp);
static void add_bt_samples(ccapi_timestamp_t timestamp);
static ccapi_timestamp_t *get_timestamp(void);
static unsigned long get_free_memory(void);
static unsigned long get_used_memory(void);
static double get_cpu_load(void);
static double get_cpu_temp(void);
static unsigned long get_cpu_freq(void);
static unsigned long get_uptime(void);
static int get_n_ifaces(void);
static void free_stream_list(net_stream_list_t *stream_list);
static void free_timestamp(ccapi_timestamp_t *timestamp);

/*------------------------------------------------------------------------------
                                  M A C R O S
------------------------------------------------------------------------------*/
/**
 * log_sm_debug() - Log the given message as debug
 *
 * @format:		Debug message to log.
 * @args:		Additional arguments.
 */
#define log_sm_debug(format, ...)									\
	log_debug("%s " format, SYSTEM_MONITOR_TAG, __VA_ARGS__)

/**
 * log_sm_info() - Log the given message as info
 *
 * @format:		Info message to log.
 * @args:		Additional arguments.
 */
#define log_sm_info(format, ...)									\
	log_info("%s " format, SYSTEM_MONITOR_TAG, __VA_ARGS__)

/**
 * log_sm_error() - Log the given message as error
 *
 * @format:		Error message to log.
 * @args:		Additional arguments.
 */
#define log_sm_error(format, ...)									\
	log_error("%s " format, SYSTEM_MONITOR_TAG, __VA_ARGS__)

/*------------------------------------------------------------------------------
                         G L O B A L  V A R I A B L E S
------------------------------------------------------------------------------*/
static volatile bool stop_requested = false;
static volatile ccapi_bool_t dp_thread_valid = CCAPI_FALSE;
static pthread_t dp_thread;
static ccapi_dp_collection_handle_t dp_collection;
static unsigned long long last_work = 0, last_total = 0;
static net_stream_list_t net_stream_list = {
	.n_streams = 0
};
static net_stream_list_t bt_stream_list = {
	.n_streams = 0
};
static stream_t net_stream_formats[] = {
	{
		.name = "state",
		.path = DATA_STREAM_NET_STATE,
		.units = DATA_STREAM_STATE_UNITS,
		.format = "int32 ts_iso",
		.type = STREAM_STATE
	},
	{
		.name = "rx_bytes",
		.path = DATA_STREAM_NET_TRAFFIC_RX,
		.units = DATA_STREAM_BYTES_UNITS,
		.format = "int64 ts_iso",
		.type = STREAM_RX_BYTES
	},
	{
		.name = "tx_bytes",
		.path = DATA_STREAM_NET_TRAFFIC_TX,
		.units = DATA_STREAM_BYTES_UNITS,
		.format = "int64 ts_iso",
		.type = STREAM_TX_BYTES
	},
};
static stream_t sys_streams[] = {
	{
		.name = "free memory",
		.path = DATA_STREAM_FREE_MEMORY,
		.units = DATA_STREAM_MEMORY_UNITS,
		.format = "int32 ts_iso",
		.type = STREAM_FREE_MEM
	},
	{
		.name = "used memory",
		.path = DATA_STREAM_USED_MEMORY,
		.units = DATA_STREAM_MEMORY_UNITS,
		.format = "int32 ts_iso",
		.type = STREAM_USED_MEM
	},
	{
		.name = "CPU load",
		.path = DATA_STREAM_CPU_LOAD,
		.units = DATA_STREAM_CPU_LOAD_UNITS,
		.format = "double ts_iso",
		.type = STREAM_CPU_LOAD
	},
	{
		.name = "CPU temperature",
		.path = DATA_STREAM_CPU_TEMP,
		.units = DATA_STREAM_CPU_TEMP_UNITS,
		.format = "double ts_iso",
		.type = STREAM_CPU_TEMP
	},
	{
		.name = "CPU frequency",
		.path = DATA_STREAM_FREQ,
		.units = DATA_STREAM_FREQ_UNITS,
		.format = "int32 ts_iso",
		.type = STREAM_FREQ
	},
	{
		.name = "uptime",
		.path = DATA_STREAM_UPTIME,
		.units = DATA_STREAM_UPTIME_UNITS,
		.format = "int32 ts_iso",
		.type = STREAM_UPTIME
	}
};

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
cc_sys_mon_error_t start_system_monitor(const cc_cfg_t *const cc_cfg)
{
	pthread_attr_t attr;
	int error;

	if (!(cc_cfg->services & SYS_MONITOR_SERVICE) || cc_cfg->sys_mon_sample_rate <= 0)
		return CC_SYS_MON_ERROR_NONE;

	if (is_system_monitor_running())
		return CC_SYS_MON_ERROR_NONE;

	error = pthread_attr_init(&attr);
	if (error != 0) {
		/* On Linux this function always succeeds. */
		log_sm_error("pthread_attr_init() error %d", error);
	}
	error = pthread_create(&dp_thread, &attr, system_monitor_threaded, (void *) cc_cfg);
	if (error != 0) {
		log_sm_error("Error while starting the system monitor, %d", error);
		pthread_attr_destroy(&attr);
		return CC_SYS_MON_ERROR_THREAD;
	}
	pthread_attr_destroy(&attr);

	return CC_SYS_MON_ERROR_NONE;
}

/*
 * is_system_monitor_running() - Check system monitor status
 *
 * Return: True if system monitor is running, false if it is not.
 */
ccapi_bool_t is_system_monitor_running(void) {
	return dp_thread_valid;
}

/*
 * stop_system_monitor() - Stop the monitoring of system variables
 */
void stop_system_monitor(void)
{
	stop_requested = true;

	if (dp_thread_valid) {
		pthread_cancel(dp_thread);
		pthread_join(dp_thread, NULL);
	}

	free_stream_list(&net_stream_list);
	free_stream_list(&bt_stream_list);
	ccapi_dp_destroy_collection(dp_collection);

	log_sm_info("%s", "Stop monitoring the system");
}

/*
 * system_monitor_threaded() - Execute the system monitoring in a new thread
 *
 * @cc_cfg:	Connector configuration struct (cc_cfg_t) where the
 * 			settings parsed from the configuration file are stored.
 */
static void *system_monitor_threaded(void *cc_cfg)
{
	ccapi_dp_error_t dp_error = init_system_monitor();
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
 * 'cc_cfg->sys_mon_sample_rate' seconds and send them to Remote Manager when
 * the number of samples per parameter is at least
 * 'cc_cfg->sys_mon_num_samples_upload'.
 *
 * The monitored values are:
 *   - Free memory
 *   - CPU load
 *   - CPU temperature
 */
static void system_monitor_loop(const cc_cfg_t *const cc_cfg)
{
	log_sm_info("%s", "Start monitoring the system");

	while (!stop_requested) {
		uint32_t n_samples_to_send = (ARRAY_SIZE(sys_streams) + net_stream_list.n_streams + bt_stream_list.n_streams) * cc_cfg->sys_mon_num_samples_upload;;
		long n_loops = cc_cfg->sys_mon_sample_rate * 1000 / LOOP_MS;
		uint32_t count = 0;
		long loop;

		add_system_samples();

		ccapi_dp_get_collection_points_count(dp_collection, &count);
		if (count >= n_samples_to_send && !stop_requested) {
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
			if (get_cloud_connection_status() == CC_STATUS_CONNECTED) {
				log_sm_debug("%s", "Sending system monitor samples");
				dp_error = ccapi_dp_send_collection(CCAPI_TRANSPORT_TCP, dp_collection);
				if (dp_error != CCAPI_DP_ERROR_NONE)
					log_sm_error("Error sending system monitor samples, %d", dp_error);
			}
		}

		for (loop = 0; loop < n_loops; loop++) {
			struct timespec sleepValue = {0};

			if (stop_requested)
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
 * Return: Error code after the initialization of the system monitor collection.
 *
 * The return value will always be 'CCAPI_DP_ERROR_NONE' unless there is any
 * problem creating the collection.
 */
static ccapi_dp_error_t init_system_monitor(void)
{
	unsigned int i;

	ccapi_dp_error_t dp_error = ccapi_dp_create_collection(&dp_collection);
	if (dp_error != CCAPI_DP_ERROR_NONE) {
		log_sm_error("Error initalizing system monitor, %d", dp_error);
		return dp_error;
	}

	for (i = 0; i < sizeof(sys_streams) / sizeof(sys_streams[0]); i++) {
		stream_t stream = sys_streams[i];

		dp_error = ccapi_dp_add_data_stream_to_collection_extra(dp_collection,
				stream.path, stream.format, stream.units, NULL);
		if (dp_error != CCAPI_DP_ERROR_NONE) {
			log_sm_error("Cannot add '%s' stream to data point collection, error %d",
					stream.path, dp_error);
			return dp_error;
		}
	}

	dp_error = init_net_streams();
	if (dp_error != CCAPI_DP_ERROR_NONE)
		return dp_error;

	bt_stream_list.streams = calloc(1 * ARRAY_SIZE(net_stream_formats), sizeof(stream_t));
	if (bt_stream_list.streams == NULL) {
		log_sm_error("Cannot get Bluetooth interfaces: %s", "Out of memory");
		free_stream_list(&net_stream_list);
		return CCAPI_DP_ERROR_INSUFFICIENT_MEMORY;
	}

	dp_error = init_iface_streams("hci0", &bt_stream_list);
	if (dp_error != CCAPI_DP_ERROR_NONE) {
		free_stream_list(&net_stream_list);
		free_stream_list(&bt_stream_list);
	}

	return dp_error;
}

/*
 * init_net_streams() - Add the network interfaces data point streams to
 *                      collection
 *
 * Return: Error code after the addition to the collection.
 *
 * The return value will always be 'CCAPI_DP_ERROR_NONE' unless there is any
 * problem creating the collection.
 */
static ccapi_dp_error_t init_net_streams(void)
{
	struct if_nameindex *if_name_idx = NULL, *iface;
	ccapi_dp_error_t dp_error;
	int n_ifaces = get_n_ifaces();

	if (n_ifaces == 0)
		return CCAPI_DP_ERROR_NONE;

	net_stream_list.streams = calloc(n_ifaces * ARRAY_SIZE(net_stream_formats), sizeof(stream_t));
	if (net_stream_list.streams == NULL) {
		log_sm_error("Cannot get network interfaces: %s", "Out of memory");
		dp_error = CCAPI_DP_ERROR_INSUFFICIENT_MEMORY;
		goto error;
	}

	if_name_idx = if_nameindex();
	for (iface = if_name_idx; iface->if_index != 0 || iface->if_name != NULL; iface++) {
		dp_error = init_iface_streams(iface->if_name, &net_stream_list);
		if (dp_error != CCAPI_DP_ERROR_NONE)
			goto error;
	}

	dp_error = CCAPI_DP_ERROR_NONE;

error:
	if (dp_error != CCAPI_DP_ERROR_NONE)
		free_stream_list(&net_stream_list);

	if_freenameindex(if_name_idx);

	return dp_error;
}

/*
 * init_iface_streams() - Add to collection the given interface data point streams
 *
 * @iface_name:		Name of the interface to init.
 * @stream_list:	Structure to initialize.
 *
 * Return: Error code after the addition to the collection.
 *
 * The return value will always be 'CCAPI_DP_ERROR_NONE' unless there is any
 * problem creating the collection.
 */
static ccapi_dp_error_t init_iface_streams(const char *const iface_name, net_stream_list_t *stream_list)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(net_stream_formats); i++) {
		stream_t stream_format = net_stream_formats[i];
		stream_t *stream = &stream_list->streams[stream_list->n_streams];
		size_t path_len = snprintf(NULL, 0, stream_format.path, iface_name);
		ccapi_dp_error_t dp_error;

		stream_list->n_streams++;

		stream->name = strdup(iface_name);
		stream->path = calloc(path_len + 1, sizeof(char));
		if (stream->name == NULL || stream->path == NULL) {
			log_sm_error("Cannot get network interfaces: %s", "Out of memory");
			return CCAPI_DP_ERROR_INSUFFICIENT_MEMORY;
		}

		sprintf(stream->path, stream_format.path, iface_name);

		stream->format = stream_format.format;
		stream->units = stream_format.units;
		stream->type = stream_format.type;

		dp_error = ccapi_dp_add_data_stream_to_collection_extra(
					dp_collection, stream->path, stream->format, stream->units, NULL);
		if (dp_error != CCAPI_DP_ERROR_NONE) {
			log_sm_error("Cannot add '%s' stream to data point collection, error %d",
				stream->path, dp_error);
			return dp_error;
		}
	}

	return CCAPI_DP_ERROR_NONE;
}

/*
 * add_system_samples() - Add free and used memory, CPU load and temp, system
 *                        frequency, uptime, and network traffic values to the
 *                        data point collection
 */
static void add_system_samples(void)
{
	ccapi_dp_error_t dp_error;
	unsigned int i;
	ccapi_timestamp_t *timestamp = get_timestamp();
	unsigned long free_mem = get_free_memory();
	unsigned long used_mem = get_used_memory();
	double load = get_cpu_load();
	double temp =  get_cpu_temp();
	unsigned long freq = get_cpu_freq();
	unsigned long uptime = get_uptime();

	for (i = 0; i < sizeof(sys_streams) / sizeof(sys_streams[0]); i++) {
		stream_t stream = sys_streams[i];

		switch(stream.type) {
			case STREAM_FREE_MEM:
				dp_error = ccapi_dp_add(dp_collection, stream.path, free_mem, timestamp);
				log_sm_debug("%s = %lu %s", stream.name, free_mem, stream.units);
				break;
			case STREAM_USED_MEM:
				dp_error = ccapi_dp_add(dp_collection, stream.path, used_mem, timestamp);
				log_sm_debug("%s = %lu %s", stream.name, used_mem, stream.units);
				break;
			case STREAM_CPU_LOAD:
				dp_error = ccapi_dp_add(dp_collection, stream.path, load, timestamp);
				log_sm_debug("%s = %f %s", stream.name, load, stream.units);
				break;
			case STREAM_CPU_TEMP:
				dp_error = ccapi_dp_add(dp_collection, stream.path, temp, timestamp);
				log_sm_debug("%s = %f %s", stream.name, temp, stream.units);
				break;
			case STREAM_FREQ:
				dp_error = ccapi_dp_add(dp_collection, stream.path, freq, timestamp);
				log_sm_debug("%s = %lu %s", stream.name, freq, stream.units);
				break;
			case STREAM_UPTIME:
				dp_error = ccapi_dp_add(dp_collection, stream.path, uptime, timestamp);
				log_sm_debug("%s = %lu %s", stream.name, uptime, stream.units);
				break;
			default:
				/* Should not occur */
				break;
		}

		if (dp_error != CCAPI_DP_ERROR_NONE)
			log_sm_error("Cannot add %s value, %d", stream.name, dp_error);
	}

	add_net_samples(*timestamp);

	add_bt_samples(*timestamp);

	free_timestamp(timestamp);
}

/*
 * add_net_samples() - Add network interfaces RX and TX bytes values to the
 *                     data point collection
 *
 * @timestamp: The timestamp for the samples.
 */
static void add_net_samples(ccapi_timestamp_t timestamp)
{
	net_stats_t stats;
	iface_info_t iface_info;
	char *iface_name = NULL;
	int i;

	for (i = 0; i < net_stream_list.n_streams; i++) {
		char desc[50] = {0};
		unsigned long long value = 0;
		ccapi_dp_error_t dp_error;
		stream_t stream = net_stream_list.streams[i];

		if (iface_name == NULL || strcmp(iface_name, stream.name) != 0) {
			iface_name = stream.name;
			get_net_stats(iface_name, &stats);
			get_iface_info(iface_name, &iface_info);
		}

		switch(stream.type) {
			case STREAM_STATE:
				value = iface_info.enabled;
				strcpy(desc, " status");
				break;
			case STREAM_RX_BYTES:
				value = stats.rx_bytes;
				strcpy(desc, " RX bytes");
				break;
			case STREAM_TX_BYTES:
				value = stats.tx_bytes;
				strcpy(desc, " TX bytes");
				break;
			default:
				/* Should not occur */
				strcpy(desc, "");
				break;
		}

		dp_error = ccapi_dp_add(dp_collection, stream.path, value, &timestamp);

		if (dp_error != CCAPI_DP_ERROR_NONE)
			log_sm_error("Cannot add %s%s value, %d", stream.name, desc, dp_error);
		else
			log_sm_debug("%s%s = %llu %s", stream.name, desc, value, stream.units);
	}
}

/*
 * add_bt_samples() - Add Bluetooth interface RX and TX bytes values to the
 *                    data point collection
 *
 * @timestamp: The timestamp for the samples.
 */
static void add_bt_samples(ccapi_timestamp_t timestamp)
{
	bt_info_t bt_info;
	char *iface_name = NULL;
	int i;

	for (i = 0; i < bt_stream_list.n_streams; i++) {
		char desc[50] = {0};
		unsigned long long value = 0;
		ccapi_dp_error_t dp_error;
		stream_t stream = bt_stream_list.streams[i];

		if (iface_name == NULL || strcmp(iface_name, stream.name) != 0) {
			iface_name = stream.name;
			get_bt_info(iface_name, &bt_info);
		}

		switch(stream.type) {
			case STREAM_STATE:
				value = bt_info.enabled;
				strcpy(desc, " status");
				break;
			case STREAM_RX_BYTES:
				value = bt_info.stats.rx_bytes;
				strcpy(desc, " RX bytes");
				break;
			case STREAM_TX_BYTES:
				value = bt_info.stats.tx_bytes;
				strcpy(desc, " TX bytes");
				break;
			default:
				/* Should not occur */
				strcpy(desc, "");
				break;
		}

		dp_error = ccapi_dp_add(dp_collection, stream.path, value, &timestamp);

		if (dp_error != CCAPI_DP_ERROR_NONE)
			log_sm_error("Cannot add %s%s value, %d", stream.name, desc, dp_error);
		else
			log_sm_debug("%s%s = %llu %s", stream.name, desc, value, stream.units);
	}
}

/*
 * get_timestamp() - Get the current timestamp of the system
 *
 * Return: The timestamp of the system.
 */
static ccapi_timestamp_t *get_timestamp(void)
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
		timestamp->iso8601 = strdup("");
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
		log_sm_error("%s", "Error getting free memory");
		return -1;
	}

	return info.freeram / 1024;
}

/*
 * get_used_memory() - Get the usded memory of the system
 *
 * Return: The used memory of the system in kB, -1 if error.
 */
static unsigned long get_used_memory(void)
{
	struct sysinfo info;

	if (sysinfo(&info) != 0) {
		log_sm_error("%s", "Error getting used memory");
		return -1;
	}

	return (info.totalram - info.freeram) / 1024;
}

/*
 * get_cpu_load() - Get the CPU load of the system
 *
 * Return: The CPU load in %, -1 if the value is not available.
 */
static double get_cpu_load(void) {
	char file_data[MAX_LENGTH] = {0};
	long file_size;
	unsigned long long int fields[10];
	unsigned long long work = 0, total = 0;
	double usage = -1;
	int i, result;

	file_size = read_file(FILE_CPU_LOAD, file_data, MAX_LENGTH);
	if (file_size <= 0) {
		log_sm_error("%s", "Error getting CPU load");
		return -1;
	}

	result = sscanf(file_data, "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
			&fields[0], &fields[1], &fields[2], &fields[3], &fields[4],
			&fields[5], &fields[6], &fields[7], &fields[8], &fields[9]);

	if (result < 4) {
		log_sm_error("%s", "Error getting CPU load");
		return -1;
	}

	for (i = 0; i < 3; i++)
		work += fields[i];
	for (i = 0; i < result; i++)
		total += fields[i];

	if (last_work == 0 && last_total == 0) {
		/* The first time report 0%. */
		usage = 0;
	} else {
		unsigned long long diff_work = work - last_work;
		unsigned long long diff_total = total - last_total;

		usage = diff_work * 100.0 / diff_total;
	}

	last_total = total;
	last_work = work;

	return usage;
}

/*
 * get_cpu_temp() - Get the CPU temperature of the system
 *
 * Return: The CPU temperature in C.
 */
static double get_cpu_temp(void)
{
	char file_data[MAX_LENGTH] = {0};
	long file_size;
	double temperature;
	int result;

	file_size = read_file(FILE_CPU_TEMP, file_data, MAX_LENGTH);
	if (file_size <= 0) {
		log_sm_error("%s", "Error getting CPU temperature");
		return -1;
	}

	result = sscanf(file_data, "%lf", &temperature);
	if (result < 1) {
		log_sm_error("%s", "Error getting CPU temperature");
		return -1;
	}

	return temperature / 1000;
}

/*
 * get_cpu_freq() - Get the CPU frequency
 *
 * Return: The CPU frequency in kHz, -1 if error.
 */
static unsigned long get_cpu_freq(void)
{
	char data[MAX_LENGTH] = {0};
	long file_size;
	long freq;

	freq = -1;

	file_size = read_file(FILE_CPU_FREQ, data, MAX_LENGTH);
	if (file_size <= 0) {
		log_sm_error("%s", "Error getting CPU frequency");
		return -1;
	}

	if (sscanf(data, "%ld", &freq) < 1) {
		log_sm_error("%s", "Error getting CPU frequency");
		return -1;
	}

	return freq;
}

/*
 * get_uptime() - Get number of seconds since boot
 *
 * Return: Number of seconds since boot.
 */
static unsigned long get_uptime(void)
{
	struct sysinfo info;

	if (sysinfo(&info) != 0) {
		log_sm_error("%s", "Error getting uptime");
		return -1;
	}

	return info.uptime;
}

/*
 * get_n_ifaces() - Get number of network interfaces
 *
 * Return: Number of network interfaces.
 */
static int get_n_ifaces(void)
{
	struct if_nameindex *if_ni, *iface;
	int if_counter = 0;

	if_ni = if_nameindex();

	if (if_ni == NULL)
		return 0;

	for (iface = if_ni; iface->if_index != 0 || iface->if_name != NULL; iface++)
		if_counter++;

	if_freenameindex(if_ni);

	return if_counter;
}

/*
 * free_stream_list() - Free the stream list
 *
 * @stream_list:	The list to free.
 */
static void free_stream_list(net_stream_list_t *stream_list)
{
	int i;

	for (i = 0; i < stream_list->n_streams; i++) {
		free(stream_list->streams[i].name);
		free(stream_list->streams[i].path);
	}

	free(stream_list->streams);

	stream_list->n_streams = 0;
}

/*
 * free_timestamp() - Free given timestamp structure
 *
 * @timestamp:	The timestamp structure to release.
 */
static void free_timestamp(ccapi_timestamp_t *timestamp)
{
	if (timestamp == NULL)
		return;

	if (timestamp->iso8601 != NULL) {
		free((char *) timestamp->iso8601);
		timestamp->iso8601 = NULL;
	}
	free(timestamp);
	timestamp = NULL;
}
