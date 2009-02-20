#include "voxve_log.h"

#include "utils.h"

voxve_status_t voxve_logging_reconfigure(int log_level, int console_log_level, char * filename)
{
	voxve_logging_config_t config;

	// set default
	logging_config_default(&config);

	config.level = log_level;
	config.console_level = console_log_level;

	if (filename != NULL)
	{
		config.log_filename = pj_str(filename);
	}

	return logging_reconfigure(&config);
}
