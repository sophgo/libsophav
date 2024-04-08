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

coda980EncRunner_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"

source ${coda980EncRunner_dir}/util_func.sh
source ${coda980EncRunner_dir}/common.sh

# Global varaibles for encoder options
build_stream=0
fail_stop=0
yuv_dir="./yuv"
stream_dir="./stream"
bin_dir="."
cfg_dir="./cfg"
                         
build_yuv=0
test_exec="./coda980_enc_test"
wiki_log_file="./log/enc_confluence.log"
enable_random=0
codec="avc"
jenkins=""
txt_param_switch=0
g_product_name="coda980"                      # This variable is used in common.sh
g_maptype_num=${#CODA980_MAPTYPE_NAME[@]}
field_flag=0


function generate_golden_stream {
    local src=$1
    local output=$2

    run_refc_enc_coda9 $src $output         # generate golden bitstream
    if [ $? -ne 0 ]; then 
        return 1
    fi

    return 0
}

# string get_stream_path(cfgfile)
function generate_golden_stream_path {
    local cfgfile=$1
    local rot_index=0

    codec_dir=""
    shopt -s nocasematch
    case "$codec" in
        avc) 
            if [ $g_enable_mvc -eq 0 ]; then
                codec_dir="AVC_ENC"
            else
                codec_dir="MVC_ENC"
            fi
            ;;
        mp4) codec_dir="MP4_ENC";;
        *)    echo ""; return;;
    esac
    shopt -u nocasematch

    if [ $g_rotAngle -eq 0 ] && [ $g_mirDir -eq 0 ]; then
        rot_index=0 
    fi
    if [ $g_rotAngle -eq 180 ] && [ $g_mirDir -eq 3 ]; then
        rot_index=0
    fi
    if [ $g_rotAngle -eq 90 ] && [ $g_mirDir -eq 0 ]; then
        rot_index=1
    fi
    if [ $g_rotAngle -eq 270 ] && [ $g_mirDir -eq 3 ]; then 
        rot_index=1
    fi
    if [ $g_rotAngle -eq 180 ] && [ $g_mirDir -eq 0 ]; then
        rot_index=2
    fi
    if [ $g_rotAngle -eq 0 ] && [ $g_mirDir -eq 3 ]; then 
        rot_index=2
    fi
    if [ $g_rotAngle -eq 270 ] && [ $g_mirDir -eq 0 ]; then
        rot_index=3
    fi
    if [ $g_rotAngle -eq 90 ] && [ $g_mirDir -eq 3 ]; then
        rot_index=3
    fi
    if [ $g_mirDir -eq 1 ] && [ $g_rotAngle -eq 180 ]; then
        rot_index=4
    fi
    if [ $g_rotAngle -eq 0 ] && [ $g_mirDir -eq 2 ]; then 
        rot_index=4
    fi
    if [ $g_mirDir -eq 1 ] && [ $g_rotAngle -eq 270 ]; then
        rot_index=5
    fi
    if [ $g_mirDir -eq 2 ] && [ $g_rotAngle -eq 90 ]; then
        rot_index=5
    fi
    if [ $g_mirDir -eq 1 ] && [ $g_rotAngle -eq 0 ]; then
        rot_index=6
    fi
    if [ $g_mirDir -eq 2 ] && [ $g_rotAngle -eq 180 ]; then
        rot_index=6
    fi
    if [ $g_mirDir -eq 1 ] && [ $g_rotAngle -eq 90 ]; then
        rot_index=7
    fi
    if [ $g_mirDir -eq 2 ] && [ $g_rotAngle -eq 270 ]; then
        rot_index=7
    fi

    streamset_name=$(basename $streamset_path)
    temp=`expr match "$streamset_name" '.*\(_[0-9]*\.cmd\)'`
    if [ "$temp" != "" ]; then
        streamset_name="${streamset_name%_[0-9]*\.cmd}.cmd"
    fi
    path="${stream_dir}/$codec_dir/$streamset_name"
    case $codec in
        avc) ext="264";;
        mp4) ext="mp4";;
        *)   ext="bin";;
    esac
    cfgname=$(basename $cfgfile)
    if [ ! -d "$path" ]; then
        mkdir -p "$path"
    fi

    path="$path/${cfgname}_$rot_index.$ext"

    g_func_ret_str=$path
}

function build_golden_streams {
    local src=$1
    local ppu_opts=(0 0 90 0 180 0 270 0 180 1 270 1 0 1 90 1)
    local indexes=(0 2 4 6 8 10 12 14)
    local golden_stream_path=""
    
    if [ $field_flag -eq 1 ]; then
        indexes=(0)
    fi

    echo "build_golden_streams cfg($src)"

    for i in ${indexes[@]}; do
        g_rotAngle=${ppu_opts[$i+0]}
        g_mirDir=${ppu_opts[$i+1]}
        echo "rotation: $g_rotAngle, mirror=$g_mirDir"
        generate_golden_stream_path $src
        golden_stream_path="$g_func_ret_str"
        echo "Encoding YUV : rotation($g_rotAngle) mirror($g_mirDir)..."
        generate_golden_stream $src $golden_stream_path
    done
}

function help {
    echo ""
    echo "-------------------------------------------------------------------------------"
    echo "Chips&Media conformance Tool v2.0"
    echo "All copyright reserved by Chips&Media"
    echo "-------------------------------------------------------------------------------"
    echo "$0 OPTION streamlist_file"
    echo "-h              help"
    echo "-c              codec. (avc, mp4)"
    echo "-n num          number of frames."
    echo "--bin-dir       ref-c directory, default: ./"
    echo "--enable-random [optional] generate random opton"
    echo "--ref-dir       stream directory"
    echo "--cfg-dir       base directory of encoder configuration files. default: ./cfg"
    echo "--yuv-dir       base directory storing YUV. default: ./yuv"
    echo "--fail-stop     Stop testing if current test is failure"
    echo "--build-stream  generate only golden streams at stream_dir."
#ifdef SUPPORT_MVC
    echo "--enable-mvc    enable MVC encoder, AVC only"
#endif /* SUPPORT_MVC */
}

OPTSTRING="-o hc:n: -l enable-random,ref-dir:,bin-dir:,cfg-dir:,yuv-dir:,match-mode:,enable-mvc,build-stream,fail-stop" 

OPTS=`getopt $OPTSTRING -- "$@"`

if [ $? -ne 0 ]; then
    exit 1
fi

eval set -- "$OPTS"

while true; do
    case "$1" in
        -h) 
            help
            exit 1
            shift;;
        -n) 
            g_frame_num="-n $2"
            shift 2;;
        -c)
            codec=$(echo $2 | tr '[:upper:]' '[:lower:]')
            shift 2;;
        --bin-dir)
            bin_dir=$2
            shift 2;;
        --enable-random)
            enable_random=1
            shift;;
        --match-mode)
            g_match_mode=$2
            shift 2;;
        --random_para_change)
            g_random_para_change=$2
            shift 2;;
        --ref-dir)
            stream_dir=$2
            shift 2;;
        --cfg-dir)
            cfg_dir=$2
            shift 2;;
        --yuv-dir)
            yuv_dir=$2
            shift 2;;
        --enable-mvc)
            g_enable_mvc=1
            shift;;
        --build-stream)
            build_stream=1;
            shift;;
        --fail-stop)
            fail_stop=1
            shift;;
        --) 
            shift
            break;;
    esac
done

shift $(($OPTIND - 1))

case "${codec}" in
    "avc")  g_codec_index=0;;
    "mp4")  g_codec_index=3;;
    *)
        echo "unsupported codec: ${codec}"
        help
        exit 1
        ;;
esac

shift $(($OPTIND - 1))

################################################################################
# Get param from target text file                                              #
################################################################################
name_value=()
input_param_name="TestRunnerParamCoda980Enc.txt"
if [ -f $input_param_name ] && [ $enable_random -eq 0 ]; then
    while read line; do
        # remove right comment
        line="${line%%\#*}"
        if [ "$line" = "" ]; then continue; fi
        if [[ "$line" = "//"* ]]; then continue; fi

        OIFS=$IFS
        IFS='='
        count=0
        for word in $line; do
            word=${word// /}
            name_value[$count]="$word"
            count=$(($count+1))
        done
        IFS=$OIFS
        attr="${name_value[0]}"
        value="${name_value[1]}"

        case "$attr" in
            default)              default_opt="$value";;
            n)                    g_frame_num="-n $value";;
            secondary-axi)        secondary_axi_temp="$value";;
            stream-endian)        stream_endian_temp="$value";;
            frame-endian)         frame_endian_temp="$value";;                  
            bsmode)               bsmode_temp="$value";;
            cbcr_interleave_mode) yuv_mode_temp="$value";;
            maptype)              maptype_temp="$value";;
            enable_linear2tiled)  linear2tiled_temp="$value";;
            rotAngle)             rotation_temp="$value";;
            mirDir)               mirror_temp="$value";;
            linebuf_int)          linebuf_int_temp="$value";;
            txt-param)            txt_param_switch=1;;
            *) ;;
        esac
    done < $input_param_name
else
    if [ ! -f $input_param_name ]; then
        echo "$input_param_name file doesn't exist"; 
    fi
fi

streamset_path=$1
streamset_file=`basename ${streamset_path}`

if [ ! -e "${streamset_path}" ]; then
    echo "No such file: ${streamset_path}"
    exit 1
fi

################################################################################
# count stream number                                                          #
################################################################################
stream_file_array=()
parse_streamset_file $streamset_path stream_file_array

num_of_streams=${#stream_file_array[@]}

#echo ${stream_file_array[@]}

count=1
g_success_count=0
g_failure_count=0
g_remain_count=${num_of_streams}

mkdir -p temp
mkdir -p output
mkdir -p log/encoder_conformance
conf_log_path="log/encoder_conformance/${streamset_file}_r${g_revision}.log"
conf_err_log_path="log/encoder_conformance/${streamset_file}_error_r${g_revision}.log"
temp_log_path="./temp/temp.log"
# truncate contents of log file
echo "" > $conf_log_path
echo "" > $conf_err_log_path
echo "" > $temp_log_path
beginTime=$(date +%s%N)

echo ""
echo ""
echo "##### PRESS 'q' for stop #####"
echo ""
echo ""

log_conf "STREAM BASE DIR: ${stream_dir}" ${conf_log_path}

################################################################################
# read cmd file.                                                               #
################################################################################
for (( c=0; c< $num_of_streams ; c++ )) do
    line=${stream_file_array[$c]}
    # press 'q' for stop
    read -t 1 -n 1 key
    if [[ $key == q ]]; then
        break;
    fi

    test_param=""
    cfg="${cfg_dir}/${line}"
    log_conf "--------------------------------------------------------------------------------" ${temp_log_path}
    log_conf "[${count}/${num_of_streams}] ${cfg}" ${temp_log_path}
    log_conf "--------------------------------------------------------------------------------" ${temp_log_path}

    if [ ! -f $cfg ]; then
        log_conf "Not found $cfg" $conf_log_path
        log_conf "[RESULT] FAILURE" $conf_log_path
        echo "Not found $stream" >> $conf_err_log_path
        echo "[RESULT] FAILURE" >> $conf_err_log_path
        cat $temp_log_path >> $conf_log_path
        g_failure_count=$(($g_failure_count + 1))
        g_remain_count=$(($g_remain_count - 1))
        count=$(($count + 1))
        continue
    fi

    c9_parse_cfg $cfg width height field_flag 

    if [ $build_stream -eq 1 ]; then
        build_golden_streams $cfg
        g_remain_count=$(($g_remain_count - 1))
        count=$(($count + 1))
        continue
    fi

################################################################################
# make argc & argv parameter                                                   #
################################################################################
    if [ $enable_random -eq 1 ] && [ $txt_param_switch -eq 0 ]; then
        get_random_param 0 coda9
    else
        get_default_param 0 coda9 0
    fi


    ### CODA9 CONSTRAINTS ###
    if [ $field_flag -eq 1 ]; then
        echo "FIELD PICTURE!!!"
        g_rotAngle=0
        g_mirDir=0
    fi

    if [ $g_rotAngle -eq 90 ] || [ $g_rotAngle -eq 270 ]; then
        if [ $height -lt 96 ]; then
            g_rotAngle=0; 
        fi
        if [ $width -gt 2304 ]; then
            g_rotAngle=0; 
        fi
    fi


    build_test_param 0 coda9
    test_param="$test_param $g_func_ret_str"

    result=0
    if [ $g_match_mode -eq 1 ]; then
        generate_golden_stream_path $cfg
        output="$g_func_ret_str"
        test_param="${test_param} -c $g_match_mode --ref_stream_path=$output"
    fi

    if [ $result -eq 0 ]; then
        log_conf "RANDOM TEST    : ${ON_OFF[$enable_random]}" $temp_log_path
        log_conf "BITSTREAM MODE : ${ENC_BSMODE_NAME[$g_bsmode]}" $temp_log_path
        if [ $g_bsmode -eq 1 ]; then
        log_conf "LINEBUFFER INT : ${ON_OFF[$g_linebuf_int]}" $temp_log_path
        fi
        log_conf "ENDIAN         : STREAM($g_stream_endian) FRAME($g_frame_endian)" $temp_log_path
        log_conf "STANDARD       : ${CODEC_NAME[$g_codec_index]}" $temp_log_path
        log_conf "MAP TYPE       : ${CODA980_MAPTYPE_NAME[$g_maptype_index]}" $temp_log_path
        log_conf "LINEAR2TILED   : ${ON_OFF[$g_enable_linear2tiled]}" $temp_log_path
        log_conf "ROTATE         : $g_rotAngle" $temp_log_path
        log_conf "MIRROR         : ${MIRROR_NAME[$g_mirDir]}" $temp_log_path
        log_conf "CBCR interleave: ${ON_OFF[$g_cbcr_interleave]}" $temp_log_path
        log_conf "NV21           : ${ON_OFF[$g_enable_nv21]}" $temp_log_path
        log_conf "Secondary AXI  : ${g_secondary_axi}" $temp_log_path

        test_param="$test_param --codec=$g_codec_index" 
        if [ $g_enable_mvc -eq 1 ]; then
            test_param="$test_param --enable-mvc"
        fi
        test_param="$test_param --cfg-path=$cfg --yuv-base=$yuv_dir"
        test_param_print=$test_param
        log_conf "Unix   : $test_exec $test_param_print" $temp_log_path
        #winexec=$(echo "$test_exec $test_param_print" | sed -e 's/\//\\/g')
        #log_conf "Windows: $winexec" $temp_log_path
        cat $temp_log_path >> $conf_log_path

        chmod 777 $test_exec
        test_exec_param="$test_exec $test_param"
        $test_exec_param || result=$?

        #"gdb $test_exec"
    else
        cat $temp_log_path >> $conf_log_path
    fi

    if [ $result -eq 0 ]; then
        log_conf "[RESULT] SUCCESS" $conf_log_path
        g_success_count=$(($g_success_count + 1))
    else
        cat ./ErrorLog.txt >> $conf_log_path
        log_conf "[RESULT] FAILURE" $conf_log_path
        g_failure_count=$(($g_failure_count + 1))
        cat $temp_log_path >> $conf_err_log_path
        cat ./ErrorLog.txt >> $conf_err_log_path
        echo "[RESULT] FAILURE" >> $conf_err_log_path
        if [ $result -eq 10 ]; then
            echo "Abnormal exit!!!"
            break
        fi
    fi
    g_remain_count=$(($g_remain_count - 1))
    count=$(($count + 1))
    # clear temp log
    echo "" > $temp_log_path
    
    if [ $fail_stop -eq 1 ] && [ $result -ne 0 ]; then
        break;
    fi
done 

if [ $build_yuv -eq 1 ]; then
    exit 0
fi

endTime=$(date +%s%N)
elapsed=$((($endTime - $beginTime) / 1000000000))
elapsedH=$(($elapsed / 3600))
elapsedS=$(($elapsed % 60))
elapsedM=$(((($elapsed - $elapsedS) / 60) % 60))
if [ "$((elapsedS / 10))" == "0" ]; then elapsedS="0${elapsedS}" ;fi
if [ "$((elapsedM / 10))" == "0" ]; then elapsedM="0${elapsedM}" ;fi
if [ "$((elapsedH / 10))" == "0" ]; then elapsedH="0${elapsedH}" ;fi

if [ $elapsed -le 60 ]; then
    time_hms="{color:red}*${elapsedH}:${elapsedM}:${elapsedS}*{color}"
else
    time_hms="${elapsedH}:${elapsedM}:${elapsedS}"
fi

log_filename=$(basename $conf_log_path)
log_err_filename=$(basename $conf_err_log_path)
if [ $g_failure_count == 0 ] && [ $num_of_streams != 0 ]; then
    pass=${PASS}
    rm $conf_err_log_path
    log_err_filename=""
else
    pass=${FAIL}
fi

wiki_log="| $streamset_file | $num_of_streams | $g_success_count | $g_failure_count | $g_remain_count | $log_filename | ${log_err_filename} | $pass | $time_hms | | | |"

echo $wiki_log
echo $wiki_log >> $wiki_log_file

if [ "$num_of_streams" == "0" ]; then
    echo "num_of_streams: $num_of_streams = exit 1"
    exit 1
fi

exit $g_failure_count
 
