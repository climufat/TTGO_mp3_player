#ifndef _DEFINES_H_
#define _DEFINES_H_ 

#include <stdio.h>


#define MAIN_TAG "main:"
// typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
// typedef int (*http_cb) (http_parser*);

#define MOUNT_POINT "/sdcard"
#define INDEX_FILE "/index.txt"
#define MAX_FILES 1000

#define GPIO_OUTPUT_IO_0    GPIO_NUM_5
#define GPIO_OUTPUT_PIN_SEL  ((1<<GPIO_OUTPUT_IO_0))

#define WS2812_PIN	22

/* Most development boards have "boot" button attached to GPIO0.
 * You can also change this to another pin.
 */
#define BUTTON_GPIO_NUM_DEFAULT     0
/* "Boot" button on GPIO0 is active low */
#define BUTTON_WAKEUP_LEVEL_DEFAULT 0


#endif