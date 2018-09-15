bw = [0.3, 1, 5]
lr = [0, 1, 2.5, 5]
delay = [50, 150, 250, 500]
import subprocess
import os

def main():
    for bandwidth in bw:
        for loss_rate in lr:
            for d in delay:
                print "Starting with bandwidth = ", bandwidth, ", loss_rate = ", loss_rate, ", delay = ", d
                os.system('/usr/bin/python ../../waf --run "tcp-variant-test --numClients=2 --duration=100 --latency=' + str(d) + ' --loss_rate=' + str(loss_rate) + ' --bandwidth=' + str(bandwidth) + '"')
main()
