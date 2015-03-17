/**************************************************************************
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>

 *
 * A disk activity light for the Raspberry Pi, using an LED connected to a GPIO pin.
 * Based on hddled.c - http://members.optusnet.com.au/foonly/whirlpool/code/hddled.c -
 * This program uses the WiringPi library by Gordon Henderson - http://wiringpi.com/ - Thanks, Gordon!
 * 
 *
 * To compile:
 *   gcc -Wall -O3 -o hddledPi hddledPi.c -lwiringPi
 *
 * Options:
 * -d, --detach               Detach from terminal (become a daemon)
 * -p, --pin=VALUE            GPIO pin (using wiringPi numbering scheme) where LED is connected (default: 10) 
 * -r, --refresh=VALUE        Refresh interval (default: 20 ms)
 *
 * Default LED Pin - wiringPi pin 10 is BCM_GPIO 8, physical pin 24 on the Pi's P1 header.
 * Note: This pin is also used for the SPI interface. If you have SPI add-ons connected,
 * you'll have to use the -p option to change it to another, unused pin.
 *
 * GPIO pin ----|>|----[330]----+
 *              LED             |
 *                             ===
 *                            Ground
 */


#define VMSTAT "/proc/vmstat"


#define _GNU_SOURCE

#include <argp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wiringPi.h>


static unsigned int o_refresh = 20; /* milliseconds */
static unsigned int o_gpiopin = 10; /* wiringPi numbering scheme */
static int o_detach = 0;

static volatile sig_atomic_t running = 1;
static char *line = NULL;
static size_t len = 0;

/* Reread the vmstat file */
int activity(FILE *vmstat) {
        static unsigned int prev_pgpgin, prev_pgpgout;
        unsigned int pgpgin, pgpgout;
        int found_pgpgin, found_pgpgout;
        int result;

        /* Reload the vmstat file */
        result = TEMP_FAILURE_RETRY(fseek(vmstat, 0L, SEEK_SET));
        if (result) {
                perror("Could not rewind " VMSTAT);
                return result;
        }

        /* Clear glibc's buffer */
        result = TEMP_FAILURE_RETRY(fflush(vmstat));
        if (result) {
                perror("Could not flush input stream");
                return result;
        }

        /* Extract the I/O stats */
        found_pgpgin = found_pgpgout = 0;
        while (getline(&line, &len, vmstat) != -1 && errno != EINTR) {
                if (sscanf(line, "pgpgin %u", &pgpgin))
                        found_pgpgin++;
                else if (sscanf(line, "pgpgout %u", &pgpgout))
                        found_pgpgout++;
                if (found_pgpgin && found_pgpgout)
                        break;
        }
        if (!found_pgpgin || !found_pgpgout) {
                fprintf(stderr, "Could not find required lines in " VMSTAT);
                return -1;
        }

        /* Anything changed? */
        result =
                (prev_pgpgin  != pgpgin) ||
                (prev_pgpgout != pgpgout);
        prev_pgpgin = pgpgin;
        prev_pgpgout = pgpgout;

        return result;
}

/* Update the LED */
void led(int on) {
        static int current = 1; /* Ensure the LED turns off on first call */
        if (current == on)
                return;

                if (on) {
                        digitalWrite (o_gpiopin, HIGH);
                } else {
                        digitalWrite (o_gpiopin, LOW);
                }

        current = on;
}

/* Signal handler -- break out of the main loop */
void shutdown(int sig) {
        running = 0;
}

/* Argp parser function */
error_t parse_options(int key, char *arg, struct argp_state *state) {
        switch (key) {
        case 'd':
                o_detach = 1;
                break;
        case 'r':
                o_refresh = strtol(arg, NULL, 10);
                if (o_refresh < 10)
                        argp_failure(state, EXIT_FAILURE, 0,
                                "refresh interval must be at least 10");
                break;
        case 'p':
                o_gpiopin = strtol(arg, NULL, 10);
                if ((o_gpiopin < 0) || (o_gpiopin > 29))
                        argp_failure(state, EXIT_FAILURE, 0,
                                "pin number must be between 0 and 29");
                break;
        }
        return 0;
}

int main(int argc, char **argv) {
        struct argp_option options[] = {
                { "detach",  'd',      NULL, 0, "Detach from terminal" },
                { "pin",     'p',   "VALUE", 0, "GPIO pin where LED is connected (default: wiringPi pin 10, physical pin 24 on the P1 header)" },
                { "refresh", 'r',   "VALUE", 0, "Refresh interval (default: 20 ms)" },
                { 0 },
        };
        struct argp parser = {
                NULL, parse_options, NULL,
                "Show disk activity using an LED wired to a GPIO pin.",
                NULL, NULL, NULL
        };
        int status = EXIT_FAILURE;
        FILE *vmstat = NULL;
        struct timespec delay;

        /* Parse the command-line */
        parser.options = options;
        if (argp_parse(&parser, argc, argv, ARGP_NO_ARGS, NULL, NULL))
                goto out;

        delay.tv_sec = o_refresh / 1000;
        delay.tv_nsec = 1000000 * (o_refresh % 1000);

        wiringPiSetup () ;
        pinMode (o_gpiopin, OUTPUT) ;


        /* Open the vmstat file */
        vmstat = fopen(VMSTAT, "r");
        if (!vmstat) {
                perror("Could not open " VMSTAT " for reading");
                goto out;
        }

        /* Ensure the LED is off */
        led(LOW);

        /* Save the current I/O stat values */
        if (activity(vmstat) < 0)
                goto out;

        /* Detach from terminal? */
        if (o_detach) {
                pid_t child = fork();
                if (child < 0) {
                        perror("Could not detach from terminal");
                        goto out;
                }
                if (child) {
                        /* I am the parent */
                        status = EXIT_SUCCESS;
                        goto out;
                }
        }

        /* We catch these signals so we can clean up */
        {
                struct sigaction action;
                memset(&action, 0, sizeof(action));
                action.sa_handler = shutdown;
                sigemptyset(&action.sa_mask);
                action.sa_flags = 0; /* We block on usleep; don't use SA_RESTART */
                sigaction(SIGHUP, &action, NULL);
                sigaction(SIGINT, &action, NULL);
                sigaction(SIGTERM, &action, NULL);
        }

        /* Loop until signal received */
        while (running) {
                int a;
                if (nanosleep(&delay, NULL) < 0)
                        break;
                a = activity(vmstat);
                if (a < 0)
                        break;
                led(a);
        }

        /* Ensure the LED is off */
        led(LOW);

        status = EXIT_SUCCESS;

out:
        if (line) free(line);
        if (vmstat) fclose(vmstat);
        return status;
}

