/*
 * storage current version and compare with custom version.
 *
 */

#ifndef MAIN_VERSION_H_
#define MAIN_VERSION_H_

#include <stdint.h>
#include "esp_log.h"


#define VERSION_MAJOR_NUM		0
#define VERSION_MINOR_NUM		0
#define VERSION_PATCH_NUM		0
#define VERSION_BUILD_NUM		9

#define VERSION_DELEMITER		'.'
#define VERSION_MAX_DIGIT		6 //+ null symbol

#define VERSION_PART_COUNT		4
#define VERSION_PART_MAJOR		0
#define VERSION_PART_MINOR		1
#define VERSION_PART_PATCH		2
#define VERSION_PART_BUILD		3

typedef struct {
    uint16_t part[VERSION_PART_COUNT]; //major.minor.patch.build
} version_app_t;

extern const version_app_t VERSION_APPLICATION;

//verify current version with value version
bool needUpdate(char *version);

#endif /* MAIN_VERSION_H_ */
