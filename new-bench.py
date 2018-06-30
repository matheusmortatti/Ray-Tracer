#!/usr/bin/env python3

import subprocess
import sys
import os
import numpy as np
import time
import timeit

BENCHMARKS = [
    #("BENCHMIN", ["-col", "1", "-row", "1"]),
	("BENCHMID", ["-col", "30", "-row", "30", "-n"]),
    ("BENCHLARGE", ["-col", "70", "-row", "70", "-n"]),
    ("BENCHXLARGE", ["-col", "100", "-row", "100", "-n"]),
]

dir_path = os.path.dirname(os.path.realpath(__file__))
prot_dir = dir_path

TIMES = 3  # Number of time the execution is repeated
NB_NODE = [16, 8, 4, 2, 1]

SLEEP_TIME = 0.1
CLEAN = False

PROFILE_NAME = "raytracer-logs"
WORKING_DIR = os.path.join(dir_path, PROFILE_NAME)

build_profiles = [
    ('static w/ 1 exec', 'Makefile', "ini-mapr")
    #('dynamic w/ 1 exec', 'Makefile', "ini-mapr-dyn")
]


def run(*popenargs, input=None, check=False, **kwargs):
    if input is not None:
        if 'stdin' in kwargs:
            raise ValueError('stdin and input arguments may not both be used.')
        kwargs['stdin'] = subprocess.PIPE

    process = subprocess.Popen(*popenargs, **kwargs)
    try:
        stdout, stderr = process.communicate(input)
    except:
        process.kill()
        process.wait()
        raise
    retcode = process.poll()
    if check and retcode:
        raise subprocess.CalledProcessError(
            retcode, process.args, output=stdout, stderr=stderr)
    return retcode, stdout, stderr


class Logger(object):
    def __init__(self, filepath):
        self.terminal = sys.stdout
        self.log = open(filepath, "a")

    def write(self, message):
        self.terminal.write(message)
        self.log.write(message)

    def flush(self):
        self.terminal.flush()
        self.log.flush()


def printMedians(csv, v):
    csv.write('{:.1f},'.format(np.mean(v)))  # Median
    csv.write('{:.1f},'.format(np.var(v)))  # Variance
    csv.write('{:.1f}\n'.format(np.std(v)))  # Std deviation


def printCSVline(csv, name, t, p, s, u, d):
    csv.write(name + ',total,')
    for v in t:
        csv.write('{:.1f},'.format(v))
    printMedians(csv, t)

    csv.write(',parallel for,')
    for v in p:
        csv.write('{:.1f},'.format(v))
    printMedians(csv, p)

    csv.write(',rest,')
    for v in s:
        csv.write('{:.1f},'.format(v))
    printMedians(csv, s)

    csv.write(',up,')
    for v in u:
        csv.write('{:.1f},'.format(v))
    printMedians(csv, u)

    csv.write(',down,')
    for v in d:
        csv.write('{:.1f},'.format(v))
    printMedians(csv, d)


if not os.path.exists(WORKING_DIR):
    os.mkdir(WORKING_DIR)
timestr = time.strftime("%Y%m%d-%H%M%S")
log_dir = os.path.join(WORKING_DIR, timestr)
os.mkdir(log_dir)
# Write message into a file as well
sys.stdout = Logger(os.path.join(log_dir, "output.log"))

print("- EXPERIMENTS STARTING")

for nb_node in NB_NODE:
    print("-- Compute on {} nodes".format(nb_node))

    for (bench_name, bench_args) in BENCHMARKS:
        print("--- CCS of benchmark {}".format(bench_name))

        csv_name = "result_{}nodes-{}.csv".format(nb_node, bench_name)
        csv = open(os.path.join(log_dir, csv_name), mode='w')
        csv.write(',,')
        for i in range(1, TIMES + 1):
            csv.write(str(i) + ',')
        csv.write('mean,var,std\n')

        for (build_name, makefile, conf_dir) in build_profiles:
            print("---- Profile - " + build_name)

            conf_file = os.path.join(dir_path, conf_dir,
                                    "cloud_{}.ini".format(nb_node))
            if not os.path.exists(conf_file):
                print("Warning - Configuration file does not exist: " + conf_file)
            os.environ['OMPCLOUD_CONF_PATH'] = conf_file

            if CLEAN:
                try:
                    print("Building...")
                    subprocess.run(['make', 'clean', '-f', makefile, '--quiet'])
                    subprocess.run(['make', '-f', makefile, '--quiet'], check=True)
                except subprocess.CalledProcessError as e:
                    print("Execution error: " + e.output)
                    pass

            total = []
            parallel = []
            serial = []
            upload = []
            download = []

            for i in range(TIMES):
                log = "log-{}nodes-{}-{}.{}.out".format(nb_node, bench_name,
                                                        conf_dir, i)
                logpath = os.path.join(log_dir, log)
                logfile = open(logpath, mode='w')

                start = timeit.default_timer()
                try:
                    subprocess.run(['./raytracer', bench_args[0], bench_args[1],
                                    bench_args[2], bench_args[3], '-n'],
                                   stdout=logfile, stderr=logfile, check=True)
                    time.sleep(SLEEP_TIME)
                except subprocess.CalledProcessError as e:
                    if e.output is None:
                        print("Execution error")
                    else:
                        print("Execution error: " + e.output)
                    pass

                elapsed = timeit.default_timer() - start
                print("Execution {} in {:.2f}s".format(i, elapsed))
                total.append(elapsed)
                logfile.close()
                time.sleep(SLEEP_TIME)  # wait to avoid JVM exception

                logfile = open(logpath, mode='r')
                t_parallel = 0
                up = 0
                down = 0
                for line in logfile:
                    if line.startswith("Parallel Time"):
                        t_parallel = float(line.split()[-1].rsplit(' ')[0])
                    if line.startswith("Target Cloud RTL --> Uploading"):
                        up = float(line.split()[-1].rsplit('s')[0])
                    if line.startswith("Target Cloud RTL --> Downloading"):
                        down = float(line.split()[-1].rsplit('s')[0])
                logfile.close()

                parallel.append(t_parallel)
                serial.append(total[i] - parallel[i])

                # Not computed in intra-cluster mode
                upload.append(up)
                download.append(down)

            print("Median: " + str(np.mean(total)))
            print("Variance: " + str(np.var(total)))
            print("Std deviation: " + str(np.std(total)))

            sys.stdout.flush()

            printCSVline(csv, build_name, total, parallel, serial, upload,
                         download)
        csv.close()

print("- EXPERIMENTS ENDING")
