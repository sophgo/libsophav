#!/bin/bash
#-----------------------------------------------------------------------------
# COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
#
# This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
# SPDX License Identifier: BSD-3-Clause
# SPDX License Identifier: LGPL-2.1-only
#
# The entire notice above must be reproduced on all authorized copies.
#
# Description  :
#-----------------------------------------------------------------------------

set -u

# ====================================================================#
# Source
# ====================================================================#

# ====================================================================#
# debug_msg() - print debug msg
# ====================================================================#
_DEBUG_HDR_FMT="[%s][%s][%s]: "
_DEBUG_MSG_FMT="${_DEBUG_HDR_FMT}%s\n"
function debug_msg() {
    printf "$_DEBUG_MSG_FMT" ${BASH_SOURCE[1]##*/} ${FUNCNAME[1]} ${BASH_LINENO[0]} "${@}"
}

# ====================================================================#
# i_enum() - 
# ====================================================================#
function i_enum()
{
    local A=0
    local i=0

    A=${@##*\{}
    A=${A%\}*}
    A=${A//,/}
    Array=(${A})
    eval "${Array[0]}=()"
    for ((i=1;i<${#Array[@]};i++)) ; do
        eval "${Array[$i]}=$(($i-1))"
        eval "${Array[0]}+=($(($i-1)))"
    done
}

# ====================================================================#
# h_enum() - 
# ====================================================================#
function h_enum()
{
    local A=0
    local i=0

    A=${@##*\{}
    A=${A%\}*}
    A=${A//,/ }
    Array=(${A})
    eval "${Array[0]}=()"
    for ((i=1;i<${#Array[@]};i++)) ; do
        eval "${Array[$i]}=$((1<<($i-1)))"
        eval "${Array[0]}+=($((1<<($i-1))))"
    done
}

# ====================================================================#
# array_to_enum() - 
# ====================================================================#
function array_to_enum
{
    local idx=0
    for val in "${@}";
    do
        eval "$val=${idx}"
        idx=$((idx+1))
    done
}

# ====================================================================#
# echo_and_run() - run command and check a return code
# ====================================================================#
function run_and_check {
    local cmd="$1"
    $cmd
    if (( $? != 0 ));then
        echo "[ERROR] CMD Failure : $cmd"
        exit 1
    fi
}

# ====================================================================#
# log_conf() - write @log_message to @log_file
# ====================================================================#
function log_conf {
    echo "$1" | tee -a "$2"
}

# ====================================================================#
# log_conf_and_run() - write @log_message and excution & check
# ====================================================================#
function log_conf_and_run {
    local arg_val=("$@")
    if (( ${#arg_val[@]} == 1 )); then
        run_and_check "${arg_val[0]}"
    elif (( ${#arg_val[@]} == 2 )); then
        if [ "" !=  "${arg_val[1]}" ];then
            echo "${arg_val[0]}" | tee -a "${arg_val[1]}"
        fi
        run_and_check "${arg_val[0]}"
    fi
}

# ====================================================================#
# ceiling() - align function
# ====================================================================#
# ceiling function
function ceiling {
    local value=$1
    local align=$2

    echo "$(((($value+$align-1)/$align)*$align))"
}

# ====================================================================#
# check_and_create_dir() - 
# ====================================================================#
function check_and_create_dir {
    CHECK_PATH=$1
    if [ ! -d "$CHECK_PATH" ]; then
        mkdir -p $CHECK_PATH
        chmod 777 $CHECK_PATH
        echo "create $CHECK_PATH"
    else
        echo "Already $CHECK_PATH Exist"
    fi
}

# ====================================================================#
# get_random() - generate constraint random value with @start end @end.
# ====================================================================#
function get_random {
    start=$1
    end=$2
    size=$(($end - $start + 1))
    val2=$RANDOM
    val1=$(($val2 % $size))
    val=$(($start + $val1 % $size))
    echo "$val"
}

# ====================================================================#
# get_random_min_max() -
# ====================================================================#
function get_random_min_max {
    start=$1
    end=$2

    use_min_max_value=$(($RANDOM % 5))
    size=$(($end - $start - 1))

    if [ $use_min_max_value = 1 ] || [ $size -le 0 ]; then
        val2=$RANDOM
        val1=$(($val2 % 2))
        if [ $val1 = 1 ]; then
            val=$end
        else
            val=$start
        fi
    else
        val2=$RANDOM
        val1=$(($val2 % $size))
        val=$(($start + $val1 % $size + 1))
    fi

    echo "$val"
}

# ====================================================================#
# gen_val() - generate random value (none or default_val -99)
# ====================================================================#
function gen_val {
    local min_val=$1
    local max_val=$2
    local default_val=$3
    local val

    if [ -z "$default_val" ] || [ "$default_val" == "-99" ]; then
        # Generate random value
        val=$(get_random $min_val $max_val)
    else
        val=$default_val
    fi

    echo "$val"
}

# ====================================================================#
# gen_val_rand() - generate random value (none or default_val 'rand')
# ====================================================================#
function gen_val_rand {
    local min_val=$1
    local max_val=$2
    local default_val=$3
    local val

    if [ -z "$default_val" ] || [ "$default_val" == "rand" ]; then
        # Generate random value
        val=$(get_random $min_val $max_val)
    else
        val=$default_val
    fi

    echo "$val"
}

# ====================================================================#
# gen_min_max_val() -
# ====================================================================#
function gen_min_max_val {
    local min_val=$1
    local max_val=$2
    local default_val=$3
    local val

    if [ -z "$default_val" ] || [ "$default_val" == "-99" ]; then
        # Generate random value
        val=$(get_random_min_max $min_val $max_val)
    else
        val=$default_val
    fi

    echo "$val"
}

# ====================================================================#
# get_bit_value() - get bit from value
# ====================================================================#
function get_bit_value()
{
    local value=${1:-0}
    local bit=${2-0}
    local bit_mask=$(( 1 << $bit ))
    echo $(( value & bit_mask ? 1 : 0 ))
}

# ====================================================================#
# get_file_extension() - get a file extension
# ====================================================================#
function get_file_extension()
{
    local file=${1}
    local extension=""
    extension="${file##*.}"
    echo "$extension"
}

# ====================================================================#
# get_file_size() - get a file size
# ====================================================================#
function get_file_size()
{
    local file=${1}
    local size=0
    size=$(wc -c $file | awk '{print $1}')
    if (( size < 0 )); then
        size=0
    fi
   echo "$size"
}

# ====================================================================#
# get_time() - get time string (YYYY_HH_MM_SS_NNNNNNNNN)
# ====================================================================#
function get_time()
{
    local time_str
    time_str=$(date +%Y_%H_%M_%S_%N)
    echo "$time_str"
}