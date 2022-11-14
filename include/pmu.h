// https://man7.org/linux/man-pages/man2/perf_event_open.2.html
#ifndef _DD_
#define _DD_

#include <asm/unistd.h>
#include <inttypes.h>
#include <linux/perf_event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

static char *labels[23] = {"CPU_CYCLES",
                           "L1D_CACHE",
                           "INST_RETIRED",
                           "BR_MIS_PRED",
                           "L2D_CACHE",
                           "STALL_FRONTEND",
                           "STALL_BACKEND",
                           "LD_SPEC",
                           "LD_COMP_WAIT_L2_MISS",
                           "LD_COMP_WAIT_L2_MISS_EX",
                           "LD_COMP_WAIT_L1_MISS",
                           "LD_COMP_WAIT_L1_MISS_EX",
                           "LD_COMP_WAIT",
                           "LD_COMP_WAIT_EX",
                           "L1_MISS_WAIT",
                           "L1_PIPE0_VAL",
                           "L1_PIPE1_VAL",
                           "L1_PIPE0_COMP",
                           "L1_PIPE1_COMP",
                           "L2_MISS_WAIT",
                           "L2_MISS_COUNT",
                           "L2_PIPE_VAL",
                           "L2_PIPE_COMP_ALL"};

#define STR(X) #X

#define CMP(X, Y, LEN)                                                         \
  if (strncmp(STR(X), Y, LEN) == 0)                                            \
    return X;

#define STRCASE(X)                                                             \
  case X:                                                                      \
    return STR(X);

enum hex_values {
  NONE = 0x00,
  CPU_CYCLES = 0x0011,
  L1D_CACHE = 0x0004,
  INST_RETIRED = 0x0008,
  BR_MIS_PRED = 0x0010,
  L2D_CACHE = 0x0016,
  STALL_FRONTEND = 0x0023,
  STALL_BACKEND = 0x0024,
  LD_SPEC = 0x0070,
  LD_COMP_WAIT_L2_MISS = 0x0180,
  LD_COMP_WAIT_L2_MISS_EX = 0x0181,
  LD_COMP_WAIT_L1_MISS = 0x0182,
  LD_COMP_WAIT_L1_MISS_EX = 0x0183,
  LD_COMP_WAIT = 0x0184,
  LD_COMP_WAIT_EX = 0x0185,
  L1_MISS_WAIT = 0x0208,
  L1_PIPE0_VAL = 0x240,
  L1_PIPE1_VAL = 0x241,
  L1_PIPE0_COMP = 0x260,
  L1_PIPE1_COMP = 0x261,
  L2_MISS_WAIT = 0x308,
  L2_MISS_COUNT = 0x0309,
  L2_PIPE_VAL = 0x0330,
  L2_PIPE_COMP_ALL = 0x0350
};

enum hex_values getHexValue(const char *ctr);
char *getHexStr(enum hex_values val);

struct read_format {
  uint64_t nr;
  struct {
    uint64_t value;
    uint64_t id;
  } values[];
};

struct perf_args {
  uint64_t *hex_vals;
  size_t counter_count;
  uint64_t *ids;
  int group_fd;
  int *fds;
  size_t runs;
  uint64_t *vals;
};

#ifdef __cplusplus
extern "C" {
#endif

static uint64_t perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                                int cpu, int group_fd, unsigned long flags);
uint64_t get_perf_event_id(int fd);
void reset_perf_event(int fd, uint8_t grouped);
static void enable_perf_event(int fd, uint8_t grouped);
static void disable_perf_event(int fd, uint8_t grouped);
uint64_t read_perf_event(int fd, uint64_t id, uint64_t num_events);

uint64_t create_perf_event(uint32_t config, int group_fd);

struct perf_args *parseHexArguments(int argc, char **argv);
void start_pmu_events(int argc, char **argv, struct perf_args *args_);
void stop_perf_events(struct perf_args *args);
void read_perf_events(struct perf_args *args);
void free_perf_args(struct perf_args *args);
void print_counters(struct perf_args *args);
uint64_t get_runs(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#define RUN_PERF(...)                                                          \
  struct perf_args *args = parseHexArguments(argc, argv);                      \
  for (size_t r = 0; r < args->runs; r++) {                                    \
    start_pmu_events(argc, argv, args);                                        \
    {__VA_ARGS__};                                                             \
    stop_perf_events(args);                                                    \
    read_perf_events(args);                                                    \
  }                                                                            \
  print_counters(args);                                                        \
  free_perf_args(args);

#endif