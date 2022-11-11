#include "pmu.h"

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

uint64_t *parseHexArguments(int arg_count, int start_idx, char **argv) {
  if (arg_count > 2 && arg_count > 0) {
    printf("Invaid counter count. Please supply atleast 1 or atmost 2 "
           "counters. \n");
    exit(1);
  }
  uint64_t *arg_array = (uint64_t *)malloc(arg_count * sizeof(uint64_t));
  for (size_t x = 0; x < arg_count; x++) {
    size_t hex_idx = x + start_idx;
    uint64_t hex = (uint64_t)getHexValue(argv[hex_idx]);
    arg_array[x] = hex;
  }
  return arg_array;
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

void read_perf_hw_cache_events(struct perf_l1_access_id *grp) {
  grp->hit_count_aggr += read_perf_event(grp->g_fd, grp->hit_id, 2);
  grp->miss_count_aggr += read_perf_event(grp->g_fd, grp->miss_id, 2);
};

perf_l1_access_id *create_perf_hw_cache_events() {

  struct perf_event_attr pe;
  struct perf_l1_access_id *out =
      (perf_l1_access_id *)malloc(sizeof(perf_l1_access_id));

  uint64_t fd;
  memset(&pe, 0, sizeof(struct perf_event_attr));

  pe.type = PERF_TYPE_HW_CACHE;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = PERF_COUNT_HW_CACHE_L1D | PERF_COUNT_HW_CACHE_OP_READ << 8 |
              PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16;
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;
  pe.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;

  fd = perf_event_open(&pe, 0, -1, -1, 0);
  if (fd == -1) {
    fprintf(stderr, "Error opening performance event hit.\n");
    exit(EXIT_FAILURE);
  }

  struct perf_event_attr pe_miss;
  uint64_t fd_miss;
  memset(&pe_miss, 0, sizeof(struct perf_event_attr));
  pe_miss.type = PERF_TYPE_HW_CACHE;
  pe_miss.size = sizeof(struct perf_event_attr);
  pe_miss.config = PERF_COUNT_HW_CACHE_L1D | PERF_COUNT_HW_CACHE_OP_READ << 8 |
                   PERF_COUNT_HW_CACHE_RESULT_MISS << 16;
  pe_miss.disabled = 1;
  pe_miss.exclude_kernel = 1;
  pe_miss.exclude_hv = 1;
  pe_miss.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
  fd_miss = perf_event_open(&pe_miss, 0, -1, fd, 0);
  if (fd_miss == -1) {
    fprintf(stderr, "Error opening performance event miss.\n");
    exit(EXIT_FAILURE);
  }

  out->g_fd = fd;
  out->hit_id = get_perf_event_id(fd);
  out->miss_id = get_perf_event_id(fd_miss);
  return out;
}