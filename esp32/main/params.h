/*
 * registration of parameters in the array and their search.
 *
 */

#ifndef MAIN_PARAMS_H_
#define MAIN_PARAMS_H_

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define PARAM_NAME_LEN			16	//maximum length parameter name, from nvs
typedef char *paramName_t;

/**
 * Handler to call read parameter function. This must
 * return ESP_OK.
 */
typedef esp_err_t (*handler_read)(const paramName_t paramName, char *value, size_t maxLen);
/**
 * Handler to call write parameter function. This must
 * return ESP_OK.
 */
typedef esp_err_t (*handler_write)(const paramName_t paramName, const char *value, size_t maxLen);

/**
 * Handler to call write parameter in storage function.
 * Variables must be prepared by handler_write
 * This must return ESP_OK.
 */
typedef esp_err_t (*handler_save)(void);


/**
 * @brief      register parameter name and function for reading or writing it.
 * 			   Only registering - or inserting, or replacing if paramName is exists.
 *
 * @param[in]  paramName Name parameter, maximum len less them len nvs parameter name
 * @param[in]  maxLen    Maximum length of parameter value
 * @param[in]  cb_read	 Pointer function read of parameter value
 * @param[in]  cb_write	 Pointer function write of parameter value
 * @param[in]  cb_commit Pointer function save of parameter value to storage
 *
 * @return
 *             - ESP_OK if parameter name was set successfully
 */
esp_err_t paramReg(const paramName_t paramName, const size_t maxLen, const handler_read cb_read, handler_write cb_write, handler_save cb_commit);
/**
 * @brief      read parameter value and length
 * @param[in]  paramName Name parameter, maximum len less them len nvs parameter name
 * @param[in]  bufValue pointer to the buffer for the value.
 * 						if NULL it returns only the size without reading the parameter itself.
 * @return
 * 				if 0 then the parameter is not found.
 */
uint8_t paramRead(const paramName_t paramName, char *bufValue);
esp_err_t paramWrite(const paramName_t paramName, const char *value, handler_save *save_ptr);
/*
 * base read function
 */
esp_err_t read_nvs_param(const char * nvs_area, const paramName_t paramName, char *value, size_t maxLen);

#endif /* MAIN_PARAMS_H_ */
