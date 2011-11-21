#!/bin/sh
# Script for starting ospfd,zebra, ospfn and ccnd
# Author Hoque  - Nov/18/2011

# Starting zebra
echo "Starting Zebra.....";
zebra -d 
sleep 2
echo "Done";


# starting ospfd
echo "starting ospfd.....";
ospfd -d -a
sleep 2
echo "Done";


#Starting ccnd
echo "Starting ccnd.....";
ccndstart
sleep 2
echo "Done";


#Starting ospfn
echo " Give ospfn.conf file path with name";
read ospfnconf
echo "starting ospfn.....";
ospfn -d -f $ospfnconf
echo "Done";



