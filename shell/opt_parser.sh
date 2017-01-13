#!/bin/bash
# Copyright (C) 2016  Ricardo Biehl Pasquali <rbpoficial@gmail.com>
# under the terms of the GNU General Public License (see LICENSE file)

#   What is it?
# This code gives a tool that parses options and mandatory arguments from
# command line. It aims to be easy, simple and powerful.
# Enjoy it :-)
#
# In _this_example_ we have two mandatory arguments, one before and another
# after options. If you want to use only one mandatory argument after/before
# options or you don't want to use any mandatory arguments simply remove
# code between
#     "PRE/POST-OPTIONS argument"
# commented lines.

script_name="$0"

function print_usage () {
	echo "usage: $script_name <your_name> [OPTIONS] <your_age>"
	echo "  try $script_name -h"
	exit 1
}

function print_help () {
	echo
	echo "    usage: $script_name <your_name> [OPTIONS] <your_age>"
	echo
	echo "OPTIONS:"
	echo "  -h|--help  Print this help."
	echo "  -d|--pwd  Print name of current/working directory."
	echo "  -p|--print <name>  Print <name>."
	exit 0
}

# help option should be always the first
if [[ "$1" == "-h" || "$1" == "--help" ]]; then
	print_help
fi

#        PRE-OPTIONS argument
# here we get pre-options argument
if [[ -z "$1" ]]; then
	print_usage
fi
your_name="$1"
shift # ok, we've already got your name!
#        PRE-OPTIONS argument

#        POST-OPTIONS argument
# if there is nothing to be parsed, print usage and return 1.
# >> this only makes sense in case where there is a mandatory post-options
#    argument, and this is our case!
if [[ -z "$1" ]]; then
	print_usage
fi
#        POST-OPTIONS argument

unset opt_pwd
unset opt_print
unset optval_print

# no effective action should happen in option parsing
while (( 1 )); do
	case "$1" in
	"-d"|"--pwd")
		opt_pwd=1
	;;
	"-p"|"--print")
		if [[ -z "$2" ]]; then
			print_usage
		fi
		opt_print=1
		optval_print="$2"
		shift
	;;
	*)
		break
	;;
	esac

	shift # each iteration we assume at least one option was parsed
done

#        POST-OPTIONS argument
# here we get post-options argument
if [[ -z "$1" ]]; then
	print_usage
fi
your_age="$1"
shift
#        POST-OPTIONS argument

#################################
# and here your program goes ...#
#################################

echo "Hello $your_name!"
echo "  I think you put something else rather than your age!"
echo "  But here it is: $your_age"
if (( $opt_pwd )); then
	echo "  Current directory is:  $(pwd)"
fi
if (( $opt_print )); then
	echo -n "  You has input: "
	if [[ -z "$optval_print" ]]; then
		echo "(nothing)"
	else
		echo "$optval_print"
	fi
fi
