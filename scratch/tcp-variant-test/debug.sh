NS_LOG="SingleLinkMultipleTcpTest" /usr/bin/python ../../waf --run tcp-variant-test --command-template="gdb --args %s --duration=200 --latency=50 --loss_rate=0 --bandwidth=0.3 --cubic=1 --bbr=1"
