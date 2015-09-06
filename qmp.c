#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <stdint.h>

#include "xutil.h"
#include "log.h"
#include "qmp.h"


/*
 * read over a non-block fd
 */
static int 
qmp_read(int fd, void *buf, size_t *len)
{
        struct pollfd pfd;
        size_t tread = 0, nread;
        int r;

        pfd.fd = fd;
        pfd.events = POLLIN;

        while ((r = poll(&pfd, 1, QMP_POLL_TIMEOUT)) != 0) {

                if (r == -1)
                        return -1;

                if (pfd.revents & POLLIN) {
                        /* read at max 1k at a time */
                        nread = xread(fd, buf, 1024);
                        tread += nread;
                        buf += nread;
                }
        }

        *len = tread;
        return 0;
}

int
qmp_establish_conn(struct qmp_conn *qmpc)
{
        int s;
        struct sockaddr_un saddr;
        size_t path_len;
        size_t nread;

        char buf[QMP_MAX_LENGTH];

        if (!(path_len = strlen(qmpc->qmp_sock_path))) {
                return -1;
        }

        if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
                return -1;
        }

        memset(&saddr, 0, sizeof(struct sockaddr_un));
        saddr.sun_family = AF_UNIX;

        strncpy(saddr.sun_path, qmpc->qmp_sock_path, path_len);
        saddr.sun_path[path_len] = '\0';

        qmpc->fd = s;

        /* connect to it */
        if (connect(qmpc->fd, (struct sockaddr *) &saddr, 
                        sizeof(struct sockaddr_un)) == -1) {

                dprintf("Failed to connect to '%s' ('%s')\n", 
                        qmpc->qmp_sock_path, strerror(errno));
                close(qmpc->fd);

                return -1;
        }

        /* set non-block and read seq */
        xsetnonblock(qmpc->fd);

        /* qmp would send a greeting message when connected */
        memset(buf, 0, QMP_MAX_LENGTH);

        if (qmp_read(qmpc->fd, buf, &nread) == -1) {
                return -1;
        }

        if (nread == 0 && strncasecmp(buf, QMP_GREETING, 7)) {
                dprintf("Failed to get QMP greeting message\n");
                return -1;
        }

        return 0; 
}

int
qmp_close_conn(const struct qmp_conn *qmpc)
{
        if (close(qmpc->fd) == -1) {
                return -1;
        }

        return 0;
}

int
qmp_negotiate(const struct qmp_conn *qmpc)
{
        size_t cmd_len, nwrite, nread;
        char buf[QMP_MAX_LENGTH];

        
        cmd_len = strlen(QMP_ENTER_COMMAND_MODE);
        nwrite = xwrite(qmpc->fd, QMP_ENTER_COMMAND_MODE, cmd_len);
        if (nwrite == 0) {
                goto err_exit;
        }

        memset(buf, 0, QMP_MAX_LENGTH);
        if (qmp_read(qmpc->fd, buf, &nread) == -1) {
                goto err_exit;
        }

        if (strncasecmp(buf, QMP_COMMAND_MODE_OK, nread)) {
                goto err_exit;
        }

        return 0;

err_exit:
        dprintf("Failed to enter in command mode\n");
        return -1;
}

/*
 * json looks like
 *
 * {"return": "RAX=ffffffff8101c9a0 RBX=ffffffff818e2880 
 *      RCX=ffffffff818550e0 RDX=0000000000000000\r\n
 */
static int
qmp_populate_reg(const char *buf, const char *reg_name, uint64_t *reg)
{
        char *start;
        start = strstr(buf, reg_name);

        if (!start) {
                return -1;
        }

        while (start && *start) {
                if (*start == '=') {
                        start++;
                        break;
                }
                start++;
        }

        if (sscanf(start, "%lx", reg) != 1) {
                return -1;
        }

        return 0;
}

static int
__qmp_get_x64_regs(char *buf, struct qregs *regs)
{
        if (qmp_populate_reg(buf, "RAX", &regs->rax) == -1) {
                dprintf("Failed to get register RAX\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "RBX", &regs->rbx) == -1) {
                dprintf("Failed to get register RBX\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "RCX", &regs->rcx) == -1) {
                dprintf("Failed to get register RCX\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "RDX", &regs->rdx) == -1) {
                dprintf("Failed to get register RDX\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "RSI", &regs->rsi) == -1) {
                dprintf("Failed to get register RSI\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "RDI", &regs->rdi) == -1) {
                dprintf("Failed to get register RDI\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "RBP", &regs->rbp) == -1) {
                dprintf("Failed to get register RBP\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "RSP", &regs->rsp) == -1) {
                dprintf("Failed to get register RSP\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "RIP", &regs->rip) == -1) {
                dprintf("Failed to get register RIP\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "R8", &regs->r8) == -1) {
                dprintf("Failed to get register R8\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "R9", &regs->r9) == -1) {
                dprintf("Failed to get register R9\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "R10", &regs->r10) == -1) {
                dprintf("Failed to get register R10\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "R11", &regs->r11) == -1) {
                dprintf("Failed to get register R11\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "R12", &regs->r12) == -1) {
                dprintf("Failed to get register R12\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "R13", &regs->r13) == -1) {
                dprintf("Failed to get register R13\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "R14", &regs->r14) == -1) {
                dprintf("Failed to get register R14\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "R15", &regs->r15) == -1) {
                dprintf("Failed to get register R15\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "CR0", &regs->cr0) == -1) {
                dprintf("Failed to get register CR0\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "CR2", &regs->cr2) == -1) {
                dprintf("Failed to get register CR2\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "CR3", &regs->cr3) == -1) {
                dprintf("Failed to get register CR3\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "CR4", &regs->cr4) == -1) {
                dprintf("Failed to get register CR4\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "ES", &regs->es) == -1) {
                dprintf("Failed to get register ES\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "CS", &regs->cs) == -1) {
                dprintf("Failed to get register CS\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "SS", &regs->ss) == -1) {
                dprintf("Failed to get register SS\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "SS", &regs->ds) == -1) {
                dprintf("Failed to get register DS\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "GS", &regs->gs) == -1) {
                dprintf("Failed to get register GS\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "FS", &regs->fs) == -1) {
                dprintf("Failed to get register FS\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "RFL", &regs->rflags) == -1) {
                dprintf("Failed to get register RFLAGS\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "EFER", &regs->efer) == -1) {
                dprintf("Failed to get register MSR_EFER\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "CPL", &regs->cpl) == -1) {
                dprintf("Failed to get register CPL\n");
                goto err_exit;
        }

        return 0;

err_exit:
        return -1;
}

static int
__qmp_get_x86_regs(char *buf, struct qregs *regs)
{
        if (qmp_populate_reg(buf, "EAX", &regs->rax) == -1) {
                dprintf("Failed to get register RAX\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "EBX", &regs->rbx) == -1) {
                dprintf("Failed to get register RBX\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "ECX", &regs->rcx) == -1) {
                dprintf("Failed to get register RCX\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "EDX", &regs->rdx) == -1) {
                dprintf("Failed to get register RDX\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "ESI", &regs->rsi) == -1) {
                dprintf("Failed to get register RSI\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "EDI", &regs->rdi) == -1) {
                dprintf("Failed to get register RDI\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "EBP", &regs->rbp) == -1) {
                dprintf("Failed to get register RBP\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "ESP", &regs->rsp) == -1) {
                dprintf("Failed to get register RSP\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "EIP", &regs->rip) == -1) {
                dprintf("Failed to get register RIP\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "CR0", &regs->cr0) == -1) {
                dprintf("Failed to get register CR0\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "CR2", &regs->cr2) == -1) {
                dprintf("Failed to get register CR2\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "CR3", &regs->cr3) == -1) {
                dprintf("Failed to get register CR3\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "CR4", &regs->cr4) == -1) {
                dprintf("Failed to get register CR4\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "ES", &regs->es) == -1) {
                dprintf("Failed to get register ES\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "CS", &regs->cs) == -1) {
                dprintf("Failed to get register CS\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "SS", &regs->ss) == -1) {
                dprintf("Failed to get register SS\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "SS", &regs->ds) == -1) {
                dprintf("Failed to get register DS\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "GS", &regs->gs) == -1) {
                dprintf("Failed to get register GS\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "FS", &regs->fs) == -1) {
                dprintf("Failed to get register FS\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "EFL", &regs->rflags) == -1) {
                dprintf("Failed to get register RFLAGS\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "EFER", &regs->efer) == -1) {
                dprintf("Failed to get register MSR_EFER\n");
                goto err_exit;
        }

        if (qmp_populate_reg(buf, "CPL", &regs->cpl) == -1) {
                dprintf("Failed to get register CPL\n");
                goto err_exit;
        }

        return 0;

err_exit:
        return -1;
}

static int
qmp_get_regs(char *buf, struct qregs *regs)
{
        if (strstr(buf, "RAX")) {
                if (__qmp_get_x64_regs(buf, regs) == -1)
                        return -1;
                regs->mode = X64;
        } else {
                if (__qmp_get_x86_regs(buf, regs) == -1)
                        return -1;
                regs->mode = X86;
        }

        return 0;
}

static void
qmp_dump_regs(const struct qregs *regs)
{
        if (regs->mode == X64) {
                dprintf("RAX=0x%.16lx, RBX=0x%.16lx, RCX=0x%.16lx, RDX=0x%.16lx\n",
                                regs->rax, regs->rbx, regs->rcx, regs->rdx);

                dprintf("RSI=0x%.16lx, RDI=0x%.16lx, RBP=0x%.16lx, RSP=0x%.16lx\n",
                                regs->rsi, regs->rdi, regs->rbp, regs->rsp);

                dprintf("RIP=0%.16lx, CPL=%.1lu\n", regs->rip, regs->cpl);

                dprintf("R8=0x%.16lx,  R9=0x%.16lx,  R10=0x%.16lx, R11=0x%.16lx\n",
                                regs->r8, regs->r9, regs->r10, regs->r11);

                dprintf("R12=0x%.16lx, R13=0x%.16lx, R14=0x%.16lx, R15=0x%.16lx\n",
                                regs->r12, regs->r13, regs->r14, regs->r15);

                dprintf("CR0=0x%.16lx, CR2=0x%.16lx, CR3=0x%.16lx, CR4=0x%.16lx\n",
                                regs->cr0, regs->cr2, regs->cr3, regs->cr4);
        } else {
                dprintf("EAX=0x%.8lx, EBX=0x%.8lx, ECX=0x%.8lx, EDX=0x%.8lx\n",
                                regs->rax, regs->rbx, regs->rcx, regs->rdx);

                dprintf("ESI=0x%.8lx, EDI=0x%.8lx, EBP=0x%.8lx, ESP=0x%.8lx\n",
                                regs->rsi, regs->rdi, regs->rbp, regs->rsp);

                dprintf("EIP=0%.8lx, CPL=%.1lu\n", regs->rip, regs->cpl);

                dprintf("CR0=0x%.8lx, CR2=0x%.8lx, CR3=0x%.8lx, CR4=0x%.8lx\n",
                                regs->cr0, regs->cr2, regs->cr3, regs->cr4);
        }
}

int
qmp_show_regs(const struct qmp_conn *qmpc)
{
        size_t nread, nwrite, cmd_len_regs;
        char *buf;
        struct qregs regs;

        cmd_len_regs = strlen(QMP_COMMAND_INFO_REGS);
        
        nwrite = xwrite(qmpc->fd, QMP_COMMAND_INFO_REGS, cmd_len_regs);
        if (nwrite != cmd_len_regs) {
                return -1;
        }

        buf = xmalloc(QMP_BUF_LEN);
        memset(buf, 0, QMP_BUF_LEN);

        if (qmp_read(qmpc->fd, buf, &nread) == -1 || nread <= 0) {
                return -1;
        }

        memset(&regs, 0, sizeof(struct qregs));

        if (qmp_get_regs(buf, &regs) == -1) {
                return -1;
        }

        qmp_dump_regs(&regs);

        xfree(buf);

        return 0;
}

static int
qmp_parse_cpu_line(const char *str, struct vcpu *vcpu)
{
        uint8_t id;
        uint64_t pc;
        char *state = NULL;
        int r;

        /* allocate using glibc %m */
        r = sscanf(str, " #%hhu: pc=0x%lx (%m[a-z]*", &id, &pc, &state);
        if (r < 2) {
                return -1;
        }
        
        vcpu->id = id;
        vcpu->pc = pc;

        if (state) {
                if (!strncasecmp(state, "halted", 6)) {
                        vcpu->state = HALTED;
                } else {
                        /* impropable, might get junk */
                        vcpu->state = UNDEFINED;
                }
        } else {
                /* if state is not present then the CPU is running...*/
                vcpu->state = RUNNING;
        }

        if (state) {
                xfree(state);
        }

        return 0;
}

/*
 * json looks like:
 *
 * {"return": "* CPU #0: pc=0xffffffff81051c02 (halted) thread_id=5132\r\n  
 *      CPU #1: pc=0xffffffff81051c02 (halted) thread_id=5133\r\n"}
 */
static int
qmp_get_vcpus(char *buf, struct vcpus *vcpus)
{
        char *r, *ptr;

        if (!(r = strstr(buf, "CPU"))) {
                return -1;
        }

        ptr = strtok(r, "CPU");
        while (ptr) {

                struct vcpu *vcpu = xmalloc(sizeof(struct vcpu));

                if (qmp_parse_cpu_line(ptr, vcpu) == -1) {
                        xfree(vcpu);
                        return -1;
                }

                /* next vcpu */
                vcpu->next = vcpus->vcpu;
                vcpus->vcpu = vcpu;
                vcpus->count++;

                ptr = strtok(NULL, "CPU");
        }

        return 0;
}

static void
qmp_dump_vcpus(const struct vcpus *vcpus)
{
        struct vcpu *cpu;

        for (cpu = vcpus->vcpu; cpu != NULL; cpu = cpu->next) {
                dprintf("CPU#%u, PC=0x%lx, ", cpu->id, cpu->pc);
                dprintf("State: ");
                switch (cpu->state) {
                case RUNNING:
                        dprintf("Running");
                break;
                case HALTED:
                        dprintf("Halted");
                break;
                case UNDEFINED:
                        dprintf("Undef");
                break;
                }
                dprintf("\n");
        }
}

static void
qmp_clean_vpcus(struct vcpus *vcpus)
{
        struct vcpu *v = vcpus->vcpu;

        while (v) {
                if (v->next == NULL) {
                        /* free the current */
                        xfree(v);
                        break;
                } else {
                        struct vcpu *vn = v->next;
                        xfree(v);
                        v = vn;
                }
        }
}

int
qmp_show_vcpus(const struct qmp_conn *qmpc)
{
        size_t nread, nwrite, cmd_len_regs;
        char *buf;
        struct vcpus vcpus;

        cmd_len_regs = strlen(QMP_COMMAND_INFO_CPU);
        
        nwrite = xwrite(qmpc->fd, QMP_COMMAND_INFO_CPU, cmd_len_regs);
        if (nwrite != cmd_len_regs) {
                return -1;
        }

        buf = xmalloc(QMP_BUF_LEN);
        memset(buf, 0, QMP_BUF_LEN);

        if (qmp_read(qmpc->fd, buf, &nread) == -1 || nread <= 0) {
                return -1;
        }

        memset(&vcpus, 0, sizeof(struct vcpus));

        if (qmp_get_vcpus(buf, &vcpus) == -1) {
                xfree(buf);
                return -1;
        }

        qmp_dump_vcpus(&vcpus);

        /* clean-up vcpus, as we used a linked list to store them */
        qmp_clean_vpcus(&vcpus);

        xfree(buf);

        return 0;
}
