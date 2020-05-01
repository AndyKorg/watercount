/*
 * WARNING! Without checked correct new application!
 *
 */

#ifndef MAIN_OTA_CLIENT_H_
#define MAIN_OTA_CLIENT_H_

#define DOWNLOAD_FILENAME		"watercounter"	//without extension

typedef void (*ota_end_cb_t)(void); 	//callback function for ota process end

void ota_check(void);	//check new firmware, restart if new new firmware loaded,
void ota_init(ota_end_cb_t ota_end_func);

#endif /* MAIN_OTA_CLIENT_H_ */
