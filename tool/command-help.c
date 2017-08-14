#include "command.h"
#include "interface.h"
#include "object.h"
#include "terminal-colors.h"
#include "util.h"

#include <errno.h>
#include <getopt.h>
#include <string.h>

static long help_interface(Cli *cli, const char *name) {
        _cleanup_(varlink_object_unrefp) VarlinkObject *parameters = NULL;
        _cleanup_(varlink_object_unrefp) VarlinkObject *reply = NULL;
        _cleanup_(freep) char *error = NULL;
        _cleanup_(varlink_interface_freep) VarlinkInterface *interface = NULL;
        const char *interfacestring = NULL;
        _cleanup_(freep) char *string = NULL;
        long r;

        varlink_object_new(&parameters);
        varlink_object_set_string(parameters, "interface", name);
        r = cli_call(cli, "org.varlink.service.GetInterfaceDescription", parameters, 0);
        if (r < 0)
                return r;

        r = cli_wait_reply(cli, &reply, &error, NULL);
        if (r < 0)
                return r;

        if (error) {
                printf("Error: %s\n", error);

                return 0;
        }

        if (varlink_object_get_string(reply, "description", &interfacestring) < 0)
                return -CLI_ERROR_CALL_FAILED;

        r = varlink_interface_new(&interface, interfacestring, NULL);
        if (r < 0)
                return -CLI_ERROR_PANIC;

        r  = varlink_interface_write_interfacestring(interface,
                                                     &string,
                                                     0, 72 - 2,
                                                     terminal_color(TERMINAL_BLUE),
                                                     terminal_color(TERMINAL_NORMAL),
                                                     terminal_color(TERMINAL_MAGENTA),
                                                     terminal_color(TERMINAL_NORMAL),
                                                     terminal_color(TERMINAL_GREEN),
                                                     terminal_color(TERMINAL_NORMAL),
                                                     terminal_color(TERMINAL_CYAN),
                                                     terminal_color(TERMINAL_NORMAL));
        if (r < 0)
                return r;

        printf("%s\n", string);

        return 0;
}

static long help_run(Cli *cli, int argc, char **argv) {
        static const struct option options[] = {
                { "help",    no_argument,       NULL, 'h' },
                {}
        };
        _cleanup_(freep) char *address = NULL;
        const char *interface = NULL;
        int c;
        long r;

        while ((c = getopt_long(argc, argv, "a:h", options, NULL)) >= 0) {
                switch (c) {
                        case 'h':
                                printf("Usage: %s help [ADDRESS/]INTERFACE\n", program_invocation_short_name);
                                printf("\n");
                                printf("Prints information about INTERFACE.\n");
                                printf("\n");
                                printf("  -h, --help             display this help text and exit\n");
                                return EXIT_SUCCESS;

                        default:
                                fprintf(stderr, "Try '%s --help' for more information\n",
                                        program_invocation_short_name);
                                return EXIT_FAILURE;
                }
        }

        if (!argv[optind]) {
                fprintf(stderr, "Usage: %s help [ADDRESS/]INTERFACE\n", program_invocation_short_name);
                return EXIT_FAILURE;
        }

        cli_split_address(argv[optind], &address, &interface);

        if (!address) {
                r = cli_resolve(cli, interface, &address);
                if (r < 0) {
                        fprintf(stderr, "Error resolving interface %s\n", interface);
                        return CLI_ERROR_CANNOT_RESOLVE;
                }
        }

        r = cli_connect(cli, address);
        if (r < 0) {
                fprintf(stderr, "Error connecting to %s\n", address);
                return CLI_ERROR_CANNOT_CONNECT;
        }

        r = help_interface(cli, interface);
        if (r < 0)
                return EXIT_FAILURE;

        return EXIT_SUCCESS;
}

const CliCommand command_help = {
        .name = "help",
        .info = "Print interface description or service information",
        .run = help_run
};
