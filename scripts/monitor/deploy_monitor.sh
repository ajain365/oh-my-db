set -x

start_monitor()
{
    SSH_HOST=$1
    iters=$2
    numPair=$3

    ssh ${SSH_HOST} << EOF
    cd oh-my-db
    ./scripts/monitor/monitor.sh
EOF
}

parse_ssh_hosts()
{
    local filename=$1
    local i=0
    local arr=()
    while read -r line; do
      if [[ $i -eq 0 ]]; then
        ((i=i+1))
        continue
      fi
      arr[i-1]=$(echo $line | awk -F',' "{print ${2}}")
      ((i=i+1))
    done < config.csv
    echo ${arr[@]}
}


# here assuming the 4-th column is the hostname, and 5-th column is the username
SSH_HOSTS=($(parse_ssh_hosts config.sh \$5\"@\"\$4))

## Get the length of the array
#size=${#SSH_HOSTS[@]}
#
##for SSH_HOST in "${SSH_HOSTS[@]}"
#for ((i=0; i<$size; i++)); do
#
#
#SSH_HOST=${SSH_HOSTS[0]}
#
# Get the length of the array
size=${#SSH_HOSTS[@]}
for ((i=0; i<$size; i++)); do

SSH_HOST=${SSH_HOSTS[$i]}

start_monitor ${SSH_HOST} $i &

done

#start_test ${SSH_HOST} 20 50

#done

wait
