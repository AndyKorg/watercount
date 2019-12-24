/*
 *
 */

#include "params.h"
#include "string.h"
#include "search.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "prm";

typedef struct {
	size_t MaxLen;
	handler_read cb_read;
	handler_write cb_write;
	handler_save cb_save;
} param_t;

typedef struct paramEntry_t {
	char *ParamName;
	struct paramEntry_t *Next;
	param_t value;
} paramEntry_t;

static paramEntry_t *arrayParam = NULL;

paramEntry_t* last(paramEntry_t *prev) {
	if (prev) {
		if (prev->Next) {
			return last(prev->Next);
		}
	}
	return prev;
}

paramEntry_t* seek(const paramName_t value, paramEntry_t *entry) {
	if (entry) {
		if (strcmp(entry->ParamName, value)) {
			if (entry->Next) {
				return seek(value, entry->Next);
			}
			return NULL;
		}
	}
	return entry;
}

esp_err_t paramReg(const paramName_t paramName, const size_t maxLen, const handler_read cb_read, handler_write cb_write, handler_save cb_save) {
	if (paramName == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	ESP_LOGI(TAG, "param reg %s", paramName);
	if (strlen(paramName) > PARAM_NAME_LEN) {
		return ESP_ERR_INVALID_ARG;
	}
	paramEntry_t *entry = seek(paramName, arrayParam), *prevEntry;
	if (entry == NULL) {
		entry = calloc(1, sizeof(paramEntry_t));
		ESP_LOGI(TAG, "param mem OK");
		if (entry == NULL) {
			return ESP_ERR_NO_MEM;
		}
		entry->ParamName = paramName;
		prevEntry = last(arrayParam);
		if (prevEntry){				//not first element
			prevEntry->Next = entry;
		}
		if (arrayParam == NULL) {	//first element
			arrayParam = entry;
		}
		ESP_LOGI(TAG, "param add %s", paramName);
	}
	entry->value.MaxLen = maxLen;
	entry->value.cb_read = cb_read;
	entry->value.cb_write = cb_write;
	entry->value.cb_save = cb_save;
	ESP_LOGI(TAG, "reg OK");
	return ESP_OK;
}

uint8_t paramRead(const paramName_t paramName, char *bufValue) {
	if (paramName) {
		ESP_LOGI(TAG, "param seek %s", paramName);
		if (strlen(paramName) <= PARAM_NAME_LEN) {
			paramEntry_t *entry = seek(paramName, arrayParam);
			if (entry) {
				ESP_LOGI(TAG, "param read %s", paramName);
				char *value = calloc(entry->value.MaxLen, sizeof(char));
				if (value) {
					uint8_t len = 0;
					ESP_LOGI(TAG, "param read start");
					if (entry->value.cb_read(paramName, value, entry->value.MaxLen) == ESP_OK) {
						ESP_LOGI(TAG, "read OK");
						if (bufValue) {
							strcpy(bufValue, value);
						}
						len = strlen(value);
						ESP_LOGI(TAG, "param value %s", value);
					}
					free(value);
					return len;
				}
			}
		}
	}
	return 0;
}

esp_err_t paramWrite(const paramName_t paramName, const char *value, handler_save *save_ptr) {
	ESP_LOGI(TAG, "param seek %s", paramName);
	if (paramName) {
		if (strlen(paramName) <= PARAM_NAME_LEN) {
			paramEntry_t *entry = seek(paramName, arrayParam);
			if (entry) {
				ESP_LOGI(TAG, "value %s", value);
				*save_ptr = entry->value.cb_save;
				ESP_LOGI(TAG, "save ptr");
				return entry->value.cb_write(paramName, value, entry->value.MaxLen);
			}
		}
	}
	return ESP_ERR_NOT_FOUND;
}
