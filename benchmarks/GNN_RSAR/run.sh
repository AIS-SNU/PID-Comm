make clean
NR_TASKLETS=16 TYPE=INT8 make all
# number of dpu(power of 2) / graph file name / size of feature matrix (power of 2) / use PID-Comm or not (1;0) 
./bin/host 1024 pubmed 256 1
./bin/host 1024 pubmed 256 0
./bin/host 1024 citeseer 256 1
./bin/host 1024 citeseer 256 0

make clean
NR_TASKLETS=16 TYPE=INT32 make all
./bin/host 1024 pubmed 128 1
./bin/host 1024 pubmed 128 0
./bin/host 1024 citeseer 128 1
./bin/host 1024 citeseer 128 0