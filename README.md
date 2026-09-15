# Mini Project 1

## C-Shell

```bash
cd c-shell
make all
./shell.out
```

## xv6 Scheduler

```bash
cd xv6
make clean
make qemu
```

Inside the xv6 shell, run:

```text
schedulertest
```

Run MLFQ or FIFO with one CPU for comparable measurements:

```bash
make clean
make qemu SCHEDULER=MLFQ CPUS=1
make clean
make qemu SCHEDULER=FIFO CPUS=1
```

## Generate Plots

From the `xv6` directory:

```bash
python3 plot_scheduler_results.py
python3 plot_mlfq_timeline.py
```

The plots are saved as `scheduler_comparison.png` and `mlfq_timeline.png`.
The report is available in `xv6/report.md`.
