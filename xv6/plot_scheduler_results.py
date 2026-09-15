"""
Raw schedulertest output

MLFQ, CPUS=1:

init: starting sh$ schedulertest
schedulertest: short pid=4 started
schedulertest: cpu-bound pid=5 started
schedulertest: io-bound pid=6 started
schedulertest: mixed pid=7 started
schedulertest: short pid=4 finished
schedulertest: metrics pid=4 turnaround=15 waiting=11 response=0 running=4 sleeping=0
schedulertest: io-bound pid=6 finished
schedulertest: metrics pid=6 turnaround=83 waiting=65 response=2 running=18 sleeping=40
schedulertest: cpu-bound pid=5 finished
schedulertest: metrics pid=5 turnaround=88 waiting=54 response=1 running=34 sleeping=0
schedulertest: mixed pid=7 finished
schedulertest: metrics pid=7 turnaround=91 waiting=58 response=3 running=33 sleeping=14
schedulertest: all workloads finished
$

Round Robin, CPUS=1:

$ schedulertest
schedulertest: short pid=4 started
schedulertest: cpu-bound pid=5 started
schedulertest: io-bound pid=6 started
schedulertest: mixed pid=7 started
schedulertest: short pid=4 finished
schedulertest: metrics pid=4 turnaround=13 waiting=9 response=0 running=4 sleeping=0
schedulertest: cpu-bound pid=5 finished
schedulertest: metrics pid=5 turnaround=83 waiting=43 response=1 running=40 sleeping=0
schedulertest: io-bound pid=6 finished
schedulertest: metrics pid=6 turnaround=88 waiting=75 response=2 running=13 sleeping=32
schedulertest: mixed pid=7 finished
schedulertest: metrics pid=7 turnaround=95 waiting=60 response=3 running=35 sleeping=16
schedulertest: all workloads finished
$

FIFO, CPUS=1:

$ schedulertest
schedulertest: short pid=4 started
schedulertest: short pid=4 finished
schedulertest: metrics pid=4 turnaround=12 waiting=0 response=0 running=12 sleeping=0
schedulertest: cpu-bound pid=5 started
schedulertest: cpu-bound pid=5 finished
schedulertest: metrics pid=5 turnaround=92 waiting=12 response=12 running=80 sleeping=0
schedulertest: io-bound pid=6 started
schedulertest: mixed pid=7 started
schedulertest: io-bound pid=6 finished
schedulertest: metrics pid=6 turnaround=164 waiting=140 response=92 running=24 sleeping=48
schedulertest: mixed pid=7 finished
schedulertest: metrics pid=7 turnaround=184 waiting=132 response=94 running=52 sleeping=16
schedulertest: all workloads finished

The original terminal output contained the same lines, with some output
interleaved when more than one CPU was used.
"""

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np


# Data extracted from the CPUS=1 schedulertest output above.
metrics = {
    "FIFO": {"turnaround": 113.00, "waiting": 71.00, "response": 49.50},
    "RR": {"turnaround": 69.75, "waiting": 46.75, "response": 1.50},
    "MLFQ": {"turnaround": 69.25, "waiting": 47.00, "response": 1.50},
}

metrics_names = ["Turnaround", "Waiting", "Response"]
schedulers = list(metrics)
values = np.array([
    [metrics[scheduler][metric.lower()] for metric in metrics_names]
    for scheduler in schedulers
])

fig, ax = plt.subplots(figsize=(9, 5.5))
x = np.arange(len(metrics_names))
width = 0.24

for index, scheduler in enumerate(schedulers):
    ax.bar(x + (index - 1) * width, values[index], width, label=scheduler)

ax.set_title("Scheduler Comparison")
ax.set_ylabel("Average time (ticks)")
ax.set_xticks(x)
ax.set_xticklabels(metrics_names)
ax.legend(title="Scheduler")
ax.grid(axis="y", linestyle="--", alpha=0.35)

watermark = "sasikanth.sibyala@research.iiit.ac.in"
fig.text(0.99, 0.01, watermark, ha="right", va="bottom", alpha=0.5)

fig.tight_layout()
fig.savefig("scheduler_comparison.png", dpi=200, bbox_inches="tight")
