#! /bin/bash
###### Run command from parameters in background
$* &
###### Save pid of last background command
pid=$!
###### Install probe, running on the pid specified
sudo insmod pf_probe_A.ko process_id=$pid
###### wait for the background job to complete
wait $pid
###### Remove the module
sudo rmmod pf_probe_A
###### Look at the last 50 lines of the console print
dmesg | tail -50
sudo kill -9 $pid
