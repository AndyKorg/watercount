/*
 * get time from sntp server
 */

#include "esp_sntp.h"

void sntp_init_app(sntp_sync_time_cb_t sync_time_cb) {

	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	sntp_set_time_sync_notification_cb(sync_time_cb);
}

