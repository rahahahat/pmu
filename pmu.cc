#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "pmu.h"

std::vector<std::string> split(std::string s, std::string delimiter) {
  size_t pos_start = 0, pos_end, delim_len = delimiter.length();
  std::string token;
  std::vector<std::string> res;

  while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
    token = s.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + delim_len;
    res.push_back(token);
  }

  res.push_back(s.substr(pos_start));
  return res;
}

enum hex_values getHexValue(const char *ctr) {
  CMP(CPU_CYCLES, ctr, 11)
  CMP(L1D_CACHE, ctr, 10)
  CMP(INST_RETIRED, ctr, 13)
  CMP(BR_MIS_PRED, ctr, 12)
  CMP(L2D_CACHE, ctr, 10)
  CMP(STALL_FRONTEND, ctr, 15)
  CMP(STALL_BACKEND, ctr, 14)
  CMP(LD_SPEC, ctr, 8)
  CMP(LD_COMP_WAIT_L2_MISS, ctr, 21)
  CMP(LD_COMP_WAIT_L2_MISS_EX, ctr, 24)
  CMP(LD_COMP_WAIT_L1_MISS, ctr, 21)
  CMP(LD_COMP_WAIT_L1_MISS_EX, ctr, 24)
  CMP(LD_COMP_WAIT, ctr, 13)
  CMP(LD_COMP_WAIT_EX, ctr, 16)
  CMP(L1_MISS_WAIT, ctr, 13)
  CMP(L1_PIPE0_VAL, ctr, 13)
  CMP(L1_PIPE1_VAL, ctr, 13)
  CMP(L1_PIPE0_COMP, ctr, 14)
  CMP(L1_PIPE1_COMP, ctr, 14)
  CMP(L2_MISS_WAIT, ctr, 13)
  CMP(L2_MISS_COUNT, ctr, 14)
  CMP(L2_PIPE_VAL, ctr, 12)
  CMP(L2_PIPE_COMP_ALL, ctr, 17)
  return NONE;
}

char *getHexStr(enum hex_values val) {
  switch (val) {
    STRCASE(CPU_CYCLES)
    STRCASE(L1D_CACHE)
    STRCASE(INST_RETIRED)
    STRCASE(BR_MIS_PRED)
    STRCASE(L2D_CACHE)
    STRCASE(STALL_FRONTEND)
    STRCASE(STALL_BACKEND)
    STRCASE(LD_SPEC)
    STRCASE(LD_COMP_WAIT_L2_MISS)
    STRCASE(LD_COMP_WAIT_L2_MISS_EX)
    STRCASE(LD_COMP_WAIT_L1_MISS)
    STRCASE(LD_COMP_WAIT_L1_MISS_EX)
    STRCASE(LD_COMP_WAIT)
    STRCASE(LD_COMP_WAIT_EX)
    STRCASE(L1_MISS_WAIT)
    STRCASE(L1_PIPE0_VAL)
    STRCASE(L1_PIPE1_VAL)
    STRCASE(L1_PIPE0_COMP)
    STRCASE(L1_PIPE1_COMP)
    STRCASE(L2_MISS_WAIT)
    STRCASE(L2_MISS_COUNT)
    STRCASE(L2_PIPE_VAL)
    STRCASE(L2_PIPE_COMP_ALL)
  default:
    return "NONE";
  }
}

static uint64_t perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                                int cpu, int group_fd, unsigned long flags) {
  uint64_t ret;

  ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);

  return ret;
}

uint64_t get_perf_event_id(int fd) {
  uint64_t ret;

  ioctl(fd, PERF_EVENT_IOC_ID, &ret);
  return ret;
}

void reset_perf_event(int fd, uint8_t grouped) {
  if (grouped) {
    ioctl(fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
  } else {
    ioctl(fd, PERF_EVENT_IOC_RESET);
  }
}

static void enable_perf_event(int fd, uint8_t grouped) {
  if (grouped) {
    ioctl(fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
  } else {
    ioctl(fd, PERF_EVENT_IOC_ENABLE);
  }
}

static void disable_perf_event(int fd, uint8_t grouped) {
  if (grouped) {
    ioctl(fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
  } else {
    ioctl(fd, PERF_EVENT_IOC_DISABLE);
  }
}

uint64_t read_perf_event(int fd, uint64_t id, uint64_t num_events) {
  char buf[8 + (num_events * 16)];
  struct read_format *rf = (struct read_format *)buf;

  read(fd, buf, sizeof(buf));
  for (int i = 0; i < rf->nr; i++) {
    if (rf->values[i].id == id) {
      return rf->values[i].value;
    }
  }
  return 0;
}

uint64_t create_perf_event(uint32_t config, int group_fd) {
  struct perf_event_attr pe;
  uint64_t fd;
  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = PERF_TYPE_RAW;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = config;
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;
  pe.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;

  fd = perf_event_open(&pe, 0, -1, group_fd, 0);
  if (fd == -1) {
    fprintf(stderr, "Error opening performance event %x\n", config);
    exit(EXIT_FAILURE);
  }
  return fd;
}

struct perf_args *parseHexArguments(int argc, char **argv) {
  struct perf_args *args_ = (perf_args *)malloc(sizeof(struct perf_args));
  size_t arg_count = argc - 1;
  std::vector<uint64_t> ctrsvec;
  std::vector<std::string> labels;
  for (size_t x = 1; x < argc; x++) {
    std::string s(argv[x]);
    if (s.find("--counters") != std::string::npos) {
      auto vec = split(s, "=");
      auto ctr_vec = split(vec[1], ",");
      for (auto itr = ctr_vec.begin(); itr != ctr_vec.end(); itr++) {
        uint64_t hex = getHexValue((*itr).c_str());
        ctrsvec.push_back(hex);
        labels.push_back((*itr));
      }
    }
    if (s.find("--runs") != std::string::npos) {
      auto vec = split(s, "=");
      args_->runs = std::stoull(vec[1]);
    }
  }
  size_t ctr_count = ctrsvec.size();
  args_->counter_count = ctr_count;
  if (ctr_count > 2 && ctr_count > 0) {
    printf("Invaid counter count. Please supply atleast 1 or atmost 2 "
           "counters. \n");
    exit(1);
  }
  uint64_t *arg_array = (uint64_t *)malloc(ctr_count * sizeof(uint64_t));
  for (size_t x = 0; x < ctr_count; x++) {
    arg_array[x] = ctrsvec[x];
  }
  args_->hex_vals = arg_array;
  return args_;
}

void start_pmu_events(int argc, char **argv, struct perf_args *args_) {
  args_->ids = (uint64_t *)malloc(args_->counter_count * sizeof(uint64_t));
  args_->vals = (uint64_t *)malloc(args_->counter_count * sizeof(uint64_t));
  args_->fds = (int *)malloc(args_->counter_count * sizeof(int));
  args_->group_fd = -1;
  args_->fds[0] = -1;
  std::vector<int> ids;
  ids.resize(args_->counter_count);
  assert(args_->fds[0] == -1 && args_->group_fd == -1);
  for (size_t x = 0; x < args_->counter_count; x++) {
    args_->vals[x] = 0;
    args_->fds[x] = create_perf_event(args_->hex_vals[x], args_->fds[0]);
    args_->ids[x] = get_perf_event_id(args_->fds[x]);
  }
  args_->group_fd = args_->fds[0];
  assert(args_->group_fd != -1 && args_->fds[0] != -1);
  reset_perf_event(args_->group_fd, 1);
  enable_perf_event(args_->group_fd, 1);
};

void stop_perf_events(struct perf_args *args) {
  disable_perf_event(args->group_fd, 1);
};

void read_perf_events(struct perf_args *args) {
  for (size_t x = 0; x < args->counter_count; x++) {
    args->vals[x] +=
        read_perf_event(args->group_fd, args->ids[x], args->counter_count);
  }
};

void free_perf_args(struct perf_args *args) {
  // Close all file descriptors.
  for (size_t x = 0; x < args->counter_count; x++) {
    close(args->fds[x]);
  }
  free(args->fds);
  free(args->vals);
  free(args->ids);
  free(args->hex_vals);
  free(args);
}

void print_counters(struct perf_args *args) {
  for (size_t x = 0; x < args->counter_count; x++) {
    std::string s = "";
    std::cout << getHexStr((hex_values)args->hex_vals[x]) << ": "
              << (uint64_t)(args->vals[x] / args->runs) << std::endl;
  }
};

uint64_t get_runs(int argc, char **argv) {
  for (size_t x = 1; x < argc; x++) {
    std::string s(argv[x]);
    if (s.find("--runs") != std::string::npos) {
      auto vec = split(s, "=");
      return std::stoull(vec[1]);
    }
  }
  return 0;
}