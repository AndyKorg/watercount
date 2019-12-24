/*
 * http_server
 *
 */

#ifndef MAIN_HTTP_SRV_H_
#define MAIN_HTTP_SRV_H_

#include <esp_http_server.h>

#define	DELEMITER_CHAR	'~'			//prefix and suffix in html-page for name variable

bool start_webserver(void);
void stop_webserver(void);

#endif /* MAIN_HTTP_SRV_H_ */
