#ifndef __QMP_H
#define __QMP_H

#define QMP_MAX_LENGTH          (256)
/* read at most 8k */
#define QMP_BUF_LEN             (8 * 1024)
/* timeout for polling */
#define QMP_POLL_TIMEOUT        (10)

#define QMP_GREETING            "{\"QMP\":"
#define QMP_ENTER_COMMAND_MODE  "{ \"execute\": \"qmp_capabilities\" }"
#define QMP_COMMAND_MODE_OK     "{\"return\": {}}\r\n"

#define QMP_COMMAND_INFO_REGS   "{\"execute\": \"human-monitor-command\", \"arguments\": {\"command-line\": \"info registers\"}}"
#define QMP_COMMAND_INFO_CPU    "{\"execute\": \"human-monitor-command\", \"arguments\": {\"command-line\": \"info cpus\"}}"

struct qmp_conn {
        int fd;
        char *qmp_sock_path;
};

enum vcpu_state {
        UNDEFINED,
        HALTED, 
        RUNNING
};

struct vcpu {
        uint8_t id;
        enum vcpu_state state;
        uint64_t pc;
        struct vcpu *next;
};

struct vcpus {
        struct vcpu *vcpu;
        uint32_t count;
};

enum qregs_arch {
        X86, X64
};

/*
 * for both x86 and x64
 */
struct qregs {
        uint64_t rax, rbx, rcx, rdx;
        uint64_t rsi, rdi, rbp, rsp;
        uint64_t r8, r9, r10, r11;
        uint64_t r12, r13, r14, r15;
        uint64_t rip, rflags, efer;

        /* these are unsigned shorts but it 
         * easier to have one parsing method */
        uint64_t es, cs, ss, ds, fs, gs, cpl;

        uint64_t cr0, cr2, cr3, cr4;

        enum qregs_arch mode;
};

extern int
qmp_establish_conn(struct qmp_conn *qmpc);

extern int
qmp_negotiate(const struct qmp_conn *qmpc);

extern int
qmp_close_conn(const struct qmp_conn *qmpc);

extern int
qmp_show_regs(const struct qmp_conn *qmpc);

extern int
qmp_show_vcpus(const struct qmp_conn *qmpc);

#endif /* __QMP_H */
