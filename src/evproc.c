#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>
#include <stdint.h>

#include "config.h"
#include "ptp.h"
#include "ptpcam.h"

struct EvProcFooter {
	uint32_t params; // Number of parameters
	uint32_t longpars; // Number of long parameters (string, file)
};

struct EvProcInt {
	uint32_t p1;
	uint32_t number;
	uint32_t p2;
	uint32_t p3;
	uint32_t p4;
};

enum Types {
	TOK_TEXT,
	TOK_STR,
	TOK_INT,
};

#define MAX_STR 128
#define MAX_TOK 10

// TODO: don't hardcode lengths
struct Tokens {
	struct T {
		int type;
		char string[MAX_STR];
		int integer;
	}t[MAX_TOK];
	int length;
};

int alpha(char c) {
	return (c >= 'a' && c <= 'z')
		|| (c >= 'A' && c <= 'Z')
		|| c == '_';
}

int digit(char c) {
	return (c >= '0' && c <= '9');
}

// Parse a formatted command into struct Tokens
// Should parse:
//  ThisCommand   123 "A String"
// TODO:
//  Parse hex into int
//  Parse filenames
struct Tokens parseCommand(char string[]) {
	struct Tokens toks;
	int t = 0;

	int c = 0;
	while(string[c] != '\0') {
		while (string[c] == ' ' || string[c] == '\t') {
			c++;
		}

		int s = 0;
		if (alpha(string[c])) {
			toks.t[t].type = TOK_TEXT;
			while (alpha(string[c])) {
				toks.t[t].string[s] = string[c];
				c++; s++;
			}
		} else if (digit(string[c])) {
			toks.t[t].integer = 0;
			toks.t[t].type = TOK_INT;
			while (digit(string[c])) {
				toks.t[t].integer *= 10;
				toks.t[t].integer += string[c] - '0';
				c++;
			}
		} else if (string[c] == '"') {
			toks.t[t].type = TOK_STR;
			c++;
			while (string[c] != '"') {
				toks.t[t].string[s] = string[c];
				c++; s++;
			}
			c++;
		} else {
			continue;
		}

		toks.t[t].string[s] = '\0';
		t++;
	}

	toks.length = t;
	
	return toks;
}

// Will return PTP status code.
// Returns "1" on parse error.
int evproc_run(char string[])
{
#if 0
	printf("Command AST for '%s':\n", string);
	for (int i = 0; i < toks.length; i++) {
		switch (toks.t[i].type) {
		case TOK_TEXT:
			printf("Found text token: %s\n", toks.t[i].string);
			break;
		case TOK_STR:
			printf("Found string token: \"%s\"\n", toks.t[i].string);
			break;
		case TOK_INT:
			printf("Found integer token: %d\n", toks.t[i].integer);
			break;			
		}
	}
#endif

	int busn = 0;
	int devn = 0;
	short force = 0;
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
		return 0;
	}

	// Command is disabled on some cams	
	ptp_activate_command(&params);

	struct EvProcFooter footer;
	footer.params = 0;
	footer.longpars = 0;

	struct Tokens toks = parseCommand(string);

	char data[1024];
	int curr = 0;

	if (toks.length == 0) {
		puts("Error, must have at least 1 parameter.");
		return 1;
	}

	// Add in initial parameter
	if (toks.t[0].type == TOK_TEXT) {
		int len = strlen(toks.t[0].string);
		memcpy(data, toks.t[0].string, len);
		data[len] = '\0';
		curr += len + 1;
	} else {
		puts("Error, first parameter must be plain text.");
	}

	for (int t = 1; t < toks.length; t++) {
		switch (toks.t[t].type) {
		case TOK_INT:
			{
				struct EvProcInt integer;
				integer.number = toks.t[t].integer;

				memcpy(data + curr, &integer, sizeof(struct EvProcInt));
				curr += sizeof(struct EvProcInt);

				footer.params++;
			}
			break;
		}
	}

	// Add in the evproc footer
	memcpy(data + curr, &footer, sizeof(struct EvProcFooter));
	curr += sizeof(struct EvProcFooter);

	unsigned int r = ptp_run_command(&params, data, curr);

	close_camera(&ptp_usb, &params, dev);
	return r;
}
