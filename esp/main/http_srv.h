/*
 * http_srv.h
 *
 *  Created on: 9 ���. 2019 �.
 *      Author: Administrator
 */

#ifndef MAIN_HTTP_SRV_H_
#define MAIN_HTTP_SRV_H_

#include <esp_http_server.h>

#define	DELEMITER_CHAR	'~'			//������ � ����� ����� ���������� � html

extern httpd_handle_t http_server;

httpd_handle_t start_webserver(void);
void stop_webserver(httpd_handle_t server);

#endif /* MAIN_HTTP_SRV_H_ */
