import sys
import subprocess
import csv


counted_events = {
    "CPU_CYCLES" : 0,
    "INST_RETIRED": 0,
    "L1D_CACHE": 0,
    "L2D_CACHE": 0,
    "L2_MISS_COUNT": 0,
    "STALL_FRONTEND": 0,
    "STALL_BACKEND": 0,
}

labels = [
    "CPU_CYCLES",
    "INST_RETIRED",
    "L1D_CACHE",
    "L2D_CACHE",
    "L2_MISS_COUNT",
    "STALL_FRONTEND",
    "STALL_BACKEND",
]

all_counters = [
    [
    "CPU_CYCLES",
    "INST_RETIRED",
    ],
    [
    "L1D_CACHE",
    "L2D_CACHE",
    ],
    [
    "L2_MISS_COUNT",
    "STALL_FRONTEND",
    ],
    [
    "STALL_BACKEND",
    ]
]
def get_exec_args(exec_args: str):
    return exec_args.split(',')

def getParams(params):
    p_obj = {
        "bin": "",
        "args": [],
        "runs": "",
        "csv_name": "",
        "out_dir": "",
    }
    for param in params:
        key, value = param.split("=")
        if  key == "bin":
            p_obj["bin"] = value
        elif key == "out_dir":
            p_obj["out_dir"] = value
        elif  key == "args":
            p_obj["args"] = get_exec_args(value)
        elif key == "runs":
            p_obj["runs"] = value
        elif key == "csv_name":
            p_obj["csv_name"] = value
        else:
            raise Exception("Unknowm key: " + key) 
    return p_obj

count = 0
params = getParams(sys.argv[1:])

for counters in all_counters:
    out_dir = params["out_dir"]
    cmd = [params["bin"]] + params["args"] + [("--runs=%s" % params["runs"]), ("--counters=%s" % ",".join(counters))]
    result = subprocess.run(cmd, stdout=subprocess.PIPE)
    for line in result.stdout.decode('utf-8').splitlines():
        if line.startswith("PMU_EVENT"):
            prefix, counter, value = line.split(":")
            counted_events[counter] = int(value)
    print(result.stdout.decode('utf-8'))

with open('%s/%s.csv' % (params["out_dir"], params["csv_name"]), 'w', newline='') as csvfile:
    fieldnames = list(counted_events.keys())
    writer = csv.DictWriter(csvfile, fieldnames=labels)
    writer.writeheader()
    writer.writerow(counted_events)
