#ifndef __XUTIL_H
#define __XUTIL_H

/**
 * @brief
 */
#define streq(s1, s2) (!strcmp((s1), (s2)))

/**
 * @brief
 */
#define strcaseeq(s1, s2) (!strcasecmp((s1), (s2)))

/**
 * @brief wrapper over malloc, tests if 
 * @param size
 * @retval a pointer to the newly allocated area
 */
extern void *
xmalloc(unsigned int size);

/**
 * @brief wrapper over calloc
 * @param numb 
 * @param size
 * @@retval
 */
extern void *
xcalloc(unsigned int numb, unsigned int size);

/**
 * @brief wrapper over realloc
 * @param old
 * @param size
 */
extern void *
xrealloc(void *old, unsigned int size);

/**
 * @brief wrapper over free
 * @param ptr
 *
 */
extern void 
xfree(void *ptr);

/**
 * @wrapper over read, read from fd 'fd' into buffer 'buf' in the amount of size
 * 'size'
 * @param fd the file descriptior
 * @param buf a pointer to a pre-allocated buffer
 * @param size the amount of bytes to read
 * @retval the amount, in bytes, read
 */
extern size_t 
xread(int fd, void *buf, size_t size);

/**
 * @brief wrapper over write
 * @param fd
 * @param buf
 * @param size
 * @retval
 */
extern size_t 
xwrite(int fd, const char *buf, size_t size);

/**
 * @brief
 */
extern void 
xskipwhitespace(const char *str);

/**
 * @brief
 */
extern char *
xstrdup(const char *s);
/**
 * @brief
 */
extern char *
xstrcpy(char *dst, const char *str);

/**
 * @brief
 */
extern size_t 
xstrlcpy(char *dst, const char *str, size_t len);

/**
 * @brief
 */
extern unsigned long int
xstroul(const char *str, char **end, int base);


/**
 * @brief
 */
extern size_t
to_bytes(unsigned char *d, const char *str, int base_from);

/**
 * @brief
 */
extern off_t
get_file_len(const char *fn);

/**
 * @brief
 */
extern int
file_read(const char *fn, char **dst, size_t *flen);

/**
 * @brief
 */
extern void
daemonize(void);

/**
 * @brief
 */
extern void
xsetnonblock(int fd);

/**
 * @brief
 */
extern int
xset_tcp_reuseaddr(int fd);

/**
 * @brief
 */
extern int 
xset_tcp_keepalive(int fd);

/**
 * @brief
 */
extern int
xset_tcp_nodelay(int fd, int val);

/**
 * @brief
 */
extern int
xenable_tcp_nodelay(int fd);

/**
 * @brief
 */
extern int
xdisable_tcp_nodelay(int fd);

#endif /* __XUTIL_H */
