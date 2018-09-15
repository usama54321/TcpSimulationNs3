import os
from shutil import copyfile
from subprocess import Popen, PIPE

def main():
    try:
        os.mkdir("graphs")
    except OSError as e:
        pass

    files = [f for f in os.listdir('./logs') if f[-4:] == 'pcap'] 

    script_dir = os.path.join(os.getcwd(), "scripts")
    makefile = os.path.join(script_dir, "Makefile")
    gnufile = os.path.join(script_dir, "throughput.gpi")
    fairnessfile = os.path.join(script_dir, "fairness.gpi")
    for f in files:
        file_path = os.path.join(os.getcwd(), 'logs', f)
        graph_dir = os.path.join(os.getcwd(), 'graphs')
        graph_out_dir = os.path.join(graph_dir, f)
        try:
            os.mkdir(graph_out_dir)
        except OSError as e:
            continue

        #copy data
        for i in range(0,2):
            print file_path
            p = Popen(["captcp",  "throughput",  "-f " + str(i + 1), file_path, "--stdio"], stdout=PIPE)
            output = p.stdout.read()
            output_file = open(os.path.join(graph_out_dir, "throughput{}.data".format(i + 1)), 'w')
            output_file.write(output)
        
        copyfile(makefile, os.path.join(graph_out_dir, "Makefile"))
        copyfile(gnufile, os.path.join(graph_out_dir, "throughput.gpi"))
        copyfile(fairnessfile, os.path.join(graph_out_dir, "fairness.gpi"))
        os.system("make -C " + graph_out_dir)
main()
