#!/bin/bash
# Copyright (C) 2017  Ricardo Biehl Pasquali <rbpoficial@gmail.com>
# under the terms of the GNU General Public License (see LICENSE file)

# put all processes except victim_pid (pid argument to this script) to run in a
# different cpu  --  victim_pid is bossy of cpu :-)
#
# about cpuset mountpoint:
#
#   cpuset_mountpoint/
#       tasks             processes/tasks belonging to this (main) cpuset
#       cpuset.cpus       list of cpus where tasks in this (main) cpuset can run
#       ...               another (now irrelevant) files in this (main) cpuset
#       foobar_cpuset/    directory containing other cpuset
#           tasks
#           cpuset.cpus
#           ...
# a process can belong to only one cpuset at a time

unset victim_pid

case "$1" in
"-h"|"--help")
	echo "put all processes except pid argument to this"
	echo "script to run in a different cpu"
;&
"")
	echo
	echo "  usage: cmd [<pid>|--help|undo]"
	echo
	echo "<pid>    a valid process id"
	echo "--help   print help"
	echo "undo     (not available yet) undo changes made by this script"
	exit 0
;;
"undo")
	echo "OP not available"
	exit 1
;;
*)
	victim_pid="$1"
;;
esac


#
# make sure we are root
#

if (( $EUID != 0 )); then
	echo "you must be root (uid 0) to run this, exiting"
	exit 1
fi


#
# access cpuset mountpoint
#

cpuset_mountpoint="$(cat /proc/mounts | grep "cpuset" | cut -d " " -f 2)"

if [[ -z "$cpuset_mountpoint" ]]; then
	echo "cpuset directory is not mounted, trying to mount it"

	cpuset_mountpoint="/dev/cpuset"
	mkdir "$cpuset_mountpoint"
	mount -t cpuset cpuset "$cpuset_mountpoint"
	if (( $? != 0 )); then
		rmdir "$cpuset_mountpoint"
		echo "couldn't mount cpuset directory, exiting"
		exit 1
	fi
	echo "successfully mounted cpuset directory:"
	echo "    \"${cpuset_mountpoint}\""
fi

cd "$cpuset_mountpoint"
if (( $? != 0 )); then
	echo "error while accessing cpuset mount directory:"
	echo "    \"${cpuset_mountpoint}\""
	exit 1
fi


#
# try to pick up a cpu
#

unset victim_cpu

# get number of online cpus
n_online_cpus="$(getconf _NPROCESSORS_ONLN)"
if (( $n_online_cpus < 2 )); then
	echo "we need at least two cpus online, exiting"
	exit 1
fi

# get cpus allowed in the main cpuset
curr_cpus_allowed="$(cat ./cpuset.cpus)"

last_cpu="$(( n_online_cpus - 1 ))"

for (( tmp_cpu = last_cpu; tmp_cpu != 0; tmp_cpu-- )); do
	if (( $(cat /sys/devices/system/cpu/cpu${tmp_cpu}/online) == 1 )) &&
	   [[ "$curr_cpus_allowed" == *${tmp_cpu}* ]]; then
		victim_cpu="$tmp_cpu"
		break
	fi
done

if (( $victim_cpu == 0 )); then
	echo "your system has only one cpu online, exiting"
	echo "  to enable cpu 1 for example, run:"
	echo "    echo 1 >/sys/devices/system/cpu/cpu1/online"
	exit 1
fi

echo "cpu $victim_cpu picked up"


#
# create cpuset for victim_pid
#

if [[ ! ( "$victim_pid" =~ ^[0-9]+$ ) ]] ||
   [[ ! -d "/proc/$victim_pid" ]]; then
	echo "process \"$victim_pid\" doesn't exist, exiting"
	exit 1
fi

victim_cpuset="ipmic_cpuset"
mkdir "$victim_cpuset"

echo "$victim_cpu" > "./${victim_cpuset}/cpuset.cpus"
echo "$victim_pid" > "./${victim_cpuset}/tasks"


#
# create cpuset for other tasks
#

other_cpuset="other_cpuset"
mkdir "$other_cpuset"

if (( $victim_cpu == 1 )); then
	# only cpu 0 available for other tasks
	echo "0" > "./${other_cpuset}/cpuset.cpus"
else
	# we have a range of cpus available for other tasks
	echo "0-$(( $victim_cpu - 1 ))"
fi

for task in `cat tasks`; do
	echo "$task" > "./${other_cpuset}/tasks"
done

# We are ready ;-)
