/*
 * http_srv.h
 *
 *  Created on: 9 июн. 2019 г.
 *      Author: Administrator
 */

#ifndef MAIN_HTTP_SRV_H_
#define MAIN_HTTP_SRV_H_

#include <esp_http_server.h>

#define	DELEMITER_CHAR	'~'			//Начало и конец имени переменной в html

extern httpd_handle_t http_server;

httpd_handle_t start_webserver(void);
void stop_webserver(httpd_handle_t server);

#endif /* MAIN_HTTP_SRV_H_ */
