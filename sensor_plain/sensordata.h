#include <stdbool.h>

// how about that:
typedef struct {
	uint8_t type;
	uint8_t index;
	uint8_t checksum;


	#ifdef BATTERY_POWERED
		uint16_t battery_level;
	#endif

	// sonic range sensor. transmitting echo return time
	#ifdef SENSOR_TYPE == 1
		uint16_t sonic_range;
	#endif

	// temperature and air humidity sensor a la dh22
	#ifdef SENSOR_TYPE == 2
		uint16_t temperature;
		uint16_t humidity;
	#endif

	// mailbox sensor. notification sent if (mail arrived | mailbox cleared )
	#ifdef SENSOR_TYPE == 3
		bool has_mail;
	#endif

} sensor_data;