#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>

#include "log.h"
#include "xutil.h"
#include "qmp.h"

/* create a new connection each time we talk to qemu */
#define HAS_NEW_CONN    (1 << 1)
#define HAS_PATH        (1 << 2)

uint32_t flags = 0x0;

static void
help(void)
{
        dprintf("v -- VCPUs\n");
        dprintf("r -- Registers\n");
}

static void
print_help(void)
{
        dprintf("qemu-qmp [-c] -p /path/to/qmp-sock\n");
        dprintf("\t-c -- create a new connection\n");
        dprintf("\t-p -- path to UNIX socket\n");
        exit(EXIT_FAILURE);
}

static void
process_command(int act, const struct qmp_conn *qmpc)
{

        switch (act) {
        case 'r':
                if (qmp_show_regs(qmpc) == -1)
                        dprintf("Failed to get registers\n");
        break;
        case 'v':
                if (qmp_show_vcpus(qmpc) == -1)
                        dprintf("Failed to get cpus\n");
        break;
        case 'h':
                help();
        break;
        }
}

static int
qemu_qmp_conn(struct qmp_conn *qmpc)
{
        int r;

        /* establish connection */
        r = qmp_establish_conn(qmpc);
        if (r == -1) {
                dprintf("Unable to talk to qemu monitor\n");
                goto err_exit;
        }

        /* perform qmp negotiate in order to send commands */
        r = qmp_negotiate(qmpc);
        if (r == -1) {
                dprintf("Failed to negotiate qmp commands\n");
                goto err_exit;
        }

        return 0;

err_exit:
        return -1;
}

int main(int argc, char *argv[])
{
        struct qmp_conn qmpc;
        int act, c;
        struct stat st;

        while ((c = getopt(argc, argv, "hcp:")) != -1) {
                switch (c) {
                case 'c':
                        flags |= HAS_NEW_CONN;
                break;
                case 'p':
                        flags |= HAS_PATH;
                        qmpc.qmp_sock_path = strdup(optarg);
                break;
                case 'h':
                default:
                        print_help();
                }
        }

        if (!(flags & HAS_PATH)) {
                print_help();
        }

        /* check if the path really exists and is a UNIX sock */
        if (stat(qmpc.qmp_sock_path, &st) == -1) {
                xfree(qmpc.qmp_sock_path);
                FATAL("Failed stat on '%s'\n", qmpc.qmp_sock_path);
        }

        if ((st.st_mode & S_IFMT) != S_IFSOCK) {
                xfree(qmpc.qmp_sock_path);
                FATAL("'%s' not a socket file\n", qmpc.qmp_sock_path);
        }

        if (!(flags & HAS_NEW_CONN)) {
                if (qemu_qmp_conn(&qmpc) == -1) {
                        xfree(qmpc.qmp_sock_path);
                        exit(EXIT_FAILURE);
                }
                dprintf("Established connection over '%s'\n", 
                                qmpc.qmp_sock_path);
        }

        /* now, we can talk to qemu, wait for keyboard inputs */
        while ((act = getchar()) != 'q') {
                if (flags & HAS_NEW_CONN) {
                        if (qemu_qmp_conn(&qmpc) == -1)
                                exit(EXIT_FAILURE);

                        process_command(act, &qmpc);

                        qmp_close_conn(&qmpc);
                } else {
                        /* we already have a connection */
                        process_command(act, &qmpc);
                }
        }

        if (!(flags & HAS_NEW_CONN)) {
                qmp_close_conn(&qmpc);
        }

        xfree(qmpc.qmp_sock_path);

        return 0;
}
