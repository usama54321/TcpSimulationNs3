NS_LOG="SingleLinkMultipleTcpTest" /usr/bin/python ../../waf --run "tcp-variant-test --duration=50 --latency=100 --loss_rate=5 --bandwidth=50 --cubic=500 --bbr=0 --vegas=0 --reno=0 --no-delay=1 --cwnd=6 --rto=1"
#NS_LOG="TcpBbr:SingleLinkMultipleTcpTest:TcpL4Protocol:TcpServerApplication:TcpClientApplication:BbrState" 
