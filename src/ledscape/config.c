/** \file
 * Parse a matrix config file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "ledscape.h"


// find the end of the line and strip the whitespace
static ssize_t
readline(
	FILE * const file,
	char * const buf,
	const int max_len
)
{
	if (!fgets(buf, max_len, file))
		return -1;
	int len = strlen(buf);
	if (len >= max_len)
	{
		buf[max_len-1] = '\0';
		return max_len;
	}

	while (len > 0)
	{
		const char c = buf[len-1];
		if (!isspace(c))
			break;
		// strip the whitespace
		buf[--len] = '\0';
	}

	printf("read %d bytes '%s'\n", len, buf);
	return len;
}


ledscape_matrix_config_t *
ledscape_matrix_config(
	const char * filename
)
{
	FILE * const file = fopen(filename, "r");
	if (!file)
	{
		fprintf(stderr, "%s: unable to open\n", filename);
		return NULL;
	}

	ledscape_matrix_config_t * const config = calloc(1, sizeof(*config));
	if (!config)
		return NULL;

	char line[1024];
	int line_num = 1;

	if (readline(file, line, sizeof(line)) < 0)
		goto fail;

	if (strcmp(line, "matrix16") != 0)
		goto fail;

	config->panel_width = 32;
	config->panel_height = 16;
	config->leds_width = 256;
	config->leds_height = 128;

	while (1)
	{
		line_num++;
		if (readline(file, line, sizeof(line)) < 0)
			break;

		int output, panel, x, y;
		char orient;
		if (sscanf(line, "%d,%d %c %d,%d", &output, &panel, &orient, &x, &y) != 5)
			goto fail;
		if (output > LEDSCAPE_MATRIX_OUTPUTS)
			goto fail;
		if (panel > LEDSCAPE_MATRIX_PANELS)
			goto fail;
		if (x < 0 || y < 0)
			goto fail;

		ledscape_matrix_panel_t * const pconfig
			= &config->panels[output][panel];

		pconfig->x = x;
		pconfig->y = y;

		switch (orient)
		{
			case 'N': pconfig->rot = 0; break;
			case 'L': pconfig->rot = 1; break;
			case 'R': pconfig->rot = 2; break;
			case 'U': pconfig->rot = 3; break;
			default: goto fail;
		}
	}

	fclose(file);
	return config;

fail:
	fprintf(stderr, "%s: read or parse error on line %d\n", filename, line_num);
	fclose(file);
	free(config);
	return NULL;
}
