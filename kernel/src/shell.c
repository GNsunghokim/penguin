#include <util/cmd.h>
#include "stdio.h"
#include "driver/charin.h"
#include "driver/charout.h"
#include "version.h"
#include "device.h"
#include "shell.h"

static bool cmd_sync;

void shell_callback() {
	void cmd_callback(char* result, int exit_status) {
		cmd_update_var(result, exit_status);
		cmd_sync = false;
	}

	static char cmd[CMD_SIZE];
	static int cmd_idx = 0;
	extern Device* device_stdout;

	if(cmd_sync)
		return;

	int ch = stdio_getchar();
	while(ch >= 0) {
		switch(ch) {
			case '\n':
				cmd[cmd_idx] = '\0';
				putchar(ch);

				int exit_status = cmd_exec(cmd, cmd_callback);
				if(exit_status != 0) {
					if(exit_status == CMD_STATUS_WRONG_NUMBER) {
						printf("Wrong number of arguments\n");
					} else if(exit_status == CMD_STATUS_NOT_FOUND) {
						printf("Command not found: %s\n", cmd);
					} else if(exit_status == CMD_VARIABLE_NOT_FOUND) {
						printf("Variable not found\n");
					} else if(exit_status < 0) {
						printf("Wrong value of argument: %d\n", -exit_status);
					}
				}

				printf("# ");
				cmd_idx = 0;
				break;
			case '\f':
				putchar(ch);
				break;
			case '\b':
				if(cmd_idx > 0) {
					cmd_idx--;
					putchar(ch);
				}
				break;
			case 0x1b:	// ESC
				ch = stdio_getchar();
				switch(ch) {
					case '[':
						ch = stdio_getchar();
						switch(ch) {
							case 0x35:
								stdio_getchar();	// drop '~'
								((CharOut*)device_stdout->driver)->scroll(device_stdout->id, -12);
								break;
							case 0x36:
								stdio_getchar();	// drop '~'
								((CharOut*)device_stdout->driver)->scroll(device_stdout->id, 12);
								break;
						}
						break;
					case 'O':
						ch = stdio_getchar();
						switch(ch) {
							case 'H':
								((CharOut*)device_stdout->driver)->scroll(device_stdout->id, INT32_MIN);
								break;
							case 'F':
								((CharOut*)device_stdout->driver)->scroll(device_stdout->id, INT32_MAX);
								break;
						}
						break;
				}
				break;
			default:
				if(cmd_idx < CMD_SIZE - 1) {
					cmd[cmd_idx++] = ch;
					putchar(ch);
				}
		}
		if(cmd_sync)
			break;

		ch = stdio_getchar();
	}
}

void shell_init() {
	printf("\nPacketNgin ver %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO, VERSION_TAG);
	printf("# ");

	extern Device* device_stdin;
	((CharIn*)device_stdin->driver)->set_callback(device_stdin->id, shell_callback);
	cmd_sync = false;
	cmd_init();
}

void shell_sync() {
	cmd_sync = true;
}
