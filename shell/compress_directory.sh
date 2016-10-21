#!/bin/bash
# Copyright (C) 2016  Ricardo Biehl Pasquali <rbpoficial@gmail.com>

# Based in `makepkg` utilitary made by Patrick Volkerding
# to the Slackware Linux

print_usage() {
	echo "usage: cmd <directory> <package_name.ext>"
	echo "  example: cmd /tmp ./my_pack.tgz"
	echo "  supported extensions:"
	echo "    tgz (tar.gz) gzip"
	echo "    tbz (tar.bz2) bzip2"
	echo "    tlz (tar.lzma) lzma"
	echo "    txz (tar.xz) xz"
	exit 1
}

directory="$1"
package_name="$2"
tar_cmd="tar cvf -"

# four checks now:

# check if parameters are blank
if [ -z "$directory" -o -z "$package_name" ]; then
	print_usage
fi

# check if $directory realy exists
if [ ! -d "$directory" ]; then
	echo "Directory $directory doesn't exist!"
	print_usage
fi

# check if $package_name file already exists
if [ -e "$package_name" ]; then
	echo "File $package_name already exists!"
	print_usage
fi

# check if tar command exists
tar --version >/dev/null
if [ ! "$?" = 0 ]; then
	echo "tar command was not found in PATH!"
	exit 1
fi

# identify package extension
extension="$(echo $package_name | rev | cut -f 1 -d . | rev)"
case "$extension" in
"tgz")
	gzip -V >/dev/null
	if [ ! "$?" = 0 ]; then
		echo "gzip command was not found in PATH!"
		exit 1
	fi
	compression_cmd="gzip -9c"
;;
"tbz")
	bzip2 -V >/dev/null
	# bzip2 returns 1 when executed with '-V'
	if [ ! "$?" = 1 ]; then
		echo "bzip2 command was not found in PATH!"
		exit 1
	fi
	compression_cmd="bzip2 -9c"
;;
"tlz")
	lzma -V >/dev/null
	if [ ! "$?" = 0 ]; then
		echo "lzma command was not found in PATH!"
		exit 1
	fi
	compression_cmd="lzma -c"
;;
"txz")
	xz -V >/dev/null
	if [ ! "$?" = 0 ]; then
		echo "xz command was not found in PATH!"
		exit 1
	fi
	compression_cmd="xz -c"
;;
*)
	echo "Error: Package extension .$extension is not supported."
	print_usage
esac

# construct tar cmd
tar_cmd="$tar_cmd $directory"

# execute final command
$tar_cmd | $compression_cmd > $package_name

errcode="$?"
if [ ! $errcode = 0 ]; then
	echo "Error: $extension returned error code $errcode"
	echo "command was:  $tar_cmd | $compression_cmd > $package_name"
	exit 1
fi
