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

#ifdef __cplusplus
extern "C" {
#endif

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

enum hex_values getHexValue(char *ctr);
char *getHexStr(enum hex_values val);

struct read_format {
  uint64_t nr;
  struct {
    uint64_t value;
    uint64_t id;
  } values[];
};

struct counters {
  uint64_t *hex_val;
  char **labels;
  uint64_t count;
};

struct perf_l1_access_id {
  uint64_t g_fd;
  uint64_t miss_id;
  uint64_t hit_id;
  uint64_t miss_count_aggr;
  uint64_t hit_count_aggr;
};

static uint64_t perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                                int cpu, int group_fd, unsigned long flags);
uint64_t get_perf_event_id(int fd);
void reset_perf_event(int fd, uint8_t grouped);
static void enable_perf_event(int fd, uint8_t grouped);
static void disable_perf_event(int fd, uint8_t grouped);
uint64_t read_perf_event(int fd, uint64_t id, uint64_t num_events);
uint64_t *parseHexArguments(int arg_count, int start_idx, char **argv);
uint64_t create_perf_event(uint32_t config, int group_fd);
void read_perf_hw_cache_events(struct perf_l1_access_id *grp);
void create_perf_hw_cache_events(struct perf_l1_access_id *grp);

#define RUN_PERF(RUNS, CTR_NUM, START_IDX, ...)                                \
  uint64_t argct = CTR_NUM;                                                    \
  uint64_t *counters = parseHexArguments(START_IDX, argv);                     \
  uint64_t *ctr_avrg = (uint64_t *)malloc(argct * sizeof(uint64_t));           \
  for (size_t x = 0; x < RUNS; x++) {                                          \
    int fds[4] = {-1, 0, 0, 0};                                                \
    uint64_t ids[4] = {0, 0, 0, 0};                                            \
    for (size_t x = 0; x < argct; x++) {                                       \
      fds[x] = create_perf_event(counters[x], fds[0]);                         \
      ids[x] = get_perf_event_id(fds[x]);                                      \
    }                                                                          \
    reset_perf_event(fds[0], 1);                                               \
    enable_perf_event(fds[0], 1);                                              \
    {__VA_ARGS__};                                                             \
    disable_perf_event(fds[0], 1);                                             \
    for (size_t x = 0; x < argct; x++) {                                       \
      uint64_t count = read_perf_event(fds[0], ids[x], argct);                 \
      ctr_avrg[x] += count;                                                    \
    }                                                                          \
  }                                                                            \
  for (size_t x = 0; x < argct; x++) {                                         \
    uint64_t avg = ctr_avrg[x] / runs;                                         \
    char buff[100];                                                            \
    snprintf(buff, 100, "%s (0x%lx) : %ld\n", getHexStr(counters[x]),          \
             counters[x], avg);                                                \
    printf("%s", buff);                                                        \
  }                                                                            \
  free(counters);

#define RUN_PERF_CACHE(RUNS, ...)                                              \
  struct perf_l1_access_id *perf_grp =                                         \
      malloc(sizeof(struct perf_l1_access_id));                                \
  for (size_t x = 0; x < RUNS; x++) {                                          \
    create_perf_hw_cache_events(perf_grp);                                     \
    perf_grp->miss_count_aggr = 0;                                             \
    perf_grp->hit_count_aggr = 0;                                              \
    reset_perf_event(perf_grp->g_fd, 1);                                       \
    enable_perf_event(perf_grp->g_fd, 1);                                      \
    {__VA_ARGS__};                                                             \
    disable_perf_event(perf_grp->g_fd, 1);                                     \
  }                                                                            \
  printf("L1 Miss: %ld\n", perf_grp->miss_count_aggr / RUNS);                  \
  printf("L1 Accesses: %ld\n", perf_grp->hit_count_aggr / RUNS);               \
  free(perf_grp);

#ifdef __cplusplus
}
#endif

#endif