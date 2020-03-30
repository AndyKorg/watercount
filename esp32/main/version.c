/*
 * version.c
 *
 */

#include <string.h>
#include <stdlib.h>
#include "esp_err.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "version.h"

const version_app_t VERSION_APPLICATION = {{VERSION_MAJOR_NUM, VERSION_MINOR_NUM, VERSION_PATCH_NUM, VERSION_BUILD_NUM}};

static const char *TAG = "VER";

static bool checkNumberVersionPart(char *prevPtr, char *ptr, const uint8_t versionPart){

	char tmp[VERSION_MAX_DIGIT] = {'\0'};
	uint8_t i=0;

	for(;(prevPtr != ptr) && (i<=VERSION_MAX_DIGIT); prevPtr++){
		tmp[i++] = *prevPtr;
		}
	tmp[i] = '\0';
	ESP_LOGI(TAG, "check i=%d cur=%d check = %d", versionPart, VERSION_APPLICATION.part[versionPart], atoi(tmp));
	return VERSION_APPLICATION.part[versionPart] < atoi(tmp);
}
/*
 * verify current version with value version
 */
bool needUpdate(char *version){
	bool ret = false;

	if (version){
		char *prevptr = version, *endptr = (version + strlen(version));

		uint8_t countPart = 0;
		for (char *ptr = strchr(version, '.'); ptr && (countPart < VERSION_PART_COUNT); ptr = strchr(++ptr, '.')){
			ret = checkNumberVersionPart(prevptr, ptr, countPart);
			if (ret){
				break;
			}
			countPart++;
			prevptr = ptr+1;
		}
		if ((!ret) && (prevptr <  endptr) && (countPart < VERSION_PART_COUNT)){//last part number version
			ret = checkNumberVersionPart(prevptr, endptr, countPart);
		}
	}
	return ret;
}
