#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#include <sys/stat.h>

/* non-block */
#include <fcntl.h>

/* keep-alive and no delay */
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include "log.h"
#include "xutil.h"

void *
xmalloc(unsigned int size)
{
        void *ptr = malloc(size);

        if (size && !ptr) {
                FATAL("malloc(@ 0x%p\n", __builtin_return_address(0));
        }
        memset(ptr, 0, size);
        return ptr;
}

void *
xcalloc(unsigned int numb, unsigned int size)
{
        void *ptr = calloc(numb, size);

        if (numb && size && !ptr) {
                FATAL("calloc(@ 0x%p)\n", __builtin_return_address(0));
        }
        return ptr;
}

void *
xrealloc(void *ptr, unsigned int size)
{
        void *nptr; 

        nptr = realloc(ptr, size);

        if (!nptr) {
                FATAL("realloc(@ 0x%p)\n", __builtin_return_address(0));
        }
        return nptr;
}

void 
xfree(void *ptr)
{
        if (ptr) {
                free(ptr);
        }
        ptr = NULL;
}


size_t 
xread(int fd, void *buf, size_t size)
{
        ssize_t total = 0;

        while (size) {
                ssize_t ret;
                ret = read(fd, buf, size);

                if (ret < 0 && errno == EINTR)
                        continue;

                /* no more to read */
                if (ret <= 0)
                        break;

                buf = (char *) buf + ret;
                size -= ret;
                total += ret;
        }

        return total;
}

size_t 
xwrite(int fd, const char *buf, size_t size)
{
        ssize_t total = 0;

        while (size) {
                ssize_t ret;
                ret = write(fd, buf, size);

                if (ret < 0 && errno == EINTR)
                        continue;

                /* no more to write */
                if (ret <= 0)
                        break;

                buf = (const char *) buf + ret;
                size -= ret;
                total += ret;
        }

        return total;
}

char *
xstrcpy(char *dst, const char *src)
{
        char *ret;
        ret = dst;

        while ((*dst++ = *src++) != '\0')
                /* do nothing */;
        return ret;
}

size_t
xstrlcpy(char *dst, const char *src, size_t size)
{
        char *dst_in;

        dst_in = dst;
        if (size > 0) {
                while (--size > 0 && *src != '\0')
                        *dst++ = *src++;
                *dst = '\0';
        }
        return dst - dst_in;
}

void
xskipwhitespace(const char *str)
{
        while (*str == ' ' || *str == '\t')
                str++;
}

char *
xstrdup(const char *s)
{
        char *d;

        if (s) {
                d = strdup(s);
        } else {
                d = strdup("");
        }

        if (d == NULL) {
                FATAL("strdup failed @ %p\n", __builtin_return_address(0));
        }
        return d;
}


void
daemonize(void)
{
        pid_t pid, sid;
        FILE *st = NULL;

        /* if we're already daemonized */
        if (getppid() == 1) {
                return;
        }

        pid = fork();
        switch (pid) {
        case -1:
                FATAL("fork()\n");
                break;
        case 0: /* child */
                break;
        default: /* parent, exists */
                exit(EXIT_SUCCESS);
        }

        dprintf("Running as daemon...\n");

        if ((sid = setsid()) == EPERM) {
                FATAL("setsid()\n");
        }

        if (chdir("/") == -1) {
                FATAL("chdir()\n");
        }

        st = freopen("/dev/null", "r", stdin);
        st = freopen("/dev/null", "w", stdout);
        st = freopen("/dev/null", "w", stderr);

        if (st == NULL) {
                FATAL("freopen()\n");
        }
}


unsigned long int
xstroul(const char *str, char **end, int base)
{
        unsigned long int val = 0UL;
        int neg;

        xskipwhitespace(str);

        if (*str == '+') {
                str++;
        } else if (*str == '-') {
                neg = 1; str++;
        }

        if ((base == 0 || base == 16) &&
            (str[0] == '0' && str[1] == 'x')) {
                str += 2; 
                base = 16;
        } else if (base == 0 && str[0] == '0') {
                str++;
                base = 8;
        } else if (base == 0) {
                base = 10;
        }


        while (str && *str) {
                int digit;

                if (*str >= '0' && *str <= '9') {
                        digit = *str - '0';
                } else if (*str >= 'a' && *str <= 'z') {
                        digit = *str - 'a' + 10;
                } else if (*str >= 'A' && *str <= 'Z') {
                        digit = *str - 'A' + 10;
                } else {
                        break;
                }

                if (digit >= base) {
                        break;
                }

                val = (val * base) + digit;
                str++;
        }


        if (end) {
                *end = (char *) str;
        }

        return (neg ? -val : val);
}

size_t
to_bytes(unsigned char *d, const char *str, int base_from)
{
        size_t sn;

        for (sn = 0; *str; str++) {
                unsigned int digit;

                if (*str >= '0' && *str <= '9') {
                        digit = *str - '0';
                } else if (*str >= 'a' && *str <= 'z') {
                        digit = *str - 'a' + 10;
                } else if (*str >= 'A' && *str <= 'Z') {
                        digit = *str - 'A' + 10;
                } else {
                        digit = base_from; /* fail */
                }

                if (digit >= (unsigned) base_from) {
                        free(d); 
                        return 0;
                }
                d[sn++] = digit;
        }

        return sn;
}

off_t
get_file_len(const char *fn)
{
        struct stat st;

        if (stat(fn, &st) == -1) {
                return -1;
        }
        return st.st_size;
}

int
file_read(const char *fn, char **dst, size_t *len)
{
        off_t flen; int fd;
        struct stat st;

        /* open file */
        if ((fd = open(fn, O_RDONLY)) == -1) {
                dprintf("open()\n");
                goto fail;
        }

        /* det size */
        if (fstat(fd, &st) == -1) {
                dprintf("stat()\n");
                close(fd);
                goto fail;
        }

        flen = st.st_size;
        if (flen <= 0) {
                dprintf("Invalid file length\n");
                close(fd);
                goto fail;
        }

        if (*dst == NULL)
                *dst = xmalloc(flen);

        if (xread(fd, *dst, flen) != (size_t) flen) {
                xfree(*dst);
                close(fd);
                goto fail;
        }

        *len = flen;
        close(fd);
        return 0;
fail:
        return -1;
}

void
xsetnonblock(int fd)
{
        fcntl(fd, F_SETFL, O_NONBLOCK | fcntl(fd, F_GETFL));
}

int 
xset_tcp_keepalive(int fd)
{
        int yes = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1) {
                dprintf("setsockopt SO_KEEPALIVE\n");
                return -1;
        }
        return 0;
}

int
xset_tcp_reuseaddr(int fd)
{
        int yes = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &yes, sizeof(yes)) == -1) {
                dprintf("setsockopt SO_REUSEADDR()\n");
                return -1;
        }
        return 0;
}

int
xset_tcp_nodelay(int fd, int val)
{
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1) {
                FATAL("setsockopt TCP_NODELAY\n");
                return -1;
        }
        return 0;
}

int 
xenable_tcp_nodelay(int fd)
{
        return xset_tcp_nodelay(fd, 1);
}

int 
xdisable_tcp_nodelay(int fd)
{
        return xset_tcp_nodelay(fd, 0);
}
