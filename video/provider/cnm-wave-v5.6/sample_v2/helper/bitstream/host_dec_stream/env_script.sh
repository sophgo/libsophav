#!/bin/bash

set -u

function run()
{
    local arg=("$@")
    echo "${arg[0]}" && ${arg[0]}
    if (( $? != 0 ));then
        exit 1
    fi
}

CP="cp -vf"
COM_PRE_PATH="../../../../../../../design/refc/src/com"
HOST_PRE_PATH="../../../../../../../design/refc/src/host"

run "$CP ${COM_PRE_PATH}/com.h ./"
run "$CP ${HOST_PRE_PATH}/host_dec_stream.c ./"
run "$CP ${HOST_PRE_PATH}/host_dec.h ./"

#include "host.h" --> host_dec_stream.c 제거 
#include "com.h" --> host_dec_stream.c 추가
#int host_dec_api( CMD_ARG* cmd_arg ); --> host_dec.h 제거
#include "com_reg.h" --> com.h 제거
#include "com_reg.h" --> com.h 제거
##define INT MAX --> com.h 제거

sed -i '/#include \"host.h\"/d' host_dec_stream.c
sed -i'' -r -e "/#include \"host_dec.h\"/i\#include \"com.h\"" host_dec_stream.c
sed -i '/int host_dec_api/d' host_dec.h
sed -i '/#include \"com_reg.h\"/d' com.h
sed -i '/#define INT_MAX/d' com.h
