#!bin/sh
#script for killing ospfn
#Author Hoque ahoque1@memphis.edu

kill  `cat /var/run/quagga-state/ospfn.pid`;
