/*
* get time from sntp server
*/

#ifndef MAIN_SNTP_CLIENT_H_
#define MAIN_SNTP_CLIENT_H_

#include "esp_sntp.h"
//setting function cb
void sntp_init_app(sntp_sync_time_cb_t sync_time_cb);

#endif /* MAIN_SNTP_CLIENT_H_ */
