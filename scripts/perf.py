import os
import sys
import subprocess

# path = ""
# if (len(sys.argv) > 1):
#     path = sys.argv[1]
# else:
#     raise Exception("Please provide a path")
# file = open(path, "r")
# lines = file.readlines()

# set = {}
# for line in lines:
#     sp = line.split("~~~~~~~")
#     if (len(sp) == 2):
#         if sp[1] in set:
#             set[sp[1]] += 1
#         else:
#             set[sp[1]] = 1
# print(set)

if len(sys.argv) < 3:
    raise Exception("Please provide all params")

hex_val_to_label = {}

all_counters = [
    [
    "CPU_CYCLES:0011",
    "L1D_CACHE:0004",
    "INST_RETIRED:0008",
    "BR_MIS_PRED:0010"
    ],
    [
    "L2D_CACHE:0016",
    "STALL_FRONTEND:0023",
    "STALL_BACKEND:0024",
    "LD_SPEC:0070"
    ],
    [
    "LD_COMP_WAIT_L2_MISS:0180",
    "LD_COMP_WAIT_L2_MISS_EX:0181",
    "LD_COMP_WAIT_L1_MISS:0182",
    "LD_COMP_WAIT_L1_MISS_EX:0183"
    ],
    [
    "LD_COMP_WAIT:0184",
    "LD_COMP_WAIT_EX:0185",
    "L1_MISS_WAIT:0208",
    "L1_PIPE0_VAL:0240"
    ],
    [
    "L1_PIPE1_VAL:0241",
    "L1_PIPE0_COMP:0260",
    "L1_PIPE1_COMP:0261",
    "L2_MISS_WAIT:0308"
    ],
    [
    "L2_MISS_COUNT:0309",
    "L2_PIPE_VAL:0330",
    "L2_PIPE_COMP_ALL:0350"
    ]
]

def get_exec_args(exec_args: str):
    return exec_args.split(',')

def make_perf_counters(counters):
    arg_counters = []
    for counter in counters:
        label, hex_val = counter.split(':')
        p_counter = "r%s:u" % hex_val
        hex_val_to_label[p_counter] = label
        arg_counters.append(p_counter)
    return ','.join(arg_counters)

def getParams(params, counters):
    p_obj = {
        "exec_args": [],
        "exec": "",
        "runs": "",
        "counters": "",
        "out_dir": ""
    }

    for param in params:
        key, value = param.split("=")
        if  key == "exec":
            p_obj["exec"] = value
        elif key == "out_dir":
            p_obj["out_dir"] = value
        elif  key == "exec_args":
            p_obj["exec_args"] = get_exec_args(value)
        elif key == "runs":
            p_obj["runs"] = value
        else:
            raise Exception("Unknowm key: " + key)
    p_obj["counters"] = make_perf_counters(counters)    
    return p_obj

count = 0
out_dir = getParams(sys.argv[1:], all_counters[0])["out_dir"]

for counters in all_counters:
    params = getParams(sys.argv[1:], counters)
    out_dir = params["out_dir"]
    cmd = ["perf", "stat", "-o", ("%s/perf-%s.txt" % (params["out_dir"], count )), "-e", params["counters"], "-r", params["runs"], params["exec"]]
    cmd = cmd + params["exec_args"]
    result = subprocess.run(cmd, stdout=subprocess.PIPE)
    count += 1

for i in range(count):
    file = open("%s/perf-%s.txt" % (out_dir, i), "r")
    lines = file.readlines()

    lines_ = lines[5:]
    for line in lines_:
        line_ = line.strip()
        if (line_ != "\n"):
            sp = line_.split("      ")
            if len(sp) > 1:
                label = hex_val_to_label[sp[1]]
                print("\t%s (%s) == %s"% (label, sp[1] ,sp[0]))
        else:
            break
