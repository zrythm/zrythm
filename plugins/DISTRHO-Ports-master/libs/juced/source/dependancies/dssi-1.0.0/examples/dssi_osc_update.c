/*
 *  This program is in the public domain
 *
 *  $Id: dssi_osc_update.c,v 1.6 2005/10/12 17:08:26 smbolton Exp $
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lo/lo.h>

static volatile int done = 0;

int update_handler(const char *path, const char *types, lo_arg **argv,
                   int argc, lo_message msg, void *user_data)
{
    int i;

    if (argc) {
        printf("%s:", path);
        for (i = 0; i < argc; ++i) {
            printf(" ");
            lo_arg_pp((lo_type)types[i], argv[i]);
        }
        printf("\n");
    } else {
        printf("%s\n", path);
    }
    done = 0;

    return 0;
}

void osc_error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    lo_server_thread st;
    lo_address a;
    char *host, *port, *path;
    char update_path[256], *tmp_url, my_url[256];

    if (argc != 2) {
	fprintf(stderr, "usage: %s <osc url>\n", argv[0]);
	return 1;
    }

    host = lo_url_get_hostname(argv[1]);
    port = lo_url_get_port(argv[1]);
    path = lo_url_get_path(argv[1]);
    a = lo_address_new(host, port);

    snprintf(update_path, 255, "%s/update", path);

    st = lo_server_thread_new(NULL, osc_error);
    tmp_url = lo_server_thread_get_url(st);
    snprintf(my_url, 255, "%s%s", tmp_url, (strlen(path) > 1 ? path + 1 : path));
    free(tmp_url);
    lo_server_thread_add_method(st, NULL, NULL, update_handler, NULL);
    lo_server_thread_start(st);

    printf("sending osc.udp://%s:%s%s \"%s\"\n", host, port, update_path, my_url);
    lo_send(a, update_path, "s", my_url);
    free(host);
    free(port);
    free(path);

    /* quit if we go 5 seconds without receiving any OSC messages */
    while (done < 5) {
	sleep(1);
	done++;
    }

    return 0;
}

/* vi:set ts=8 sts=4 sw=4: */
