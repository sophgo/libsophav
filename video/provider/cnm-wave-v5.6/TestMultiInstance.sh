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

multiRunner_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"

source ${multiRunner_dir}/util_func.sh
source ${multiRunner_dir}/common.sh


g_product_name=""
g_frame_num_param=""
enc_prebuilt=""
enable_wtl=1
enable_sync=0
enable_afbce=0
enable_afbcd=0
enable_scaler=0
enable_cframe50d=0
multi_random_cfg=0
enable_irb=0
haps=0
vu440=0
process_test="true"
test_exec="./multi_instance_test"
wiki_log_file="./log/multi_confluence.log"
yuv_base="yuv/"
bin_dir="."
multi_customer=""
bitstream_buf_size=0 #in byte
simulation="false"
check_scaler_test="false"
enable_multi_sfs=0
multi_sfs_count=0
bitfile=""
first_sfs=0
multi_random_stream=0
cfg_base_path=""
dec_pvric_test=""
enc_pvric_test=0
g_pvric_dec_temp=0
g_forced_10b_temp=0
g_last_2bit_in_8to10_temp=0
g_csc_enable_temp=0
g_csc_ayuv_temp=0
g_csc_rgb_order_temp=0
enable_cframe50sd=""
enable_sfs=0
enable_rrb=0
enable_skipVLC=0
numofFrames=0
enable_mcts=0
scale_width=0
scale_height=0
num=0


#====================================================================#
# enc multi test
#====================================================================#
declare -A g_multi_enc_cmd_vars
readonly multi_enc_cmd_keys=(
    stdIdx
    cfgPath
    refPath
    isEncoder
    is10B
    srcFormat
)

function parse_multi_cmd
{
    local cmd_line=$1
    local OLDIFS=$IFS                        
    local val
    local idx

    for key in ${multi_enc_cmd_keys[@]};
    do
        g_multi_enc_cmd_vars[${key}]="None"
    done

    IFS=$' '
    idx=0
    for val in $cmd_line;
    do 
        g_multi_enc_cmd_vars[${multi_enc_cmd_keys[${idx}]}]="$val"
        ((idx++))
    done
    IFS=$OLDIFS
}


readonly g_cfg_param_name=(
    FramesToBeEncoded
    InputFile
    SourceWidth
    SourceHeight
    ScalingListFile
    EnModeMap
    ModeMapFile
    EnLambdaMap
    LambdaMapFile
    EnCustomLambda
    CustomLambdaFile
    InputBitDepth
    InternalBitDepth
    EnSVC
    EnRoi
    RoiFile
    WeightedPred
    BitstreamFile
    EnBgDetect
    RateControl
    EnHvsQp
    EnNoiseEst
    CULevelRateControl
    IntraCtuRefreshMode
    WaveFrontSynchro
    EncBitrate
    LookAheadRcEnable
    UseAsLongTermRefPeriod
    S2SearchCenterEnable
    S2SearchCenterYdiv4
    S2SearchRangeYdiv4
    GopPreset
    GOPSize
    Frame1
    DeSliceMode
    IdrPeriod
    IntraPeriod
    EncMonochrome
    AdaptiveRound
    CustomMapFile
)

function help {
    echo ""
    echo "-------------------------------------------------------------------------------"
    echo "Chips&Media conformance Tool v2.0"
    echo "All copyright reserved by Chips&Media"
    echo "-------------------------------------------------------------------------------"
    echo "$0 OPTION streamlist_file"
    echo "-h                    Help"
    echo "--product             product name(coda960, coda980, wave521, wave521c, wave521c_dual, wave517)"
    echo "-n                    the number of display frames, default 0 : decoding until the end"
}

function minst_init_g_cfg_param() {
    init_cfg_param ${g_cfg_param_name[@]}
    g_cfg_param[BitstreamFile]=""
    g_cfg_param[AdaptiveRound]=1
    g_cfg_param[CustomMapFile]=1
}
#------------------------------------------------------------------------------------------------------------------------------
# Main process routine
#------------------------------------------------------------------------------------------------------------------------------
################################################################################
# Parse arguments                                                              #
################################################################################
OPTSTRING="-o hn: -l product:"
OPTSTRING="$OPTSTRING,customer:,bs-size:,sim"
OPTSTRING="${OPTSTRING},scaler-test-mode:,scaler-step-cnt:,ori-width:,enable-scaler,ori-height:,scale-width:,scale-height:"
OPTSTRING="${OPTSTRING},csc-enable:,csc-ayuv:,forced-10b:,last-2bit-in-8to10:,csc-rgb-order:"
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
            g_frame_num_param="-n $2"
            g_refc_frame_num="-m $2"
            shift 2;;
        --product)
            g_product_name=$2
            shift 2;;
        --customer)
            multi_customer=$2
            shift 2;;
        --bs-size)
            bitstream_buf_size=$2
            shift 2;;
        --sim)
            simulation="true"
            shift;;
#ifdef CNM_FPGA_PLATFROM
        --bitfile)
            bitfile=$2
            shift 2;;
        --haps)
            haps=$2
            shift 2;;
        --vu440)
            vu440=$2
            shift 2;;
#endif /* CNM_FPGA_PLATFORM */
        --enable-scaler)
            enable_scaler=1
            shift;;
        --scaler-test-mode)
            g_scaler="true"
            scaler_test_mode=$2
            shift 2;;
        --scaler-step-cnt)
            g_scaler="true"
            scaler_step_cnt=$2
            shift 2;;
        --ori-width)
            ori_width=$2
            shift 2;;
        --ori-height)
            ori_height=$2
            shift 2;;
        --forced-10b)
            g_forced_10b_temp=$2
            shift 2;;
        --last-2bit-in-8to10)
            g_last_2bit_in_8to10_temp=$2
            shift 2;;
        --csc-enable)
            g_csc_enable_temp=$2
            shift 2;;
        --csc-ayuv)
            g_csc_ayuv_temp=$2
            shift 2;;
        --csc-rgb-order)
            g_csc_rgb_order_temp=$2
            shift 2;;
        --)
            shift
            break;;
    esac
done

shift $(($OPTIND - 1))


streamset_path=$1
streamset_file=`basename ${streamset_path}`

if [ ! -f "${streamset_path}" ]; then
    echo "No such file: ${streamset_path}"
    exit 1
fi




################################################################################
# count stream number                                                          #
################################################################################
stream_command_array=()
linenum=0


while read -r line || [ -n "$line" ]; do
    line=$(echo $line | tr -d '\n')
    line=$(echo $line | tr -d '\r')
    line="${line//\\//}" # replace backslash to slash
    line=${line#*( )}    # remove front whitespace
    firstchar=${line:0:1}
    case "$firstchar" in
        "@") break;;        # exit flag
        "#") continue;;     # comment
        ";") continue;;     # comment
        "")  continue;;     # comment
        *)
    esac

    stream_command_array[$linenum]="$line"
    linenum=$(($linenum + 1))
done < ${streamset_path}

num_of_streams=${#stream_command_array[@]}

g_success_count=0
g_failure_count=0
g_remain_count=2          # process test + thread test = 2
if [ "$process_test" == "false" ]; then
    g_remain_count=1      # only thread test
fi

mkdir -p temp
mkdir -p output
mkdir -p log/multi_conformance
conf_log_path="log/multi_conformance/${streamset_file}.log"
conf_err_log_path="log/multi_conformance/${streamset_file}_error.log"
temp_log_path="./temp/temp.log"
# truncate contents of log file
echo "" > $conf_log_path
echo "" > $conf_err_log_path
echo "" > $temp_log_path
beginTime=$(date +%s%N)


################################################################################
# Multi random stream for decoder                                              #
################################################################################
if [ "$multi_random_stream" = "1" ]; then
    dest=()
    echo "================================================="
    echo "[INFO] Multi random stream test"
    max_instance_num=8
    rand_instance_num=$(get_random_min_max 2 $max_instance_num)
    echo "rand instance num : $rand_instance_num"

    #delete array
    range_num=${#stream_command_array[@]}
    while [ ${#stream_command_array[@]} != $rand_instance_num ]
    do
        rand=$[ $RANDOM % $range_num ]
        if [ ! -z ${stream_command_array[$rand]+x} ];then
            unset stream_command_array[$rand]
        fi
    done

    cnt=0
    for ((idx=0;idx<$range_num;idx++)) ; do
        if [ ! -z ${stream_command_array[$idx]+x} ];then
            dest[$cnt]="$idx"
            cnt=$(($cnt+1))
        fi
    done
    #shuffle streams
    dest=($(shuf -e "${dest[@]}"))

    echo "${dest[@]}"
    num_of_streams=${#stream_command_array[@]}
    echo "num of stream : $num_of_streams"
    for ((idx=0;idx<${#dest[@]};idx++)) do
        echo "[$idx] CMD : ${stream_command_array[${dest[$idx]}]}"
        stream_command_array[$idx]=${stream_command_array[${dest[$idx]}]}
    done
    echo "================================================="
fi

################################################################################
# Read CMD File && Make test param                                             #
################################################################################
for ((i=0;i<$num_of_streams;i++)) do
    ################################################################################
    # SET DEFAULT VALUE                                                            #
    ################################################################################
    minst_init_g_cfg_param
    pvric[$i]=""
    scaler[$i]=0
    wtl[$i]=1
    scaleWidth[$i]=0
    scaleHeight[$i]=0
    codec_std[$i]=$(echo ${stream_command_array[$i]} | cut -d' ' -f1)
    stream[$i]=$(echo ${stream_command_array[$i]} | cut -d' ' -f2)
    ref_file_path[$i]=$(echo ${stream_command_array[$i]} | cut -d' ' -f3)
    isEncoder[$i]=$(echo ${stream_command_array[$i]} | cut -d' ' -f4)
    main10[$i]=$(echo ${stream_command_array[$i]} | cut -d' ' -f5)

    if (( ${isEncoder[$i]} == 1 )); then
        parse_multi_cmd "${stream_command_array[$i]}"
        echo ">>>>>>>>>> [$i] encoder multi cmd file info <<<<<<<<<<"
        for key in ${!g_multi_enc_cmd_vars[@]};
        do
            echo "g_multi_enc_cmd_vars[${key}] : ${g_multi_enc_cmd_vars[${key}]}"
        done
    fi

    # default settings
    wtl_format[$i]=0
    if [ "${g_product_name}" = "coda960" ] || [ "${g_product_name}" = "coda980" ]; then
        stream_endian[$i]=0 #VDI_LITTLE_ENDIAN
        frame_endian[$i]=0
        source_endian[$i]=0
    else # WAVE series
        stream_endian[$i]=16 #VDI_128BIT_LITTLE_ENDIAN
        frame_endian[$i]=16
        source_endian[$i]=31
        if [[ ${g_product_name} == *"wave6"* ]] ; then
            custom_map_endian[$i]=16
        fi
    fi

    if (( ${isEncoder[$i]} == 0 )); then
        wtl_format[$i]=$(echo ${stream_command_array[$i]} | cut -d' ' -f6)
        stream_endian[$i]=$(echo ${stream_command_array[$i]} | cut -d' ' -f7)
        frame_endian[$i]=$(echo ${stream_command_array[$i]} | cut -d' ' -f8)
        scaler[$i]=$(echo ${stream_command_array[$i]} | cut -d' ' -f9)
        scaleWidth[$i]=$(echo ${stream_command_array[$i]} | cut -d' ' -f10)
        scaleHeight[$i]=$(echo ${stream_command_array[$i]} | cut -d' ' -f11)
    fi
    match_mode[$i]=1

    if (( ${isEncoder[$i]} == 0 )); then
        echo "---------------------- INFO ----------------------"
        echo "wtl-format : ${wtl_format[$i]}"
        echo "stream-endian : ${stream_endian[$i]} frame_endian : ${frame_endian[$i]}, source_endian :${source_endian[$i]}"
        echo "enable-scaler : ${scaler[$i]}"
        echo "scale-width : ${scaleWidth[$i]} scale-height : ${scaleHeight[$i]}"
    fi

    afbce[$i]=0
    afbcd[$i]=0
    cframe50d[$i]=0
    cframe50d_mbl[$i]=0
    cframe50d_422[$i]=0
    bsmode[$i]=0
    enable_mvc[$i]=0
    core[$i]=0
    ring[$i]=0
    refRingBufMode[$i]=0
    maxVerticalMV[$i]=0
    skipVLC[$i]=0

    case "${codec_std[$i]}" in
        "0")  codec_name[$i]="avc";;
        "1")  codec_name[$i]="vc1";;
        "2")  codec_name[$i]="mp2";;
        "3")  codec_name[$i]="mp4";;
        "4")  codec_name[$i]="h263";;
        "5")  codec_name[$i]="dv3";;
        "6")  codec_name[$i]="rvx";;
        "7")  codec_name[$i]="avs";;
        "9")  codec_name[$i]="tho";;
        "10") codec_name[$i]="vp3";;
        "11") codec_name[$i]="vp8";;
        "12") codec_name[$i]="hevc";;
        "13") codec_name[$i]="vp9";;
        "14") codec_name[$i]="avs2";;
        "15") codec_name[$i]="svac";;
        "16") codec_name[$i]="av1";;
        *)
            echo "unsupported codec_std: ${codec_std[$i]}"
            help
            exit 1
            ;;
    esac
    
    case "${codec_std[$i]}" in
        "0")  g_refc_codec=3;;
        "12") g_refc_codec=1;;
        "16") g_refc_codec=27;;
        *) ;;
    esac

    if [ "${isEncoder[$i]}" = "1" ]; then
        minst_init_g_cfg_param
        read_cfg ${stream[$i]} ${g_cfg_param_name[@]}
        output[$i]="instance_${i}_output.bin"
    else
        if [ "$dec_pvric_test" == "true" ]; then
            output[$i]="instance_${i}_output.bin"
        else
            output[$i]=""
        fi
    fi

    srcFormat[$i]=0
    if (( ${isEncoder[$i]} == 1 )); then
        if [ "${g_multi_enc_cmd_vars[srcFormat]}" == "None" ];then
            echo "default source format is 0"
        else
            srcFormat[$i]=${g_multi_enc_cmd_vars[srcFormat]}
        fi
        srcFormat_temp=${srcFormat[$i]}
    fi

    ################################################################################
    # SET RANDOM VALUE                                                             #
    ################################################################################
    str=`echo ${stream[$i]} | grep "_MVC_\|MVCDS\|MVCICT\|MVCRP\|MVCSPS"` || echo ignore_error
    if [ "${str}" != "" ]; then
        enable_mvc[$i]=1
    fi

    if [ "${codec_name[$i]}" = "vp9" ] || [ "${codec_name[$i]}" = "dv3" ] || [ "${codec_name[$i]}" = "tho" ]; then
        bsmode[$i]=2
        ext=${stream[$i]##*.}
        if [ "$ext" = "ivf" ] && [ "${codec_name[$i]}" != "vp9" ]; then
            bsmode[$i]=0
        fi
    fi

    if [ "$enable_wtl" = "-99" ]; then
        wtl[$i]=$(get_random 0 1)
    elif [ "$enable_wtl" = "0" ]; then
        wtl[$i]=0
    fi

    if [ "${isEncoder[$i]}" = "1" ]; then
        match_mode[$i]=3 # bitstream compare
        wtl[$i]=0
    fi

    if [ "${g_product_name}" = "coda960" ] || [ "${g_product_name}" = "coda980" ]; then
        wtl[$i]=0
    fi

    if (( ${match_mode[$i]} == 1 )); then
        wtl[$i]=1
    fi
    if [ "$enable_sfs" = "1" ] && [ "${isEncoder[$i]}" = "1" ] && [ "${cframe50sd[$i]}" == "0" ]; then
        sfs[$i]=$(get_random 0 1)
        if [ "$first_sfs" == "1" ]; then #support only one sfs in multi-instnace
            sfs[$i]=0
        fi
        if [ "${sfs[$i]}" == "1" ]; then
            first_sfs=1
        fi
    else
        irb[$i]=0
        sfs[$i]=0
    fi

    if [ "$enable_multi_sfs" == "1" ] && [ "${isEncoder[$i]}" == "1" ]; then
        sfs[$i]=1
        if [ "$multi_sfs_count" == "4" ]; then #support only four sfs in multi-instnace
            sfs[$i]=0
        fi
        if [ "${sfs[$i]}" == "1" ]; then
            multi_sfs_count=$(($multi_sfs_count + 1))
        fi
    else
        sfs[$i]=0
    fi




    if [ "${g_product_name}" = "wave521c_dual" ]; then
        core[$i]=$(get_random 1 2)
        if [ "$enable_ring" == "1" ]; then
            ring[$i]=$(get_random 0 1)
            if [ ${ring[$i]} == 1 ]; then
                ring[$i]=2
            fi
        else
            ring[$i]=0
        fi
    elif [ "${g_product_name}" = "wave537" ]; then
        core[$i]=$(get_random 1 2)
    elif [ "${g_product_name}" = "wave663" ] || [ "${g_product_name}" = "wave677" ]; then
        core[$i]=$(get_random 1 2)
    fi

    forced_10b[$i]=0
    last_2bit_in_8to10[$i]=0
    if (( ${g_cfg_param[InputBitDepth]} == 8 )) && (( ${g_cfg_param[InternalBitDepth]} == 8 )); then
        forced_10b[$i]=$(gen_val 0 1 $g_forced_10b_temp)
        if (( ${forced_10b[$i]} == 1 )); then
            g_cfg_param[InternalBitDepth]=10
            last_2bit_in_8to10[$i]=$(gen_val 0 3 $g_last_2bit_in_8to10_temp)
        fi
    fi
    csc_enable[$i]=0
    csc_rgb_order[$i]=0
    csc_ayuv[$i]=0
    csc_srcformat[$i]=0
    enc_csc_ry_coef[$i]=0
    enc_csc_gy_coef[$i]=0
    enc_csc_by_coef[$i]=0
    enc_csc_rcb_coef[$i]=0
    enc_csc_gcb_coef[$i]=0
    enc_csc_bcb_coef[$i]=0
    enc_csc_rcr_coef[$i]=0
    enc_csc_gcr_coef[$i]=0
    enc_csc_bcr_coef[$i]=0
    enc_csc_y_offset[$i]=0
    enc_csc_cb_offset[$i]=0
    enc_csc_cr_offset[$i]=0

    if [[ "${isEncoder[$i]}" == "1" ]] && [[ "${g_product_name}" = *"wave6"* ]]; then
        csc_enable[$i]=$(gen_val 0 1 $g_csc_enable_temp)
        if (( ${csc_enable[$i]} == 1 )); then
            csc_ayuv[$i]=$(gen_val 0 1 $g_csc_ayuv_temp)

            csc_rgb_order[$i]=$(gen_val 0 13 $g_csc_rgb_order_temp)
            if (( ${csc_rgb_order[$i]} == 6 )); then
                 csc_rgb_order[$i]=$(get_random 0 5)
            elif (( ${csc_rgb_order[$i]} == 7 )); then
                 csc_rgb_order[$i]=$(get_random 8 13)
            fi
        fi
    fi
done

################################################################################
# Generate compare file path                                                   #
################################################################################
for ((i=0;i<$num_of_streams;i++)) do
    if [ "${isEncoder[$i]}" = "1" ]; then
        echo "[A] path -> ${ref_file_path[$i]}"
    else
        codec=${codec_name[$i]}
        g_wtl_format=${wtl_format[$i]}
        is_main10=${main10[$i]}
        echo "[INFO] CODEC : ${codec_name[$i]}, WTL_FORMAT : ${g_yuv_fmt_list[$g_wtl_format]}"

        g_codec_index=${codec_std[$i]}
        g_bsmode=${bsmode[$i]}
        if [ "${scaler[$i]}" = "1" ]; then
            if [ "$g_product_name" == "wave512" ]; then
                if [ "${codec_name[$i]}" = "vp9" ]; then
                    bin_dir="../../../design/ref_c_vp9/bin/Linux"
                elif [ "${codec_name[$i]}" = "hevc" ]; then
                    bin_dir="../../../design/ref_c_hevc/bin/Linux"
                    g_support_minipippen=1
                fi
            else
                g_support_minipippen=1
            fi
            g_scaler="true"
            g_sclw=${scaleWidth[$i]}
            g_sclh=${scaleHeight[$i]}
        else
            g_scaler="false"
            g_sclw=0
            g_sclh=0
        fi
        if [ "${match_mode[$i]}" == "1" ]; then
            generate_yuv ${stream[$i]} ${ref_file_path[$i]}
        elif [ "${match_mode[$i]}" == "3" ]; then
            golden_yuv_path="${ref_file_path[$i]}"
            generate_yuv ${stream[$i]} $golden_yuv_path
        fi
    fi
done

echo 444
################################################################################
# Update irb and rrb(need height info after random cfg)
################################################################################
for ((i=0;i<$num_of_streams;i++)) do
    # SET DEFAULT VALUE                                                            #
    if [ "${isEncoder[$i]}" = "1" ]; then
        minst_init_g_cfg_param
        read_cfg ${stream[${i}]} ${g_cfg_param_name[@]}
    fi


done



################################################################################
# Add test param                                                               #
# Options of the each instance are seperated by ","                            #
################################################################################
test_param=""
output_test_param=""
for ((i=0;i<$num_of_streams;i++)) do
    if [ "$i" = "0" ]; then
        codec_std_test_param="${codec_std[$i]}"
        stream_test_param="${stream[$i]}"
        if [ "${output[$i]}" != "" ]; then
            output_test_param="${output[$i]}"
        fi
        wtl_test_param="${wtl[$i]}"
        wtl_format_param="${wtl_format[$i]}"

        if [[ ${g_product_name} == *"coda"* ]] ; then
            stream_endian_param="${stream_endian[$i]}"
            frame_endian_param="${frame_endian[$i]}"
        else
            stream_endian_param="$( get_random 16 31 )"
            frame_endian_param="$( get_random 16 31 )"
        fi

        if [ $enc_pvric_test -eq 1 ] || [[ ${g_product_name} == *"coda"* ]] ; then
            source_endian_param="${source_endian[$i]}"
        else
            source_endian_param="$( get_random 16 31 )"
        fi
        if [[ ${g_product_name} == *"wave6"* ]] ; then
            custom_map_endian_param="$( get_random 16 31 )"
        fi
        bsmode_test_param="${bsmode[$i]}"
        match_mode_test_param="${match_mode[$i]}"
        ref_file_path_test_param="${ref_file_path[$i]}"
        enc_dec_param="${isEncoder[$i]}"
        core_param="${core[$i]}"
        ring_param="${ring[$i]}"
        sfs_test_param="${sfs[$i]}"
        scaler_test_param="${scaler[$i]}"
        scaleWidth_test_param="${scaleWidth[$i]}"
        scaleHeight_test_param="${scaleHeight[$i]}"
        csc_enable_param="${csc_enable[$i]}"
        forced_10b_param="${forced_10b[$i]}"
        last_2bit_in_8to10_param="${last_2bit_in_8to10[$i]}"
        csc_rgb_order_param="${csc_rgb_order[$i]}"
        csc_srcformat_param="${csc_srcformat[$i]}"
        enc_csc_ry_coef_param="${enc_csc_ry_coef[$i]}"
        enc_csc_gy_coef_param="${enc_csc_gy_coef[$i]}"
        enc_csc_by_coef_param="${enc_csc_by_coef[$i]=}"
        enc_csc_rcb_coef_param="${enc_csc_rcb_coef[$i]}"
        enc_csc_gcb_coef_param="${enc_csc_gcb_coef[$i]}"
        enc_csc_bcb_coef_param="${enc_csc_bcb_coef[$i]}"
        enc_csc_rcr_coef_param="${enc_csc_rcr_coef[$i]}"
        enc_csc_gcr_coef_param="${enc_csc_gcr_coef[$i]}"
        enc_csc_bcr_coef_param="${enc_csc_bcr_coef[$i]}"
        enc_csc_y_offset_param="${enc_csc_y_offset[$i]}"
        enc_csc_cb_offset_param="${enc_csc_cb_offset[$i]}"
        enc_csc_cr_offset_param="${enc_csc_cr_offset[$i]}"
    else
        codec_std_test_param="${codec_std_test_param},${codec_std[$i]}"
        stream_test_param="${stream_test_param},${stream[$i]}"
        if [ "${output[$i]}" != "" ]; then
            output_test_param="${output_test_param},${output[$i]}"
        fi
        wtl_test_param="${wtl_test_param},${wtl[$i]}"
        wtl_format_param="${wtl_format_param},${wtl_format[$i]}"

        if [[ ${g_product_name} == *"coda"* ]] ; then
            stream_endian_param="${stream_endian_param},${stream_endian_param}"
            frame_endian_param="${frame_endian_param},${frame_endian_param}"
        else
            stream_endian_param="${stream_endian_param},$( get_random 16 31 )"
            frame_endian_param="${frame_endian_param},$( get_random 16 31 )"
        fi

        if [ $enc_pvric_test -eq 1 ] || [[ ${g_product_name} == *"coda"* ]] ; then
            source_endian_param="${source_endian_param},${source_endian[$i]}"
        else
            source_endian_param="${source_endian_param},$( get_random 16 31 )"
        fi

        if [[ ${g_product_name} == *"wave6"* ]] ; then
            custom_map_endian_param="${custom_map_endian_param},$( get_random 16 31 )"
        fi
        bsmode_test_param="${bsmode_test_param},${bsmode[$i]}"
        match_mode_test_param="${match_mode_test_param},${match_mode[$i]}"
        ref_file_path_test_param="${ref_file_path_test_param},${ref_file_path[$i]}"
        enc_dec_param="${enc_dec_param},${isEncoder[$i]}"
        core_param="${core_param},${core[$i]}"
        ring_param="${ring_param},${ring[$i]}"
        sfs_test_param="${sfs_test_param},${sfs[$i]}"
        scaler_test_param="${scaler_test_param},${scaler[$i]}"
        scaleWidth_test_param="${scaleWidth_test_param},${scaleWidth[$i]}"
        scaleHeight_test_param="${scaleHeight_test_param},${scaleHeight[$i]}"
        csc_enable_param="${csc_enable_param},${csc_enable[$i]}"        
        forced_10b_param="${forced_10b_param},${forced_10b[$i]}"
        last_2bit_in_8to10_param="${last_2bit_in_8to10_param},${last_2bit_in_8to10[$i]}"
        csc_rgb_order_param="${csc_rgb_order_param},${csc_rgb_order[$i]}"
        csc_srcformat_param="${csc_srcformat_param},${csc_srcformat[$i]}"
        enc_csc_ry_coef_param="${enc_csc_ry_coef_param},${enc_csc_ry_coef[$i]}"
        enc_csc_gy_coef_param="${enc_csc_gy_coef_param},${enc_csc_gy_coef[$i]}"
        enc_csc_by_coef_param="${enc_csc_by_coef_param},${enc_csc_by_coef[$i]=}"
        enc_csc_rcb_coef_param="${enc_csc_rcb_coef_param},${enc_csc_rcb_coef[$i]}"
        enc_csc_gcb_coef_param="${enc_csc_gcb_coef_param},${enc_csc_gcb_coef[$i]}"
        enc_csc_bcb_coef_param="${enc_csc_bcb_coef_param},${enc_csc_bcb_coef[$i]}"
        enc_csc_rcr_coef_param="${enc_csc_rcr_coef_param},${enc_csc_rcr_coef[$i]}"
        enc_csc_gcr_coef_param="${enc_csc_gcr_coef_param},${enc_csc_gcr_coef[$i]}"
        enc_csc_bcr_coef_param="${enc_csc_bcr_coef_param},${enc_csc_bcr_coef[$i]}"
        enc_csc_y_offset_param="${enc_csc_y_offset_param},${enc_csc_y_offset[$i]}"
        enc_csc_cb_offset_param="${enc_csc_cb_offset_param},${enc_csc_cb_offset[$i]}"
        enc_csc_cr_offset_param="${enc_csc_cr_offset_param},${enc_csc_cr_offset[$i]}"
    fi
    if [ "${scaler[$i]}" = "1" ]; then
        check_scaler_test="true"
    fi
done

################################################################################
# make test param                                                              #
################################################################################
output_param=""
if [ "${output_test_param}" != "" ]; then
    output_param="--output=${output_test_param}"
fi

test_param="--instance-num=${num_of_streams} -c ${match_mode_test_param} -e ${enc_dec_param} --codec=${codec_std_test_param}"
test_param="$test_param --bsmode=${bsmode_test_param} --enable-wtl=${wtl_test_param} --wtl-format=${wtl_format_param} --stream-endian=${stream_endian_param} --frame-endian=${frame_endian_param}"
if [ "${g_product_name}" == "wave627" ] || [ "${g_product_name}" == "wave637" ]; then
    test_param="$test_param --source-endian=${source_endian_param}"
    if [[ ! "$bitfile" == *"mthread"* ]] || [[ ${g_product_name} == *"wave6"* ]]; then
        test_param="$test_param --custom-map-endian=${custom_map_endian_param}"
    fi
fi
test_param="$test_param --input=${stream_test_param} ${output_param} --ref_file_path=${ref_file_path_test_param}"

if [ "$enable_sfs" = "1" ]; then
    test_param="${test_param} --sfs=${sfs_test_param}"
fi


if [ "$check_scaler_test" = "true" ]; then
    test_param="${test_param} --scaler=${scaler_test_param} --sclw=${scaleWidth_test_param} --sclh=${scaleHeight_test_param}"
fi

if [ "$g_product_name" = "wave521c_dual" ]; then
    test_param="$test_param --cores=$core_param"
    test_param="$test_param --ring=$ring_param"
elif [ "$g_product_name" = "wave537" ]; then
    test_param="$test_param --cores=$core_param"
elif [ "$g_product_name" = "wave663" ]; then
    test_param="$test_param --cores=$core_param"
fi

if [ $bitstream_buf_size -ne 0 ]; then
        test_param="$test_param --bs-size=$bitstream_buf_size"
fi

if [[ "${g_product_name}" = *"wave6"* ]]; then
        test_param="$test_param --forced-10b=${forced_10b_param} --last-2bit-in-8to10=${last_2bit_in_8to10_param}"
        test_param="$test_param --csc-enable=${csc_enable_param} --csc-format-order=${csc_rgb_order_param} --csc-srcFormat=${csc_srcformat_param}"
        test_param="${test_param} --enc-csc-ry-coef=${enc_csc_ry_coef_param} --enc-csc-gy-coef=${enc_csc_gy_coef_param} --enc-csc-by-coef=${enc_csc_by_coef_param}"
        test_param="${test_param} --enc-csc-rcb-coef=${enc_csc_rcb_coef_param} --enc-csc-gcb-coef=${enc_csc_gcb_coef_param} --enc-csc-bcb-coef=${enc_csc_bcb_coef_param}"
        test_param="${test_param} --enc-csc-rcr-coef=${enc_csc_rcr_coef_param} --enc-csc-gcr-coef=${enc_csc_gcr_coef_param} --enc-csc-bcr-coef=${enc_csc_bcr_coef_param}"
        test_param="${test_param} --enc-csc-y-offset=${enc_csc_y_offset_param} --enc-csc-cb-offset=${enc_csc_cb_offset_param} --enc-csc-cr-offset=${enc_csc_cr_offset_param}"
fi


test_param="$test_param $g_frame_num_param"

################################################################################
# Test Mode Thread                                                             #
################################################################################
result=0
log_conf "MultiInstance test : Thread" ${temp_log_path}
log_conf "--------------------------------------------------------------------------------" ${temp_log_path}
for ((i=0;i<$num_of_streams;i++)) do
    log_conf "[$(($i + 1))/${num_of_streams}] ${stream[$i]}" ${temp_log_path}
done
log_conf "--------------------------------------------------------------------------------" ${temp_log_path}
cat $temp_log_path >> $conf_log_path

log_conf "$test_exec $test_param" ${temp_log_path}

log_conf "$test_exec $test_param" $temp_log_path
nice -n -5 $test_exec $test_param || result=$?

if [ "$result" == "0" ]; then
    for ((i=0; i<$num_of_streams; i++)); do
        output_file=${output[$i]}
        g_pvric=${pvric[$i]}
        if [ "$g_pvric" == "1" ]; then
            compare_pvric $numofFrames ${ref_file_path[$i]}
            if [ "$?" != "0" ]; then
                echo "[PVRIC MISMATCH] ${stream[$i]}"
                result=1
            fi
        fi
    done

fi

if [ "$result" == "0" ]; then
    log_conf "[RESULT] SUCCESS" $conf_log_path
    g_success_count=$(($g_success_count + 1))
else
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

# clear temp log
echo "" > $temp_log_path

g_remain_count=$(($g_remain_count - 1))

################################################################################
# Create Test Log                                                              #
################################################################################
endTime=$(date +%s%N)
elapsed=$((($endTime - $beginTime) / 1000000000))
elapsedH=$(($elapsed / 3600))
elapsedS=$(($elapsed % 60))
elapsedM=$(((($elapsed - $elapsedS) / 60) % 60))
if [ "$((elapsedS / 10))" == "0" ]; then elapsedS="0${elapsedS}" ;fi
if [ "$((elapsedM / 10))" == "0" ]; then elapsedM="0${elapsedM}" ;fi
if [ "$((elapsedH / 10))" == "0" ]; then elapsedH="0${elapsedH}" ;fi
time_hms="${elapsedH}:${elapsedM}:${elapsedS}"

log_filename=$(basename $conf_log_path)
log_err_filename=$(basename $conf_err_log_path)
if [ $g_failure_count == 0 ] && [ $num_of_streams != 0 ]; then
    pass=${PASS}
    rm $conf_err_log_path
    log_err_filename=""
else
    pass=${FAIL}
fi


if [ "$num_of_streams" == "0" ]; then
    echo "num_of_streams: $num_of_streams = exit 1"
    exit 1
fi

exit $g_failure_count

