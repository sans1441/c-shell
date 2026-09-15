import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

"""
 schedulertestschedulertest: short pid=4 started
mlfq_trace tick=54 pid=4 queue=0 event=sample
mlfq_trace tick=54 pid=4 queue=1 event=demote from=0
schedulertest: cpu-bound pid=5 started
mlfq_trace tick=55 pid=5 queue=0 event=sample
mlfq_trace tick=55 pid=5 queue=1 event=demote from=0
schedulertest: io-bound pid=6 started
mlfq_trace tick=56 pid=6 queue=0 event=sample
mlfq_trace tick=56 pid=6 queue=1 event=demote from=0
schedulertest: mixed pid=7 started
mlfq_trace tick=57 pid=7 queue=0 event=sample
mlfq_trace tick=57 pid=7 queue=1 event=demote from=0
mlfq_trace tick=58 pid=4 queue=1 event=sample
mlfq_trace tick=59 pid=5 queue=1 event=sample
mlfq_trace tick=60 pid=7 queue=1 event=sample
mlfq_trace tick=61 pid=4 queue=1 event=sample
mlfq_trace tick=62 pid=5 queue=1 event=sample
mlfq_trace tick=63 pid=7 queue=1 event=sample
mlfq_trace tick=64 pid=4 queue=1 event=sample
mlfq_trace tick=65 pid=5 queue=1 event=sample
mlfq_trace tick=66 pid=6 queue=1 event=sample
schedulertest: short pid=4 finished
schedulertest: metrics pid=4 turnaround=13 waiting=9 response=0 running=4 sleeping=0
mlfq_trace tick=67 pid=5 queue=1 event=sample
mlfq_trace tick=67 pid=5 queue=2 event=demote from=1
mlfq_trace tick=68 pid=5 queue=2 event=sample
mlfq_trace tick=69 pid=7 queue=1 event=sample
mlfq_trace tick=70 pid=7 queue=1 event=sample
mlfq_trace tick=70 pid=7 queue=2 event=demote from=1
mlfq_trace tick=71 pid=7 queue=2 event=sample
mlfq_trace tick=72 pid=6 queue=1 event=sample
mlfq_trace tick=73 pid=6 queue=1 event=sample
mlfq_trace tick=74 pid=5 queue=2 event=sample
mlfq_trace tick=75 pid=7 queue=2 event=sample
mlfq_trace tick=76 pid=5 queue=2 event=sample
mlfq_trace tick=77 pid=5 queue=2 event=sample
mlfq_trace tick=78 pid=6 queue=1 event=sample
mlfq_trace tick=78 pid=6 queue=2 event=demote from=1
mlfq_trace tick=79 pid=6 queue=2 event=sample
mlfq_trace tick=80 pid=7 queue=2 event=sample
mlfq_trace tick=81 pid=5 queue=2 event=sample
mlfq_trace tick=82 pid=7 queue=2 event=sample
mlfq_trace tick=83 pid=5 queue=2 event=sample
mlfq_trace tick=84 pid=7 queue=2 event=sample
mlfq_trace tick=85 pid=5 queue=2 event=sample
mlfq_trace tick=86 pid=6 queue=2 event=sample
mlfq_trace tick=87 pid=7 queue=2 event=sample
mlfq_trace tick=88 pid=5 queue=2 event=sample
mlfq_trace tick=88 pid=5 queue=3 event=demote from=2
mlfq_trace tick=89 pid=5 queue=3 event=sample
mlfq_trace tick=90 pid=5 queue=3 event=sample
mlfq_trace tick=91 pid=7 queue=2 event=sample
mlfq_trace tick=92 pid=7 queue=2 event=sample
mlfq_trace tick=92 pid=7 queue=3 event=demote from=2
mlfq_trace tick=93 pid=6 queue=2 event=sample
mlfq_trace tick=94 pid=6 queue=2 event=sample
mlfq_trace tick=95 pid=7 queue=3 event=sample
mlfq_trace tick=96 pid=5 queue=0 event=boost
mlfq_trace tick=96 pid=6 queue=0 event=boost
mlfq_trace tick=96 pid=7 queue=0 event=boost
mlfq_trace tick=96 pid=5 queue=0 event=sample
mlfq_trace tick=96 pid=5 queue=1 event=demote from=0
mlfq_trace tick=97 pid=7 queue=0 event=sample
mlfq_trace tick=97 pid=7 queue=1 event=demote from=0
mlfq_trace tick=98 pid=7 queue=1 event=sample
mlfq_trace tick=99 pid=6 queue=0 event=sample
mlfq_trace tick=99 pid=6 queue=1 event=demote from=0
mlfq_trace tick=100 pid=5 queue=1 event=sample
mlfq_trace tick=101 pid=5 queue=1 event=sample
mlfq_trace tick=102 pid=5 queue=1 event=sample
mlfq_trace tick=103 pid=7 queue=1 event=sample
mlfq_trace tick=104 pid=5 queue=1 event=sample
mlfq_trace tick=104 pid=5 queue=2 event=demote from=1
mlfq_trace tick=105 pid=6 queue=1 event=sample
mlfq_trace tick=106 pid=7 queue=1 event=sample
mlfq_trace tick=107 pid=7 queue=1 event=sample
mlfq_trace tick=107 pid=7 queue=2 event=demote from=1
mlfq_trace tick=108 pid=7 queue=2 event=sample
mlfq_trace tick=109 pid=5 queue=2 event=sample
mlfq_trace tick=110 pid=7 queue=2 event=sample
mlfq_trace tick=111 pid=6 queue=1 event=sample
mlfq_trace tick=112 pid=6 queue=1 event=sample
mlfq_trace tick=113 pid=5 queue=2 event=sample
mlfq_trace tick=114 pid=5 queue=2 event=sample
mlfq_trace tick=115 pid=5 queue=2 event=sample
mlfq_trace tick=116 pid=7 queue=2 event=sample
mlfq_trace tick=117 pid=6 queue=1 event=sample
mlfq_trace tick=117 pid=6 queue=2 event=demote from=1
mlfq_trace tick=118 pid=5 queue=2 event=sample
mlfq_trace tick=119 pid=7 queue=2 event=sample
mlfq_trace tick=120 pid=5 queue=2 event=sample
mlfq_trace tick=121 pid=7 queue=2 event=sample
mlfq_trace tick=122 pid=5 queue=2 event=sample
mlfq_trace tick=123 pid=6 queue=2 event=sample
mlfq_trace tick=124 pid=5 queue=2 event=sample
mlfq_trace tick=124 pid=5 queue=3 event=demote from=2
mlfq_trace tick=125 pid=5 queue=3 event=sample
mlfq_trace tick=126 pid=7 queue=2 event=sample
mlfq_trace tick=127 pid=7 queue=2 event=sample
mlfq_trace tick=128 pid=7 queue=2 event=sample
mlfq_trace tick=128 pid=7 queue=3 event=demote from=2
mlfq_trace tick=129 pid=6 queue=2 event=sample
mlfq_trace tick=130 pid=6 queue=2 event=sample
mlfq_trace tick=131 pid=7 queue=3 event=sample
mlfq_trace tick=132 pid=5 queue=3 event=sample
mlfq_trace tick=133 pid=7 queue=3 event=sample
mlfq_trace tick=134 pid=5 queue=3 event=sample
schedulertest: io-bound pid=6 finished
schedulertest: metrics pid=6 turnaround=81 waiting=64 response=2 running=17 sleeping=41
mlfq_trace tick=135 pid=5 queue=3 event=sample
mlfq_trace tick=136 pid=5 queue=3 event=sample
mlfq_trace tick=137 pid=7 queue=3 event=sample
schedulertest: cpu-bound pid=5 finished
schedulertest: metrics pid=5 turnaround=84 waiting=51 response=1 running=33 sleeping=0
mlfq_trace tick=138 pid=7 queue=3 event=sample
mlfq_trace tick=139 pid=7 queue=3 event=sample
mlfq_trace tick=140 pid=7 queue=3 event=sample
mlfq_trace tick=141 pid=7 queue=3 event=sample
mlfq_trace tick=142 pid=7 queue=3 event=sample
mlfq_trace tick=143 pid=7 queue=3 event=sample
mlfq_trace tick=144 pid=7 queue=0 event=boost
mlfq_trace tick=144 pid=7 queue=0 event=sample
mlfq_trace tick=144 pid=7 queue=1 event=demote from=0
schedulertest: mixed pid=7 finished
schedulertest: metrics pid=7 turnaround=93 waiting=56 response=3 running=37 sleeping=15
schedulertest: all workloads finished
"""

# Data extracted from the mlfq_trace output, in the form (tick, pid, queue).
# The original output also contained demotion events at 54, 55, 56, 57,
# 67, 70, 78, 88, 92, 104, 107, 117, 124, and 128.
samples = [
    (54, 4, 0), (55, 5, 0), (56, 6, 0), (57, 7, 0),
    (58, 4, 1), (59, 5, 1), (60, 7, 1), (61, 4, 1),
    (62, 5, 1), (63, 7, 1), (64, 4, 1), (65, 5, 1),
    (66, 6, 1), (67, 5, 1), (68, 5, 2), (69, 7, 1),
    (70, 7, 1), (71, 7, 2), (72, 6, 1), (73, 6, 1),
    (74, 5, 2), (75, 7, 2), (76, 5, 2), (77, 5, 2),
    (78, 6, 1), (79, 6, 2), (80, 7, 2), (81, 5, 2),
    (82, 7, 2), (83, 5, 2), (84, 7, 2), (85, 5, 2),
    (86, 6, 2), (87, 7, 2), (88, 5, 2), (89, 5, 3),
    (90, 5, 3), (91, 7, 2), (92, 7, 2), (93, 6, 2),
    (94, 6, 2), (95, 7, 3),
    (96, 5, 0), (97, 7, 0), (98, 7, 1), (99, 6, 0),
    (100, 5, 1), (101, 5, 1), (102, 5, 1), (103, 7, 1),
    (104, 5, 1), (105, 6, 1), (106, 7, 1), (107, 7, 1),
    (108, 7, 2), (109, 5, 2), (110, 7, 2), (111, 6, 1),
    (112, 6, 1), (113, 5, 2), (114, 5, 2), (115, 5, 2),
    (116, 7, 2), (117, 6, 1), (118, 5, 2), (119, 7, 2),
    (120, 5, 2), (121, 7, 2), (122, 5, 2), (123, 6, 2),
    (124, 5, 2), (125, 5, 3), (126, 7, 2), (127, 7, 2),
    (128, 7, 2), (129, 6, 2), (130, 6, 2), (131, 7, 3),
    (132, 5, 3), (133, 7, 3), (134, 5, 3), (135, 5, 3),
    (136, 5, 3), (137, 7, 3), (138, 7, 3), (139, 7, 3),
    (140, 7, 3), (141, 7, 3), (142, 7, 3), (143, 7, 3),
    (144, 7, 0),
]

colors = {4: "tab:blue", 5: "tab:orange", 6: "tab:green", 7: "tab:red"}
names = {4: "short", 5: "cpu-bound", 6: "io-bound", 7: "mixed"}

for pid in names:
    points = [(tick, queue) for tick, point_pid, queue in samples if point_pid == pid]
    plt.scatter(
        [point[0] for point in points],
        [point[1] for point in points],
        color=colors[pid],
        label=f"{names[pid]} (pid {pid})",
    )

for tick in (96, 144):
    plt.axvline(tick, color="black", linestyle="--", alpha=0.5)

plt.yticks([0, 1, 2, 3])
plt.xlabel("Time (ticks)")
plt.ylabel("MLFQ queue")
plt.title("MLFQ Queue Timeline")
plt.grid(alpha=0.3)
plt.legend()
plt.text(0.99, 0.01, "sasikanth.sibyala@research.iiit.ac.in", transform=plt.gca().transAxes,
         ha="right", alpha=0.5)
plt.tight_layout()
plt.savefig("mlfq_timeline.png", dpi=200)
