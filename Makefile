V = @

ifeq ($(V),1)
override V = 
endif
ifeq ($(V),0)
override V = @
endif

CC = gcc
CFLAGS = -Wall -Werror -Wextra -Wstrict-prototypes -Wmissing-prototypes
CFLAGS += -D_FORTIFY_SOURCE=2 -fno-strict-aliasing -O2 -march=native

# tools
FIND = find
INSTALL = install
GZIP = gzip
RM = rm
CTAGS = ctags
CSCOPE = cscope

OBJDIR = obj
DEBUG_FLAGS = -DDEBUG -DDEBUG_HEADERS

ifeq ($(DEBUG), 1)
WITH_DEBUG = -g3 -O0 $(DEBUG_FLAGS)
endif

ifeq ($(SANITIZE), 1)
WITH_DEBUG = -g3 -O0 -fsanitize=address $(DEBUG_FLAGS)
endif

ifeq ($(UNDEFINED), 1)
WITH_DEBUG = -g3 -O0 -fsanitize=undefined -fsanitize=shift \
	     -fsanitize=unreachable -fsanitize=integer-divide-by-zero\
	     -fsanitize=vla-bound -fsanitize=null -fsanitize=return\
	     -fsanitize=signed-integer-overflow $(DEBUG_FLAGS)
endif

LIBS = 

INCLUDE = -Iinclude -I.

#override CFLAGS += -D_REENTRANT

QEMU_QMP_SRC = xutil.c qmp.c main.c
QEMU_QMP_O = $(patsubst %.c,%.o,$(QEMU_QMP_SRC))

TARGETS = qemu-qmp

all: $(TARGETS)

%.o: %.c
	@mkdir -p $(@D)
	@echo CC $<
	$(V)$(CC) $(CFLAGS) $(LIBS) $(INCLUDE) $(WITH_DEBUG) -c -o $@ $<

qemu-qmp: $(QEMU_QMP_O)
	@echo LD $@
	$(V)$(CC) $(CFLAGS) $(LIBS) $(WITH_DEBUG) -o $@ $^


clean:
	@rm -rf core $(QEMU_QMP_O) $(TARGETS)

distclean: clean
	@rm -rf tags tags-sys $(CSCOPE_FILES) $(CSCOPE_SYS_FILES)

mtags:
	$(V)$(RM) -rf $(CSCOPE_FILES) tags
	$(V)$(FIND) . -name \*.[chS] > cscope.files
	$(V)$(CTAGS) -R -L cscope.files

tags: mtags

tags-sys: 
	$(V)$(RM) -rf $(CSCOPE_SYS_FILES) tags-sys
	$(V)$(FIND) /usr/include -name \*.[chS] > cscope-sys.files
	$(V)$(CTAGS) -R -L cscope-sys.files -f tags-sys

cscope: 
	$(V)$(RM) -rf $(CSCOPE_FILES)
	$(V)$(FIND) . -name \*.[chS] > cscope.files
	$(V)$(CSCOPE) -b -q

cscope-sys: cscope-sys.files
	$(V)$(CSCOPE) -b -q -i cscope-sys.files -f cscope-sys.out
