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

###############################################################################
# COMMON DATA
###############################################################################
readonly TRUE=1
readonly FALSE=0

ON_OFF=(
    "OFF"
    "ON"
)
declare -A g_cfg_param=()

readonly CODA_MIN_ENDIAN=0
readonly CODA_MAX_ENDIAN=3
readonly WAVE_MIN_ENDIAN=16
readonly WAVE_MAX_ENDIAN=31

readonly CODA_SEC_AXI_MAX=63
readonly SCALER_SEC_AXI_MAX=15
readonly BWB_SECONDARY_AXI_MAX=7

YUV_FORMAT_LIST=(
    "FORMAT_420"
    "FORMAT_422"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "FORMAT_420_P10_16BIT_MSB"
    "FORMAT_420_P10_16BIT_LSB"
    "FORMAT_420_P10_32BIT_MSB"
    "FORMAT_420_P10_32BIT_LSB"
    "FORMAT_422_P10_16BIT_MSB"
    "FORMAT_422_P10_16BIT_LSB"
    "FORMAT_422_P10_32BIT_MSB"
    "FORMAT_422_P10_32BIT_LSB"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
)

YUV_FORMAT_LIST_8BIT=(
    "FORMAT_420"
    "FORMAT_422"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
)

# DO NOT CHANGE ORDER
YUV_FORMAT_LIST_SCALER_10BIT=(
    "FORMAT_420"
    "FORMAT_422"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "FORMAT_420_P10_16BIT_MSB"
    "FORMAT_420_P10_16BIT_LSB"
    "FORMAT_420_P10_32BIT_MSB"
    "FORMAT_420_P10_32BIT_LSB"
    "FORMAT_422_P10_16BIT_MSB"
    "FORMAT_422_P10_16BIT_LSB"
    "FORMAT_422_P10_32BIT_MSB"
    "FORMAT_422_P10_32BIT_LSB"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
)

YUV_FORMAT_LIST_SCALER_8BIT=(
    "FORMAT_420"
    "FORMAT_422"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
    "RESERVED"
)


AFBCE_FORMAT_NAME=(
    "NONE"
    "AFBCE_FORMAT_8BIT"
    "AFBCE_FORMAT_10BIT"
)

C9_ENDIAN_NAME=(
    "VDI_64BIT_LE"
    "VDI_64BIT_BE"
    "VDI_32BIT_LE"
    "VDI_32BIT_BE"
)

W4_ENDIAN_NAME=(
    "VDI_128BIT_BIG_ENDIAN"
    "VDI_128BIT_BE_BYTE_SWAP"
    "VDI_128BIT_BE_WORD_SWAP"
    "VDI_128BIT_BE_WORD_BYTE_SWAP"
    "VDI_128BIT_BE_DWORD_SWAP"
    "VDI_128BIT_BE_DWORD_BYTE_SWAP"
    "VDI_128BIT_BE_DWORD_WORD_SWAP"
    "VDI_128BIT_BE_DWORD_WORD_BYTE_SWAP"
    "VDI_128BIT_LE_DWORD_WORD_BYTE_SWAP"
    "VDI_128BIT_LE_DWORD_WORD_SWAP"
    "VDI_128BIT_LE_DWORD_BYTE_SWAP"
    "VDI_128BIT_LE_DWORD_SWAP"
    "VDI_128BIT_LE_WORD_BYTE_SWAP"
    "VDI_128BIT_LE_WORD_SWAP"
    "VDI_128BIT_LE_BYTE_SWAP"
    "VDI_128BIT_LITTLE_ENDIAN"
)

DEC_BSMODE_NAME=(
    "Interrupt"
    "Reserved"
    "PicEnd"
)

ENC_BSMODE_NAME=(
    "Ringbuffer"
    "Linebuffer"
)

ENC_WAVE_BSMODE_NAME=(
    "Linebuffer"
    "Ringbuffer, Wrap On"
    "Ringbuffer, Wrap Off"
)

CODA980_MAPTYPE_NAME=(
    "LINEAR_FRAME"
    "TILED_FRAME_V"
    "TILED_FRAME_H"
    "TILED_FIELD_V"
    "TILED_MIXED_V"
    "TILED_FRAME_MB_RASTER"
    "TILED_FIELD_MB_RASTER"
    "TILED_FRAME_NO_BANK"
    "TILED_FIELD_NO_BANK"
)

CODA960_MAPTYPE_NAME=(
    "LINEAR_FRAME"
    "TILED_FRAME_V"
    "TILED_FRAME_H"
    "TILED_FIELD_V"
    "TILED_MIXED_V"
    "TILED_FRAME_MB_RASTER"
    "TILED_FIELD_MB_RASTER"
)

MIRROR_NAME=(
    "NONE"
    "VERTICAL"
    "HORIZONTAL"
    "BOTH"
)

CODEC_NAME=(
    "H.264"
    "VC-1"
    "MPEG-2"
    "MPEG-4"
    "H.263"
    "DIVX3"
    "RV"
    "AVS"
    "NONE"
    "THEORA"
    "NONE"
    "VP8"
    "HEVC"
    "VP9"
    "AVS2"
    "SVAC"
    "AV1"
)

SRC_FORMAT_NAME=(
    "8BIT"
    "10BIT - 1Pixel 2bytes MSB"
    "10BIT - 1Pixel 2bytes LSB"
    "10BIT - 3Pixel 4bytes MSB"
    "10BIT - 3Pixel 4bytes LSB"
)


PASS="PASS"
FAIL="FAIL"

is_main10=""
disable_wtl_temp=-99
MODE_COMP_ENCODED=0
bitfile=""
yuv_dir=""


###############################################################################
# GLOBAL VARIABLES
###############################################################################
g_fpga_reset=0                       # 1 : reset fpga every time
g_multi_vcore=0
g_force_num_cores=0
g_num_cores_temp=-99
g_ringBuffer_temp=-99
g_vcore_idc=0
g_failure_count=0
g_success_count=0


######################## COMMON VARAIABLES FOR DECODER ########################
g_yuv_fmt_list=("${YUV_FORMAT_LIST[@]}")
g_match_mode=1                        # 0 - nomatch, 1 - yuv, 2 - md5, 3 - pvric
g_yuv_mode_temp=0
g_frame_num=0
g_yuv_output=""
default_opt=0
stream_endian_temp=-99
frame_endian_temp=-99
source_endian_temp=-99
custom_map_endian_temp=-99
g_srcFormat_temp=-99
g_ringBuffer_temp=-99
g_vcore_idc=0
g_vcore_idc_temp=-99
srcFormat_temp=-99
g_bw_opt_temp=-99
g_secondary_axi=0
g_stream_endian=0
g_source_endian=0
g_frame_endian=0
g_custom_map_endian=0
g_bsmode=0
g_enable_thumbnail=0
g_cbcr_interleave=0
g_cbcr_interleave_temp=-99
g_enable_nv21=0
g_enable_nv21_temp=-99
g_enable_i422=0
g_enable_yuv422=0
g_enable_yuv422_temp=0
g_enable_wtl=1
g_wtl_format=0
g_bs_size=0
g_codec_index=0
g_refc_frame_num=""
secondary_axi_temp=0
g_output_hw_temp=0
bsmode_temp=0
g_is_monochrome=0
g_luma_bit_depth=8
g_chroma_bit_depth=8
g_cframe_temp=0
g_subFrameSync_temp=0
g_sfs_reg_base_temp=0
g_sfs_fb_count_temp=2   #mininum frame buffer count == 2 
g_nal_report_temp=0
g_cframe_422=0
g_scaler="false"
g_rand_scaler=0
g_revision=""
g_bsmode_force=""
g_cframe=0

########################## WAVE5xx DECODER VARAIABLES #########################
g_tid_test=0
g_bw_opt=0
g_afbce=0
g_pvric=0

####################### CODA9&CODA7 DECODER VARAIABLES ########################
g_maptype_index=0           # CODA9xx feature
g_enable_tiled2linear=0
g_enable_deblock=0
g_enable_dering=0
g_enable_mvc=0              # H.264 MVC
g_mp4class=0
g_maptype_num=1
g_enable_ppu=1
rotAngle_temp=0
mirDir_temp=0
g_prp_mode=0

######################## VARAIABLES FOR WAVE5 ENCODER #########################
g_subframe_sync=0
g_packedFormat=0
g_srcFormat=0
######################## VARAIABLES FOR CODA9 ENCODER #########################
g_enable_linear2tiled=0
g_linebuf_int=0

########################## VARAIABLES FOR PPU #################################
g_rotAngle=0
g_mirDir=0
g_rot_mir_enable=0

readonly SCALER_CSC_EXEC="scaler_csc"
readonly SCALER_EXEC="scaler"
g_oriw=0
g_orih=0
g_sclw=0
g_sclh=0
g_min_scale_w=0
g_min_scale_h=0

g_enable_csc="false"

srcFormat_temp=-99

###############################################################################
# GLOBAL FUNCTIONS
###############################################################################
# parse_streamset_file streamset_path *array
# - parse streamset file and store data into @g_stream_file_array
#
function parse_streamset_file {
    local f="$1"
    local index=0
    local line=""
    while read -r line || [ -n "$line" ]; do
        line="${line#*( )}"         # remove front whitespace
        line="${line%%[ $'\t']*}"   # remove start of whitespace
        line="${line//\\//}"        # replace backslash to slash
        line="${line/$'\r'/}"       # remove carriage return
        firstchar=${line:0:1}
        case "$firstchar" in
            "@") break;;        # exit flag
            "#") continue;;     # comment
            ";") continue;;     # comment
            "")  continue;;     # comment
            *)
        esac
        eval $2[$index]="$line"
        echo "$line"
        index=$(($index + 1))
    done < $f
}

function read_cfg {
    local       args=( $@ )
    local       _cfg_path=${args[0]}
    unset       args[0]
    local       cfg_pnames=( ${args[@]} )
    local       line=""
    local       attr=""
    local       value=""
    local       name=""

    if [ -f ${_cfg_path} ]; then
        while read line; do
            # remove right comment
            line=$(echo $line | tr -d '\n')
            line=$(echo $line | tr -d '\r')
            line="${line%%\#*}"
            attr=$(echo $line | cut -d':' -f1)
            attr=$(echo $attr | tr -d ' ')
            value=$(echo $line | cut -d':' -f2)
            value=$(echo $value | tr -d ' ')

            if [ -z ${attr} ] ; then
                continue
            fi
            name=$( is_cfg_param ${attr} ${cfg_pnames[@]} )
            if [ -z ${name} ] ; then
                continue
            fi
            g_cfg_param[${attr}]=${value}
        done < $_cfg_path
    else
        echo "cfg not exist, cfg_path=$_cfg_path"
    fi
}

function is_cfg_param() {
    local args=( $@ )
    local name=${args[0]}
    unset args[0]
    local cfg_pnames=( ${args[@]} )
    local max_num=${#cfg_pnames[@]}
    local i=0

    for ((i = 0; i < max_num ; i++)) ; do
        if [[ ${name} == ${cfg_pnames[$i]} ]] ; then
            break;
        fi
    done

    if [ $i -ge ${max_num} ] ; then
        echo ""
    else
        echo ${cfg_pnames[$i]}
    fi
}

function init_cfg_param() {
    local cfg_pnames=( $@ )
    local max_num=${#cfg_pnames[@]}
    local i=0

    for ((i = 0; i < max_num; i++)) ; do
        g_cfg_param[${cfg_pnames[$i]}]=0
    done
}



function set_min_max_param {
    if [[ $g_product_name =~ coda9* ]]; then
        MIN_ENDIAN=$CODA_MIN_ENDIAN
        MAX_ENDIAN=$CODA_MAX_ENDIAN
        MAX_SEC_AXI=$CODA_SEC_AXI_MAX
    else
        MIN_ENDIAN=$WAVE_MIN_ENDIAN
        MAX_ENDIAN=$WAVE_MAX_ENDIAN
        if [ $g_scaler == "true" ]; then
            if [[ $g_product_name =~ wave6* ]]; then
                MAX_SEC_AXI=$BWB_SECONDARY_AXI_MAX
            else
                MAX_SEC_AXI=$SCALER_SEC_AXI_MAX
            fi
        else
            MAX_SEC_AXI=$BWB_SECONDARY_AXI_MAX
        fi
    fi
}

function set_yuv_mode_dual {
    local max_val=$1
    g_cbcr_interleave=0
    g_enable_nv21=0
    g_enable_i422=0
    case $(gen_val 0 $max_val $g_yuv_mode_temp) in
        0) g_cbcr_interleave=1        #NV12
           g_enable_nv21=0;;
        1) g_cbcr_interleave=1        #NV21
           g_enable_nv21=1;;
        2) g_cbcr_interleave=1        #NV16
           g_enable_nv21=0
           g_enable_i422=1;;
        3) g_cbcr_interleave=1        #NV61
           g_enable_nv21=1
           g_enable_i422=1;;
    esac
}

function set_yuv_mode {
    local max_val=$1
    g_cbcr_interleave=0
    g_enable_nv21=0
    g_packedFormat=0
    g_enable_i422=0
    case $(gen_val 0 $max_val $g_yuv_mode_temp) in
        0) g_cbcr_interleave=0        #separate YUV
           g_enable_nv21=0;;
        1) g_cbcr_interleave=1        #NV12
           g_enable_nv21=0;;
        2) g_cbcr_interleave=1        #NV21
           g_enable_nv21=1;;
        3) g_packedFormat=1;;         #YUYV
        4) g_packedFormat=2;;         #YVYU
        5) g_packedFormat=3;;         #UYVY
        6) g_packedFormat=4;;         #VYUY
        7) g_enable_i422=1;;          #I422
        8) g_cbcr_interleave=1        #NV16
           g_enable_nv21=0
           g_enable_i422=1;;
        9) g_cbcr_interleave=1        #NV61
           g_enable_nv21=1
           g_enable_i422=1;;
        *) g_cbcr_interleave=0
           g_enable_nv21=0
           g_enable_i422=0
           g_packedFormat=0;;
    esac
}

function set_bsmode {
    g_bsmode=$(gen_val 0 1 $bsmode_temp)
    if [ -n "$g_bsmode_force" ];  then #only coda
        g_bsmode=$g_bsmode_force
    fi

    if [ "$g_bsmode" = "1" ]; then
        g_bsmode=2;
    fi

    # VP9 constraint(PIC_END Mode)
    if [ "$codec" = "vp9" ]; then
        g_bsmode=2
    fi
}

function set_rot_mir {
    val=$(gen_val 0 3 $rotAngle_temp)
    if [ "$val" -le "3" ]; then
        g_rotAngle=$(expr $val \* 90)
    else
        g_rotAngle=$val
    fi
    g_mirDir=$(gen_val 0 3 $mirDir_temp)

    set_prp_mode $g_rotAngle $g_mirDir
}

function set_prp_mode {
    local rot=$1
    local mir=$2

    if (( $mir == 0 )); then
        case $rot in
            0)   g_prp_mode=0;;
            90)  g_prp_mode=1;;
            180) g_prp_mode=2;;
            270) g_prp_mode=3;;
            *)   echo "[ERROR] check rotation=$rot, mirror=$mir"
                 exit 1;;
        esac
    elif (( $mir == 1 )); then
        case $rot in
            0)   g_prp_mode=4;;
            90)  g_prp_mode=5;;
            180) g_prp_mode=6;;
            270) g_prp_mode=7;;
            *)   echo "[ERROR] check rotation=$rot, mirror=$mir"
                 exit 1;;
        esac
    elif (( $mir == 2 )); then
        case $rot in
            0)   g_prp_mode=8;;
            90)  g_prp_mode=9;;
            180) g_prp_mode=10;;
            270) g_prp_mode=11;;
            *)   echo "[ERROR] check rotation=$rot, mirror=$mir"
                 exit 1;;
        esac
    elif (( $mir == 3 )); then
        case $rot in
            0)   g_prp_mode=12;;
            90)  g_prp_mode=13;;
            180) g_prp_mode=14;;
            270) g_prp_mode=15;;
            *)   echo "[ERROR] check rotation=$rot, mirror=$mir"
                 exit 1;;
        esac
    else
        echo "[ERROR] check rotation=$rot, mirror=$mir"
        exit 1
    fi

    echo "rotation=$rot, mirror=$mir, prp_mode=$g_prp_mode"
}

function set_decoder_option_w511 {
    local output_hw=""
    g_enable_wtl=1
    if [ "$g_scaler" = "true" ]; then
        # DOWN SCALING TEST
        output_hw="scaler"
    else
        if [ "$disable_wtl_temp" != "-99" ]; then
            if [ "$disable_wtl_temp" == "0" ]; then
                    output_hw="bwb"
            else
                    output_hw="fbc"
            fi
        else
            val=$(gen_val 0 1 $disable_wtl_temp)
            if [ "$val" == "0" ]; then
                output_hw="bwb"
            else
                output_hw="fbc"
            fi
        fi
    fi

    case $output_hw in
    "bwb") # BWB
        if [ "$is_main10" == "1" ]; then
            g_yuv_fmt_list=("${YUV_FORMAT_LIST[@]}")
        else
            g_yuv_fmt_list=("${YUV_FORMAT_LIST_8BIT[@]}")
        fi
        g_wtl_format=$(gen_wtl_format $wtl_format_temp)
        ;;
    "fbc") # FBC
        # FBC IS A COMPRESSED FRAMEBUFFER FORMAT. DISABLE NV12/21
        g_enable_wtl=0
        g_cbcr_interleave=0
        g_enable_nv21=0
        ;;
    "scaler")
        g_wtl_format=0
        # SCALER
        if [ "$g_scaler" == "true" ]; then
            if [ "$is_main10" == "1" ]; then
                g_yuv_fmt_list=("${YUV_FORMAT_LIST_SCALER_10BIT[@]}")
            else
                g_yuv_fmt_list=("${YUV_FORMAT_LIST_SCALER_8BIT[@]}")
            fi
        else
            if [ "$is_main10" == "1" ]; then
                g_yuv_fmt_list=("${YUV_FORMAT_LIST[@]}")
            else
                g_yuv_fmt_list=("${YUV_FORMAT_LIST_8BIT[@]}")
            fi
        fi
        g_wtl_format=$(gen_wtl_format $wtl_format_temp)
        ;;
    esac

    # BANDWIDTH OPTIMIZATION
    if [ $g_enable_wtl -eq 1 ]; then
        g_bw_opt=$(gen_val 0 1 $g_bw_opt_temp)
    fi
}

function set_decoder_option_w517 {
    local output_hw=""
    g_enable_wtl=1

    if [ "$g_scaler" = "true" ]; then
        # DOWN SCALING TEST
        output_hw="scaler"
    else
        if [ "$disable_wtl_temp" != "-99" ]; then
            if [ "$disable_wtl_temp" == "0" ]; then
                    output_hw="bwb"
            else
                    output_hw="fbc"
            fi
        else
            val=$(gen_val 0 1 $disable_wtl_temp)
            if [ "$val" == "0" ]; then
                output_hw="bwb"
            else
                output_hw="fbc"
            fi
        fi
    fi

    case $output_hw in
    "bwb") # BWB
        if [ "$is_main10" == "1" ]; then
            g_yuv_fmt_list=("${YUV_FORMAT_LIST[@]}")
        else
            g_yuv_fmt_list=("${YUV_FORMAT_LIST_8BIT[@]}")
        fi
        g_wtl_format=$(gen_wtl_format $wtl_format_temp)
        ;;
    "fbc") # FBC
        # FBC IS A COMPRESSED FRAMEBUFFER FORMAT. DISABLE NV12/21
        g_enable_wtl=0
        g_cbcr_interleave=0
        g_enable_nv21=0
        ;;
    "scaler")
        g_wtl_format=0
        # SCALER
        if [ "$g_scaler" == "true" ]; then
            if [ "$is_main10" == "1" ]; then
                g_yuv_fmt_list=("${YUV_FORMAT_LIST_SCALER_10BIT[@]}")
            else
                g_yuv_fmt_list=("${YUV_FORMAT_LIST_SCALER_8BIT[@]}")
            fi
        else
            if [ "$is_main10" == "1" ]; then
                g_yuv_fmt_list=("${YUV_FORMAT_LIST[@]}")
            else
                g_yuv_fmt_list=("${YUV_FORMAT_LIST_8BIT[@]}")
            fi
        fi
        g_wtl_format=$(gen_wtl_format $wtl_format_temp)
        ;;
    esac

    # BANDWIDTH OPTIMIZATION
    if [ $g_enable_wtl -eq 1 ]; then
        g_bw_opt=$(gen_val 0 1 $g_bw_opt_temp)
    fi
}

function set_decoder_option_w6xx {
    local output_hw=""
    g_enable_wtl=1

    if [ "$g_scaler" = "true" ]; then
        # DOWN SCALING TEST
        output_hw="scaler"
    else
        if [ "$disable_wtl_temp" != "-99" ]; then
            if [ "$disable_wtl_temp" == "0" ]; then
                    output_hw="bwb"
            else
                    output_hw="fbc"
            fi
        else
            val=$(gen_val 0 1 $disable_wtl_temp)
            if [ "$val" == "0" ]; then
                output_hw="bwb"
            else
                output_hw="fbc"
            fi
        fi
    fi

    if [ "$output_hw" = "bwb" ] || [ "$output_hw" = "scaler" ]; then
        #g_cbcr_interleave=0 # todo re-check test-constraints
        #g_enable_nv21=0
        if (( is_main10 == 1)); then
            g_yuv_fmt_list=(
                    "FORMAT_420"
                    "RESERVED"
                    "RESERVED"
                    "RESERVED"
                    "RESERVED"
                    "FORMAT_420_P10_16BIT_MSB"
                    "FORMAT_420_P10_16BIT_LSB"
                )
        else
            g_yuv_fmt_list=("FORMAT_420")
        fi
    fi

    case $output_hw in
    "bwb") # BWB
        g_wtl_format=$(gen_wtl_format $wtl_format_temp)
        ;;
    "fbc") # FBC
        # FBC IS A COMPRESSED FRAMEBUFFER FORMAT. DISABLE NV12/21
        g_enable_wtl=0
        g_cbcr_interleave=0
        g_enable_nv21=0
        ;;
    "scaler")
        g_wtl_format=$(gen_wtl_format $wtl_format_temp)
        ;;
    esac

    # BANDWIDTH OPTIMIZATION
    if [ $g_enable_wtl -eq 1 ]; then
        g_bw_opt=$(gen_val 0 1 $g_bw_opt_temp)
    fi
}

function set_decoder_option_w512 {
    # WAVE512 has three output H/W modules.
    # 0 - BWB, 1 - AFBCE, 2 - DOWN-SCALER, 3 - DISABLE-WTL
    # BWB, AFBCE AND DOWNSCALER NEED WTL FUNCTION
    g_enable_wtl=1
    g_afbce=0
    if [ "$g_scaler" = "true" ]; then
        # DOWN SCALING TEST
        val=2
    else
        val=$(gen_val 0 3 $g_output_hw_temp)
    fi

    # WAVE515 do not support afbce
    if [ "$g_product_name" = "wave515" ] && [ "$val" == "1" ]; then
        val=0
    fi
    # WAVE511 do not support afbce
    if [ "$g_product_name" = "wave511" ] && [ "$val" == "1" ]; then
        val=0
    fi

    # WAVE521c_dual do not support afbce,
    if [ "$g_product_name" = "wave521c_dual" ] && [ "$val" == "1" ]; then
        val=0
    fi
    if [ -z "$wtl_format_temp" ]; then
        val=0
    fi
    case $val in
    0) # BWB
        if [ "$is_main10" == "1" ]; then
            g_yuv_fmt_list=("${YUV_FORMAT_LIST[@]}")
        else
            g_yuv_fmt_list=("${YUV_FORMAT_LIST_8BIT[@]}")
        fi
        g_wtl_format=$(gen_wtl_format $wtl_format_temp)
        ;;
    1)
        g_wtl_format=0
        ;;
    2)
        g_wtl_format=0
        # SCALER
        if [ "$g_scaler" == "true" ]; then
            if [ "$is_main10" == "1" ]; then
                g_yuv_fmt_list=("${YUV_FORMAT_LIST_SCALER_10BIT[@]}")
            else
                g_yuv_fmt_list=("${YUV_FORMAT_LIST_SCALER_8BIT[@]}")
            fi
        else
            if [ "$is_main10" == "1" ]; then
                g_yuv_fmt_list=("${YUV_FORMAT_LIST[@]}")
            else
                g_yuv_fmt_list=("${YUV_FORMAT_LIST_8BIT[@]}")
            fi
        fi
        g_wtl_format=$(gen_wtl_format $wtl_format_temp)
        ;;
    3) # FBC
        # FBC IS A COMPRESSED FRAMEBUFFER FORMAT. DISABLE NV12/21
        g_enable_wtl=0
        g_cbcr_interleave=0
        g_enable_nv21=0
        ;;
    esac

    # BANDWIDTH OPTIMIZATION
    if [ $g_enable_wtl -eq 1 ]; then
        g_bw_opt=$(gen_val 0 1 $g_bw_opt_temp)
    fi
}

function set_decoder_option_w525 {
    # WAVE512 has three output H/W modules.
    # 0 - BWB, 1 - DISABLE-WTL
    g_enable_wtl=1
    val=$(gen_val 0 1 $g_output_hw_temp)

    case $val in
    0) # BWB
        if [ "$is_main10" == "1" ]; then
            g_yuv_fmt_list=("${YUV_FORMAT_LIST[@]}")
        else
            g_yuv_fmt_list=("${YUV_FORMAT_LIST_8BIT[@]}")
        fi
        g_wtl_format=$(gen_wtl_format $wtl_format_temp)
        ;;
    1)
        # FBC IS A COMPRESSED FRAMEBUFFER FORMAT. DISABLE NV12/21
        g_enable_wtl=0
        g_cbcr_interleave=0
        g_enable_nv21=0
        ;;
    esac

    # BANDWIDTH OPTIMIZATION
    if [ $g_enable_wtl -eq 1 ]; then
        g_bw_opt=$(gen_val 0 1 $g_bw_opt_temp)
    fi
}

function set_decoder_option {
    set_bsmode
    set_yuv_mode 2 #2:without packedformat
    # CODA9 features
    if [[ $g_product_name == "coda9"* ]]; then
        set_rot_mir
        if [ "$codec" = "mp2" ] || [ "$codec" = "mp4" ]; then
            g_enable_deblock=$(gen_val 0 1 $deblock_temp)
            g_enable_dering=$(gen_val 0 1 $dering_temp)
        else
            g_enable_deblock=0
            g_enable_dering=0
        fi
        g_maptype_index=$(gen_val 0 $(($g_maptype_num-1)) $maptype_temp)
        # TILED2LINEAR
        g_enable_wtl=0
        g_enable_tiled2linear=0
        if [ $g_maptype_index -gt 0 ]; then
            local val=$(get_random 0 1)
            if [ $val -eq 0 ]; then
                g_enable_tiled2linear=1
            else
                g_enable_wtl=1
            fi
        fi

        # check combination of parameters
        if [ $g_enable_wtl -eq 1 ] || [ $g_enable_ppu -eq 0 ]; then
            g_rotAngle=0
            g_mirDir=0
            g_enable_dering=0
        fi
    else
        case $g_product_name in
        wave511 )
            set_decoder_option_w511;; #bwb, scaler, fbc
        wave512 | wave515 )
            set_decoder_option_w512;; #bwb, afbce, scaler, fbc
        wave521c | wave521c_dual | wave517 | wave537)
            set_decoder_option_w517;; #bwb, scaler, fbc, pvric
        wave637 | wave617 | wave677)
            set_decoder_option_w6xx;; #bwb, scaler, fbc, pvric
        *) echo "Unknown product name: $g_product_name"
           exit 1;;
        esac
    fi
}

function set_encoder_option {
    local prp_test_type=$1
    local enc_yuv_feeding_mode=$2
    
    set_rot_mir
    if [[ $g_product_name == "coda9"* ]]; then
        set_yuv_mode 2 #2:without packedformat
        g_maptype_index=$(gen_val 0 $(($g_maptype_num -1)) $maptype_temp)
        g_enable_linear2tiled=$(gen_val 0 1 $linear2tiled_temp)
        g_linebuf_int=$(gen_val 0 1 $linebuf_int_temp)
        g_bsmode=$(gen_val 0 1 $bsmode_temp)
        g_subframe_sync=$(gen_val 0 1 $subframe_sync_temp)
    else
        if [ "$g_product_name" = "wave521c_dual" ]; then #constraint
            set_yuv_mode_dual 2 #nv12, nv21
        else
            g_enable_yuv422=$(gen_val 0 1 $g_enable_yuv422_temp)
            if [[ "$g_enable_yuv422" == "1" ]]; then
                set_yuv_mode 9
            else
                set_yuv_mode 6
            fi
        fi
        MODE_COMP_ENCODED=$(gen_val 0 1 $MODE_COMP_ENCODED_temp)
        if [ ${g_cfg_param[InputBitDepth]} -eq 8 ]; then
            if [[ "$g_enable_i422" == "1" ]]; then
                g_srcFormat=5
            else
                g_srcFormat=0
            fi
        else
            if [ $srcFormat_temp == 0 ]; then
                srcFormat_temp="-99"
            fi
            if [ "$g_product_name" = "wave521c_dual" ]; then #constraint
                if [[ "$g_enable_i422" == "1" ]]; then
                    g_srcFormat=$(gen_val 8 9 $srcFormat_temp)
                else
                    g_srcFormat=$(gen_val 3 4 $srcFormat_temp)
                    if [ "$g_srcFormat" == "1" ] || [ "$g_srcFormat" == "2" ]; then
                        g_srcFormat=$(get_random 3 4)
                    fi
                fi
            else
                if [[ "$g_enable_i422" == "1" ]]; then
                    g_srcFormat=$(gen_val 6 9 $srcFormat_temp)
                else
                    g_srcFormat=$(gen_val 1 4 $srcFormat_temp)
                fi
            fi
        fi
        g_ringBuffer=$(gen_val 0 1 $g_ringBuffer_temp)

        if [ "$g_product_name" == "wave521c_dual" ]; then
            if [ "$g_ringBuffer" == "1" ]; then
                g_ringBuffer=2
            fi
        fi
        g_cframe=$(gen_val 0 1 $g_cframe_temp)
        if [[ $cfg_path == *"mc"* ]]; then
            g_cframe=0
        fi
        if [ "$g_cframe" == "1" ]; then
            if [ "$g_product_name" == "wave521" ] || [ "$g_product_name" == "wave521c" ] || [ "$g_product_name" == "521E1" ];  then
                g_cframelossless=$(gen_val 0 1 $g_cframelossless_temp)
                if [ ${g_cfg_param[InputBitDepth]} -eq 8 ]; then
                    #32, 48, 64, 80, 96, 112
                    g_cframetx16y=$(gen_val 2 7 $g_cframetx16y_temp)
                    #32, 48, 64, 80, 96, 112
                    g_cframetx16c=$(gen_val 2 7 $g_cframetx16c_temp)
                else
                    #32, 48, 64, 80, 96, 112, 128, 144
                    g_cframetx16y=$(gen_val 2 9 $g_cframetx16y_temp)
                    #32, 48, 64, 80, 96, 112, 128, 144
                    g_cframetx16c=$(gen_val 2 9 $g_cframetx16c_temp)
                fi
                if [ $g_cframetx16y -le 10 ]; then
                    g_cframetx16y=$(($g_cframetx16y * 16))
                fi
                if [ $g_cframetx16c -le 10 ]; then
                    g_cframetx16c=$(($g_cframetx16c * 16))
                fi
            fi
            if [ "$g_product_name" == "wave521" ] || [ "$g_product_name" == "wave521c" ] || [ "$g_product_name" == "521E1" ];  then
                g_cframe_422=$(gen_val 0 1 $g_cframe_422_temp)
            fi
        fi
        if [[ "$bitfile" == *"sigmastar"* ]] || [[ "$bitfile" == *"realtek"* ]]; then
            g_irb_height=$(gen_val 256 $((${g_cfg_param[SourceHeight]} * 2)) $g_irb_height_temp) #input ringbuffer height and enable(not zero)
            g_irb_height=$(ceiling $g_irb_height 64)
            g_maxVerticalMV=0
            g_refRingBufMode=$(gen_val 0 1 $g_refRingBufMode_temp)
            if [ $g_refRingBufMode == 1 ]; then
                if [ ${g_cfg_param[UseAsLongTermRefPeriod]} -ge 1 ]; then
                    g_refRingBufMode=2
                fi
                if [ "$g_codec_index" = "12" ]; then #hevc
                    g_cfg_param[S2SearchCenterYdiv4]=`echo ${g_cfg_param[S2SearchCenterYdiv4]} | tr -d -` #abs
                    g_cfg_param[S2SearchRangeYdiv4]=`echo ${g_cfg_param[S2SearchRangeYdiv4]} | tr -d -`
                    if [ ${g_cfg_param[S2SearchCenterEnable]} == 1 ] || [ ${g_cfg_param[S2SearchRangeYdiv4]} != 0 ]; then
                        minVal=$(( (${g_cfg_param[S2SearchCenterYdiv4]} * 4) + (${g_cfg_param[S2SearchRangeYdiv4} * 4) + 16 + 4 + 4 + 64 ))
                        g_maxVerticalMV=$(gen_val $minVal $((${g_cfg_param[SourceHeight]} * 2)) $g_maxVerticalMV_temp)
                        g_maxVerticalMV=$minVal
                    else
                        g_maxVerticalMV=$(gen_val 216 $((${g_cfg_param[SourceHeight]} * 2)) $g_maxVerticalMV_temp)
                    fi
                else
                    g_maxVerticalMV=$(gen_val 64 $((${g_cfg_param[SourceHeight]} * 2)) $g_maxVerticalMV_temp)
                fi
                g_maxVerticalMV=$(ceiling $g_maxVerticalMV 4)
            fi
            g_skipVLC=$(gen_val 0 1 $g_skipVLC_temp)
            if [ ${g_cfg_param[DeSliceMode]} -eq 2 ]; then
                echo "DeSliceMode==2, disable g_skipVLC"
                g_skipVLC=0
            fi
            g_mvHisto=$(gen_val 0 1 $g_mvHisto_temp)
        fi
        g_subFrameSync=$(gen_val 0 1 $g_subFrameSync_temp)
        if [ $g_subFrameSync == 0 ]; then
            g_sfs_reg_base=0
            g_sfs_fb_count=0
        else
            g_num_cores=1
        fi

        if [ "$g_product_name" = "wave521c_dual" ]; then
            g_rotAngle=0
            g_mirDir=0
        fi
   fi
}

#set common options - fpga(clk, latency) , secAxi, endian
function set_common_option {

        set_min_max_param
        g_secondary_axi=$(gen_val 0 $MAX_SEC_AXI $secondary_axi_temp)
        if [ $is_decoder -eq 1 ] && [[ $g_product_name =~ wave* ]]; then
            if [ "0" == "$g_codec_index" ]; then
                if [ "$g_product_name" == "wave521c_dual" ] || [ "$g_product_name" == "wave511" ]; then
                    g_secondary_axi=$(($g_secondary_axi & 15)) # LF/IP/VCPU/(SCALER)
                else
                    g_secondary_axi=$(($g_secondary_axi & 11)) # LF/IP/x(tBPU)/SCALER
                fi
            elif [ "16" == "$g_codec_index" ]; then # AV1 constraints
                g_secondary_axi=$(($g_secondary_axi & 11)) # LF/IP/x(tBPU)/SCALER
            fi
        fi
        g_stream_endian=$(gen_val $MIN_ENDIAN $MAX_ENDIAN $stream_endian_temp)
        g_frame_endian=$(gen_val $MIN_ENDIAN $MAX_ENDIAN $frame_endian_temp)
        g_source_endian=$(gen_val $MIN_ENDIAN $MAX_ENDIAN $source_endian_temp) #only for encoder
        g_custom_map_endian=$(gen_val $MIN_ENDIAN $MAX_ENDIAN $custom_map_endian_temp)
}

#set w6 common options - fpga(clk, latency) , secAxi, endian
function set_w6_common_option {
        set_min_max_param
        g_secondary_axi=$(gen_val 0 $MAX_SEC_AXI $secondary_axi_temp)
        g_stream_endian=$(gen_val $MIN_ENDIAN $MAX_ENDIAN $stream_endian_temp)
        g_frame_endian=$(gen_val $MIN_ENDIAN $MAX_ENDIAN $frame_endian_temp)
        g_source_endian=$(gen_val $MIN_ENDIAN $MAX_ENDIAN $source_endian_temp) #only for encoder
        g_custom_map_endian=$(gen_val $MIN_ENDIAN $MAX_ENDIAN $custom_map_endian_temp)
}

function get_default_option {
    g_secondary_axi=0
    g_stream_endian=0
    g_source_endian=0
    g_frame_endian=0
    g_custom_map_endian=0
    g_bsmode=0
    g_cbcr_interleave=0
    g_enable_nv21=0
    g_enable_i422=0
    g_rotAngle=0
    g_mirDir=0
    if [ $is_decoder -eq 1 ]; then
        g_enable_thumbnail=0
        g_enable_wtl=1
        g_wtl_format=0
        # CODA9 features
        g_enable_deblock=0
        g_enable_dering=0
        g_maptype_index=0
        if [[ $g_product_name =~ coda9* ]]; then
            g_enable_wtl=0;
        fi
        # END CODA9
    else
        g_subframe_sync=0
        g_packedFormat=0
    fi
    if [ "$codec" = "vp9" ]; then
        g_bsmode=2
    fi
}

function get_default_param {
    local arg_val=("$@")
    local enc_yuv_feeding_mode=-1
    is_decoder=${arg_val[0]}
    # arg_val[0] -> is_decoder, 0 : encoder , 1 : decoder
    # arg_val[1] -> product_name
    # arg_val[2] -> prp test type
    # arg_val[3] -> feeding mode for yuv

    echo "default_opt=$default_opt"
    if [ "$default_opt" = "0" ]; then
        set_common_option
        if (( arg_val[0] == 1 )); then
            set_decoder_option
        else
            if (( ${#arg_val[@]} > 3 )); then
                enc_yuv_feeding_mode=${arg_val[3]}
            fi
            set_encoder_option "${arg_val[2]}" "$enc_yuv_feeding_mode"
        fi
    else
        get_default_option
    fi
}

function gen_wtl_format {
    local default_val=$1
    local val

    # g_yuv_fmt_list variable should be defined in TestRunnerXXX.sh
    if [ -z "$default_val" ] || [ "$default_val" == "-99" ]; then
        while : ; do
            val=$(get_random 0 ${#g_yuv_fmt_list[@]}-1)
            if [ "${g_yuv_fmt_list[$val]}" != "RESERVED" ]; then
                break
            fi
        done
    else
        val=$default_val
    fi
    echo "$val"
}

function build_encoder_test_param {
    local csc_enable=0
    # ENCODER
    if [ "$MODE_COMP_ENCODED" == "1" ];  then param="${param} -c"; fi

    param="$param $param2"

    if [[ "$g_product_name" == *"wave6"* ]]; then
        param="$param --custom-map-endian=$g_custom_map_endian"
#ifdef SUPPORT_ENC_CSC_FORMAT_INPUT
        csc_enable=$g_csc_enable
#endif
    fi

    if [[ $g_product_name =~ coda9* ]]; then
        if [ $g_enable_linear2tiled -eq 1 ]; then
            param="$param --enable-linear2tiled"
        fi
        if [ $g_bsmode -eq 0 ]; then
            param="$param --enable-ringBuffer"
        else
            param="$param --enable-lineBufInt"
        fi
        if [ $g_cbcr_interleave -eq 1 ]; then
            param="$param --enable-cbcrInterleave"
            if [ $g_enable_nv21 -eq 1 ]; then
                param="$param --enable-nv21"
            fi
        fi
        param="$param --rotate=$g_rotAngle"
        param="$param --mirror=$g_mirDir"
    else
        ### WAVE5xx ###
        param="$param --codec=$g_codec_index"
        param="$param --source-endian=$g_source_endian"
        param="$param --rotAngle=${g_rotAngle} --mirDir=${g_mirDir}"
        if [[ "$bitfile" == *"sigmastar"* ]] || [[ "$bitfile" == *"realtek"* ]]; then
            param="${param} --inputRingBufHeight=$g_irb_height"
            param="${param} --refRingBufMode=$g_refRingBufMode"
            param="${param} --maxVerticalMV=$g_maxVerticalMV"
            param="${param} --skipVLC=$g_skipVLC"
        fi

        if [ "$g_cframe" == "1" ] && [ "$g_rotAngle" == "0" ] && [ "$g_mirDir" == "0" ]; then
            if [ "$g_product_name" == "wave521" ] || [ "$g_product_name" == "wave521c" ] || [ "$g_product_name" == "521E1" ]; then
                param="${param} --cframe50Enable=1 --cframe50Lossless=$g_cframelossless --cframe50Tx16Y=$g_cframetx16y --cframe50Tx16C=$g_cframetx16c --cframe50_422=$g_cframe_422"
            fi
            if [ ${g_cfg_param[InputBitDepth]} -eq 8 ]; then
                g_srcFormat=0
            else
                g_srcFormat=$(expr $g_srcFormat % 5)
            fi
            param="${param} --srcFormat=$g_srcFormat"
            g_enable_i422=0
#ifdef SUPPORT_ENC_CSC_FORMAT_INPUT
        elif [ "$csc_enable" == "1" ]; then
                param="${param} --srcFormat=${g_csc_srcFormat}"
                if [ "$g_enable_nv21" == "0" ] && [ "$g_cbcr_interleave" == "1" ]; then
                    param="${param} --enable-cbcrInterleave --nv21=${g_enable_nv21}"
                else
                    echo "[ERROR] csc not support only nv21"
                    exit 1
                fi
#endif
        else
            if [ ${g_cfg_param[InputBitDepth]} -eq 8 ] && [ ${g_enable_i422} -eq 0 ]; then
                param="${param} --srcFormat=0"
            else
                param="${param} --srcFormat=${g_srcFormat}"
            fi
            if  [ $g_packedFormat -ne 0 ]; then
                param="${param} --packedFormat=${g_packedFormat}"
            elif [ "$g_enable_nv21" == "0" ] && [ "$g_cbcr_interleave" == "1" ]; then
                param="${param} --enable-cbcrInterleave --nv21=${g_enable_nv21}"
            elif [ "$g_enable_nv21" == "1" ] && [ "$g_cbcr_interleave" == "1" ]; then
                param="${param} --enable-cbcrInterleave --nv21=${g_enable_nv21}"
            fi
        fi
        if [ "$g_enable_i422" == "1" ]; then
            param="${param} --i422=${g_enable_i422}"
        fi
        if [ "$g_subFrameSync" == "1" ]; then
            param="${param} --subFrameSyncEn=${g_subFrameSync}"
            if [[ "$g_product_name" == *"wave6"* ]]; then
                param="${param} --sfs-reg-base=${g_sfs_reg_base}"
                param="${param} --sfs-fb-count=${g_sfs_fb_count}"
            fi
        fi
    fi
}


function build_decoder_test_param {
    if [ "$g_match_mode" == "3" ]; then
        param="$param -c 0"
    else
        param="$param -c $g_match_mode"
    fi
    param="$param --render 0"
    param="$param --codec $g_codec_index"

    # CODA9 features.
    if [ $g_codec_index -eq 3 ]; then
        param="$param --mp4class=$g_mp4class"
    fi

    if [ $g_enable_deblock -eq 1 ]; then
        param="$param --enable-deblock"
    fi
    if [ $g_enable_mvc -eq 1 ]; then
        param="$param --enable-mvc"
    fi
    # END and CODA9

    param="$param $param2"

    if [ $g_bsmode -gt 0 ]; then
        param="$param --bsmode=$g_bsmode"
    fi
    if [ $g_enable_thumbnail -eq 1 ]; then
        param="$param --enable-thumbnail"
    fi

    if [ $g_cbcr_interleave -eq 1 ]; then
        param="$param --enable-cbcrinterleave"
        if [ $g_enable_nv21 -eq 1 ]; then
            param="$param --enable-nv21"
        fi
    fi

    if [[ $g_product_name =~ coda9* ]]; then
        if [ $g_enable_dering -eq 1 ]; then
            param="$param --enable-dering"
        fi
        if [ $g_enable_wtl -eq 0 ] && [ $g_maptype_index -gt 0 ]; then
            param="$param --disable-wtl"
        fi
        if [ $g_enable_tiled2linear -eq 1 ]; then
            param="$param --enable-tiled2linear"
        fi
        if [ $g_rotAngle -gt 0 ]; then
            param="$param --rotate=$g_rotAngle"
        fi
        if [ $g_mirDir -gt 0 ]; then
            param="$param --mirror=$g_mirDir"
        fi
    else
        if [ $g_enable_wtl -eq 0 ]; then
            param="${param} --disable-wtl"
        else
            param="${param} --wtl-format=$g_wtl_format"
            if [ $g_bw_opt -eq 1 ]; then
                param="${param} --bwopt=1"
            fi
        fi
    fi

    param="${param} ${g_frame_num}"
    if [ $g_scaler == "true" ]; then
        param="$param --sclw=$g_sclw --sclh=$g_sclh"
    fi
}

function build_test_param {
    local is_decoder=$1
    local param=""
    local param2=""



    if [[ $g_product_name =~ coda9* ]]; then
        param2="$param2 --maptype=$g_maptype_index"
    fi

    param2="$param2 --stream-endian=$g_stream_endian"
    param2="$param2 --frame-endian=$g_frame_endian"
    param2="$param2 --secondary-axi=$g_secondary_axi"
    if [ $is_decoder -eq 1 ]; then
        build_decoder_test_param
    else
        build_encoder_test_param
    fi

    g_func_ret_str="$param"
}

function get_random_param {
    local is_decoder=$1
    local g_product_name=$2

    default_opt=0

    aclk_freq_temp=-99
    bclk_freq_temp=-99
    cclk_freq_temp=-99

    read_latency_temp=-99
    write_latency_temp=-99

    secondary_axi_temp=-99

    stream_endian_temp=-99
    frame_endian_temp=-99
    source_endian_temp=-99

    g_maptype_index_temp=-99 #coda960 & coda980

    g_yuv_mode_temp=-99
    if [ $is_decoder -eq 1 ]; then
        bsmode_temp=-99
        deblock_temp=-99
        dering_temp=-99
        rotAngle_temp=-99
        mirDir_temp=-99

        maptype_temp=-99
        wtl_format_temp=-99
        g_bw_opt_temp=-99
    else
        rotAngle_temp=-99
        mirDir_temp=-99
        if [[ $g_product_name =~ coda9* ]]; then
            maptype_temp=-99
            linear2tiled_temp=-99
            linebuf_int_temp=-99
            bsmode_temp=-99
            subframe_sync_temp=-99
        fi
    fi

    if [ "$g_product_name" = "wave512" ] || [ "$g_product_name" = "wave515" ] || [ "$g_product_name" = "wave511" ] || [ "$g_product_name" = "wave517" ] || [ "$g_product_name" = "wave537" ]; then
        g_output_hw_temp=-99
    fi

    get_default_param $is_decoder $g_product_name 0
}

function run_bwb_converter {
    #convert bwb formats for 3p4b , 422 and cbcrInterleave
    local input_path=$1
    local output_path=$2
    local fmt=$3
    local enabled_bwb_conv=0
    local cbcrint_param=""
    local bwb_input_format="-a 2" #10b MSB from scaler bin

    debug_msg "run bwb_converter c-model.. param : $input_path, $output_path, $fmt"

    if [ -e $output_path ]; then
        echo "rm ${output_path}"
        rm ${output_path}
    fi

    case $fmt in
    FORMAT_420 | FORMAT_420_P10_16BIT_MSB | FORMAT_420_P10_16BIT_LSB)
        echo "disabled bwb converter"
        ;;
    FORMAT_420_P10_32BIT_MSB | FORMAT_420_P10_32BIT_LSB)
        enabled_bwb_conv=1
        ;;
    FORMAT_422)
        enabled_bwb_conv=1
        ;;
    FORMAT_422_P10_16BIT_MSB | FORMAT_422_P10_16BIT_LSB)
        enabled_bwb_conv=1
        ;;
    FORMAT_422_P10_32BIT_MSB | FORMAT_422_P10_32BIT_LSB)
        enabled_bwb_conv=1
        ;;
    *)
        echo "[WARN] check wtl-format"
        ;;
    esac

    if [ "$g_cbcr_interleave" != "0" ]; then
        if [ "$g_enable_nv21" == "0" ]; then
            cbcrint_param="-f 1"
        else
            cbcrint_param="-f 2"
        fi
        enabled_bwb_conv=1
    fi

    if [ "$is_main10" == "1" ]; then
        if [ $fmt == FORMAT_420 ] || [ $fmt == FORMAT_422 ]; then
            bwb_input_format="-a 0"
        else
            if [ "$g_scaler" != "true" ]; then
                    bwb_input_format="-a 2" #10b MSB input from refc bin
            fi
        fi
    else
         bwb_input_format="-a 0" #8b from refc bin
    fi

    if [ -f $bin_dir/bwb_convert ]; then
        chmod 755 $bin_dir/bwb_convert
    fi
    if [ $enabled_bwb_conv == "1" ]; then
        local bwb_conv_cmd="${bin_dir}/bwb_convert -i $input_path -o $output_path -d display_info.txt -m out.md5 -e 0 $cbcrint_param"
        if [ "$g_sclw" != "0" ] && [ "$g_sclh" != "0" ]; then #there are no scl_x, scl_y, scaler is disabled
            bwb_conv_cmd="${bwb_conv_cmd} -x $g_sclw -y $g_sclh"
        fi

        if [ $fmt == FORMAT_420 ]; then
            bwb_conv_cmd="${bwb_conv_cmd} $bwb_input_format -b 0"
        elif [ $fmt == FORMAT_420_P10_16BIT_MSB ]; then
            bwb_conv_cmd="${bwb_conv_cmd} $bwb_input_format -b 2"
        elif [ $fmt == FORMAT_420_P10_16BIT_LSB ]; then
            bwb_conv_cmd="${bwb_conv_cmd} $bwb_input_format -b 1"
        elif [ $fmt == FORMAT_420_P10_32BIT_MSB ]; then
            bwb_conv_cmd="${bwb_conv_cmd} $bwb_input_format -b 6"
        elif [ $fmt == FORMAT_420_P10_32BIT_LSB ]; then
            bwb_conv_cmd="${bwb_conv_cmd} $bwb_input_format -b 5"
        elif [ $fmt == FORMAT_422 ]; then
            bwb_conv_cmd="${bwb_conv_cmd} $bwb_input_format -b 0 -c 1"
        elif [ $fmt == FORMAT_422_P10_16BIT_MSB ]; then
            bwb_conv_cmd="${bwb_conv_cmd} $bwb_input_format -b 2 -c 1"
        elif [ $fmt == FORMAT_422_P10_16BIT_LSB ]; then
            bwb_conv_cmd="${bwb_conv_cmd} $bwb_input_format -b 1 -c 1"
        elif [ $fmt == FORMAT_422_P10_32BIT_MSB ]; then
            bwb_conv_cmd="${bwb_conv_cmd} $bwb_input_format -b 6 -c 1"
        elif [ $fmt == FORMAT_422_P10_32BIT_LSB ]; then
            bwb_conv_cmd="${bwb_conv_cmd} $bwb_input_format -b 5 -c 1"
        fi
        echo $bwb_conv_cmd; $bwb_conv_cmd
    fi
    return $?
}



function parse_dec_info_file 
{
    local file_name=$1
    local OLDIFS=$IFS
    local line_val=()
    local parsed_val_arr=()
    local val

    if [ "$file_name" == "display_info.txt" ]; then
        echo "Parsing : $file_name"
        IFS=$'\t'
        line_val=("$(head -n 1 "$file_name")")
        for val in ${line_val[@]};
        do   
            val=$(echo "$val" | cut -f2 -d':' | tr -d ' ')
            parsed_val_arr+=($((10#${val})))
        done

        echo "picture_num   : ${parsed_val_arr[0]}"
        echo "monochrome    : ${parsed_val_arr[1]}" && g_is_monochrome=${parsed_val_arr[1]}
        echo "bitdepth_y    : ${parsed_val_arr[2]}" && g_luma_bit_depth=${parsed_val_arr[2]}
        echo "bitdepth_c    : ${parsed_val_arr[3]}" && g_chroma_bit_depth=${parsed_val_arr[3]}
        echo "width_dis     : ${parsed_val_arr[4]}" && g_oriw=${parsed_val_arr[4]}
        echo "height_dis    : ${parsed_val_arr[5]}" && g_orih=${parsed_val_arr[5]}
        echo "log2_ctb_size : ${parsed_val_arr[6]}"
        echo "c10v2         : ${parsed_val_arr[7]}"
        echo "codec_std     : ${parsed_val_arr[8]}"
    else
        echo "[ERROR] Check Display info file path : $file_name"
        exit 1
    fi
    IFS=$OLDIFS
}

function get_refc_wtl_idx {
    local fmt=$1
    local wtl_idx=0

    if [ "$fmt" == "FORMAT_420" ] || [ "$fmt" == "FORMAT_422" ]; then
        wtl_idx=0
    elif [ "$fmt" == "FORMAT_420_P10_16BIT_LSB" ]; then
        wtl_idx=5
        if [ "$g_cbcr_interleave" != "0" ]; then
            wtl_idx=1 #bwb_converter input format
        fi
    else
        case $fmt in
            FORMAT_420_P10_16BIT_MSB)
                wtl_idx=1
                ;;
            FORMAT_420_P10_32BIT_MSB | FORMAT_420_P10_32BIT_LSB)
                wtl_idx=1 #bwb_converter input format
                ;;
            FORMAT_422_P10_16BIT_MSB | FORMAT_422_P10_16BIT_LSB)
                wtl_idx=1 #bwb_converter input format
                ;;
            FORMAT_422_P10_32BIT_MSB | FORMAT_422_P10_32BIT_LSB)
                wtl_idx=1 #bwb_converter input format
                ;;
            *)
                echo "[ERROR] Check wtl format : $fmt"
                wtl_idx=99
                ;;
        esac
    fi
    echo "$wtl_idx"
}

function get_scaler_wtl_idx {
    local fmt=$1
    local wtl_idx=1 #output format, 0: 1btye/1pixel (default), 1: 2btye/1pixel MSB justification 2: 2btye/1pixel LSB justification


    if [ "$fmt" == "FORMAT_420" ] || [ "$fmt" == "FORMAT_422" ]; then
        wtl_idx=0
    elif [ "$fmt" == "FORMAT_420_P10_16BIT_LSB" ]; then
        wtl_idx=2
        if [ "$g_cbcr_interleave" != "0" ]; then
            wtl_idx=1 #bwb_converter input format
        fi
    else
        case $fmt in
            FORMAT_420_P10_16BIT_MSB)
                wtl_idx=1
                ;;
            FORMAT_420_P10_32BIT_MSB | FORMAT_420_P10_32BIT_LSB)
                wtl_idx=1 #bwb_converter input format
                ;;
            FORMAT_422_P10_16BIT_MSB | FORMAT_422_P10_16BIT_LSB)
                wtl_idx=1 #bwb_converter input format
                ;;
            FORMAT_422_P10_32BIT_MSB | FORMAT_422_P10_32BIT_LSB)
                wtl_idx=1 #bwb_converter input format
                ;;
            *)
                echo "[ERROR] Check wtl format : $fmt"
                wtl_idx=99
                ;;
        esac
        if [ "$g_pvric" != "0" ]; then
            wtl_idx=2 #the 10b scaler output is used by yuv_divider
        fi
    fi
    echo "$wtl_idx"
}

function run_refc_scaler {
    local input_path=$1
    local output_path=$2
    local fmt=$3
    local wtl_mode_index=0

    debug_msg "run scaler c-model.. param : $input_path, $output_path, $fmt"

    if [ -e $output_path ]; then
        echo "rm ${output_path}"
        rm ${output_path}
    fi

    wtl_mode_index=$(get_scaler_wtl_idx $fmt)


    if [ "$codec" = "av1" ]; then
        if [ "$g_sclw" == "0" ] && [ "$g_sclh" == "0" ]; then
            echo "[WARN] scaler off case"
        else
            g_oriw=$(ceiling $g_oriw 8)
            g_orih=$(ceiling $g_orih 8)
        fi
    elif [ "$codec" = "avs2" ]; then
        if [ "$g_sclw" == "0" ] && [ "$g_sclh" == "0" ]; then
            echo "[WARN] scaler off case"
        else
            g_oriw=$(ceiling $g_oriw 8)
            g_orih=$(ceiling $g_orih 8)
        fi
    elif [ "$codec" = "vp9" ]; then
        g_oriw=$(ceiling $g_oriw 8)
        g_orih=$(ceiling $g_orih 8)
    fi

    echo "wtl_mode_index : $wtl_mode_index"
    scaler_options="-i $input_path -x $g_oriw -y $g_orih -o $output_path -a $g_sclw -b $g_sclh -n 0"

    if [ "$g_enable_csc" = "true" ]; then
        scaler_options="$scaler_options -g 3 -h 1" # input 1p2B MSB , output 1p2B MSB
    else
        if [ "$is_main10" != "1" ]; then
            scaler_options="$scaler_options -g 0 -h $wtl_mode_index"
        else
            # refc output 10bit
            scaler_options="$scaler_options -g 2 -h $wtl_mode_index"
        fi
    fi
    scaler_options="$scaler_options --realsize 1"

    if [ $g_is_monochrome -eq 1 ]; then
        echo "monochrome stream"
        scaler_options="${scaler_options} -z 1"
    fi

    if [[ "$bitfile" == *"csc"* ]]; then
        echo "scaler_csc prom : $bitfile"
        str_cmd="${bin_dir}/${SCALER_CSC_EXEC} ${scaler_options}"
    else
        echo "scaler prom : $bitfile"
        str_cmd="${bin_dir}/${SCALER_EXEC} ${scaler_options}"
    fi

    echo "${str_cmd}"
    ${str_cmd}


    if [[ $g_product_name =~ coda9* ]]; then
        echo "CODA9 Product, don't have to run bwb_converter"
    else
        if [ "$g_pvric" = "0" ] && [ "$g_enable_csc" = "false" ]; then
            run_bwb_converter $output_path "bwb_out.yuv" $fmt
        fi
    fi

    if [ -e "./bwb_out.yuv" ];then
        echo "cp bwb_out.yuv $output_path"
        cp bwb_out.yuv $output_path
    fi


    return $?
}

function run_refc_dec_av1 {
    local stream_path="$1"
    local output_path="$2"
    local wtl_mode_index=0
    local skip_option=0
    local max_frames=""
    local file_ext=${stream_path##*.}
    local stream_format_param="--ivf"
    local fmt="${g_yuv_fmt_list[$g_wtl_format]}"
    local str_opt
    debug_msg "run decoder c-model.. param : $stream_path, $output_path"


    if [ "$file_ext" = "obu" ]; then
        stream_format_param="--annexb"
    elif [ "$file_ext" = "av1" ]; then
        stream_format_param=""
    fi

    if [ "$g_refc_frame_num" == "-m 0" ]; then
        g_refc_frame_num=""
    fi

    str_opt="--codec av1_dec -i ${stream_path} -c ${max_frames} $g_refc_frame_num"

    if [ "$g_scaler" == "true" ]; then
        str_opt="$str_opt -o output_orig.yuv"
    else
        str_opt="$str_opt -o $output_path"
    fi

    if [ $g_enable_thumbnail -eq 1 ]; then
        str_opt="${str_opt} --thumbnail"
    fi

    if [ $g_match_mode -eq 2 ]; then
        if [ $wtl_mode_index -ne 5 ]; then
            wtl_mode_index=1
        fi
        if [ $is_main10 -eq 1 ] && [ $g_enable_wtl -eq 0 ]; then
            wtl_mode_index=1
        fi
        str_opt="${str_opt} -5 -y ${wtl_mode_index}"
    elif [ $g_match_mode -eq 3 ]; then
        if [ $g_wtl_format -eq 7 ] || [ $g_wtl_format -eq 8 ] ; then
            wtl_mode_index=5
        fi
        str_opt="${str_opt} -y ${wtl_mode_index}"
    else
        if [ "$is_main10" == "1" ]; then
            wtl_mode_index=$(get_refc_wtl_idx $fmt)
            if [ $g_scaler == "true" ] && [ $g_enable_csc == "false" ]; then
                wtl_mode_index=5
            fi
        fi
        str_opt="${str_opt} -y ${wtl_mode_index}"
    fi

    str_cmd="${bin_dir}/av1_dec ${str_opt} $stream_format_param"
    echo "${str_cmd}"
    ${str_cmd}

    if [ "$?" != "0" ]; then
        echo "Failed to ref-c"
        return 1;
    fi


    parse_dec_info_file "display_info.txt" #set g_oriw/g_orih from display_info.txt


    if [ $g_scaler == "true" ]; then
        local fmt_name="${g_yuv_fmt_list[$g_wtl_format]}"
        local padder_opt=0
        local scaler_input="output_orig.yuv"
        if [ "$is_main10" == "1" ]; then
            padder_opt=1
        fi
        if (( $g_sclw == 0 && $g_sclh == 0 )); then
            echo "[WARN] scaled down width : $g_sclw height : $g_sclh"
        else
            yuv_8align_padding $scaler_input "output_padded.yuv" "$padder_opt"
            scaler_input="output_padded.yuv"
            g_oriw=$(ceiling $g_oriw 8)
            g_orih=$(ceiling $g_orih 8)
        fi
        run_refc_scaler $scaler_input "${output_path}" "$fmt_name"
        if [ "$?" != "0" ]; then
            echo "Failed to ref-c"
            return 1;
        fi
    fi

    return 0
}

function run_refc_dec_h265 {
    local stream_path="$1"
    local output_path="$2"
    local wtl_mode_index=0
    local skip_option=0
    local max_frames=""
    local file_ext=${stream_path##*.}
    local fmt="${g_yuv_fmt_list[$g_wtl_format]}"
    local str_opt

    debug_msg "run decoder c-model.. param : $stream_path, $output_path"

    if [ "$file_ext" = "mp4" ] || [ "$file_ext" = "mkv" ] || [ "$file_ext" = "avi" ]; then
        cp ${bin_dir}/../../../util/bin/Linux/vcp ${bin_dir} || echo ingore_err
        ${bin_dir}/vcp -i $stream_path output.bin
        stream_path="output.bin"
    fi

    if [ "$g_refc_frame_num" == "-m 0" ]; then
        g_refc_frame_num=""
    fi

    str_opt="-i ${stream_path} $max_frames $g_refc_frame_num"

    if [ "$g_scaler" == "true" ]; then
        str_opt="$str_opt -o output_orig.yuv"
    else
        str_opt="$str_opt -o $output_path"
    fi

    str_opt="${str_opt} -c" #crop function disabled

    if [ $g_enable_thumbnail -eq 1 ]; then
        str_opt="${str_opt} --thumbnail"
    fi

    if [ $g_match_mode -eq 2 ]; then
        if [ $wtl_mode_index -ne 5 ]; then
            wtl_mode_index=1
        fi
        if [ $is_main10 -eq 1 ] && [ $g_enable_wtl -eq 0 ]; then
            wtl_mode_index=1
        fi
        str_opt="${str_opt} -5 -y ${wtl_mode_index}"
    elif [ "$g_match_mode" == "3" ]; then
        if [ "$is_main10" == "1" ]; then
            wtl_mode_index=5
        fi
        str_opt="${str_opt} -y ${wtl_mode_index}"
    else
        if [ "$is_main10" == "1" ]; then
            wtl_mode_index=$(get_refc_wtl_idx $fmt)
            if [ $g_scaler == "true" ] && [ $g_enable_csc == "false" ]; then
                wtl_mode_index=5
            fi
        fi
        str_opt="${str_opt} -y ${wtl_mode_index}"
    fi

    str_cmd="${bin_dir}/hevc_dec --codec hevc_dec ${str_opt} --sbs 46080"
    echo "${str_cmd}"
    ${str_cmd}

    if [ "$?" != "0" ]; then
        echo "Failed to ref-c"
        return 1;
    fi


    parse_dec_info_file "display_info.txt" #set g_oriw/g_orih from display_info.txt


    if [ $g_scaler == "true" ]; then
        local fmt_name="${g_yuv_fmt_list[$g_wtl_format]}"
        local scaler_input_path="output_orig.yuv"
        run_refc_scaler "${scaler_input_path}" "${output_path}" "$fmt_name"
        if [ "$?" != "0" ]; then
            echo "Failed to ref-c"
            return 1;
        fi
    fi

    return 0
}

function run_refc_dec_vp9 {
    local stream_path="$1"
    local output_path="$2"
    local arg_cci=""
    local wtl_mode_index=0
    local max_frames=""
    local fmt="${g_yuv_fmt_list[$g_wtl_format]}"

    debug_msg "run decoder c-model.. param : $stream_path, $output_path"

    if [ $g_match_mode -eq 2 ]; then
        if [ $wtl_mode_index -ne 0 ]; then
            wtl_mode_index=5    # 16BIT MSB
        fi
    elif [ "$g_match_mode" == "3" ]; then
        if [ "$is_main10" == "1" ]; then
            wtl_mode_index=5
        fi
    else #yuv comp
        if [ "$is_main10" == "1" ]; then
            wtl_mode_index=$(get_refc_wtl_idx $fmt)
            if [ $g_scaler == "true" ] && [ $g_enable_csc == "false" ]; then
                wtl_mode_index=5
            fi
        fi
    fi

    if [ "$g_refc_frame_num" == "-m 0" ]; then
        g_refc_frame_num=""
    fi

    param="-i ${stream_path} --codec vp9_dec -y $wtl_mode_index"
    param="${param} -c" #crop function disabled


    if [ "$g_scaler" == "true" ]; then
        param="$param -o output_orig.yuv"
    else
        param="$param -o $output_path"
    fi

    if [ $g_enable_thumbnail -eq 1 ]; then
        param="${param} --thumbnail"
    fi

    file_ext=${stream_path##*.}

    if [ "$file_ext" == "ivf" ] || [ "$file_ext" == "vp9" ]; then
        param="$param --ivf"
    fi

    param="$param $g_refc_frame_num"

    str_cmd="${bin_dir}/vp9_dec ${param}"

    echo "${str_cmd}"
    ${str_cmd}

    if [ "$?" != "0" ]; then
        echo "Failed to ref-c"
        return 1;
    fi


    parse_dec_info_file "display_info.txt" #set g_oriw/g_orih from display_info.txt


    if [ $g_scaler == "true" ]; then
        if [ -z "$is_main10" ]; then
            is_main10="0"
        fi
        local fmt_name="${g_yuv_fmt_list[$g_wtl_format]}"
        local scaler_input_path="output_orig.yuv"
        run_refc_scaler "${scaler_input_path}" "${output_path}" "$fmt_name"
        if [ "$?" != "0" ]; then
            echo "Failed to ref-c"
            return 1;
        fi
    fi

    return 0
}

function run_refc_dec_avs2 {
    local stream_path="$1"
    local output_path="$2"
    local output_param=""
    local wtl_mode_index=0
    local refc_bin=""
    local fmt="${g_yuv_fmt_list[$g_wtl_format]}"

    debug_msg "run decoder c-model.. param : $stream_path, $output_path"

    if [ "$g_scaler" == "true" ]; then
        output_param="-o output_orig.yuv"
    else
        output_param="-o $output_path"
    fi

    if [ "$g_refc_frame_num" == "-m 0" ]; then
        g_refc_frame_num=""
    fi

    if [ "$g_product_name" = "wave517" ] || [ "$g_product_name" = "wave537" ]; then
        refc_bin="avs2_dec"
        param="-i ${stream_path} $output_param --codec avs2_dec"
        if [ $g_match_mode -eq 2 ]; then
            if [ $wtl_mode_index -ne 0 ]; then
                wtl_mode_index=5    # 16BIT LSB
            fi
            param="$param -5"
        elif [ $g_match_mode -eq 3 ]; then
            if [ $g_wtl_format -eq 7 ] || [ $g_wtl_format -eq 8 ] ; then
                wtl_mode_index=5
            fi
            str_opt="-y ${wtl_mode_index} ${output_param}"
        else #yuv comp
            if [ "$is_main10" == "1" ]; then
                wtl_mode_index=$(get_refc_wtl_idx $fmt)
            if [ $g_scaler == "true" ] && [ $g_enable_csc == "false" ]; then
                wtl_mode_index=5
            fi
            fi
        fi
        param="$param $g_refc_frame_num -y $wtl_mode_index"
        if [ $g_enable_thumbnail -eq 1 ]; then
            param="${param} --thumbnail"
        fi
    else
        refc_bin="avs2_dec_bwb"
        if [ $g_match_mode -eq 2 ]; then
            if [ $wtl_mode_index -ne 0 ]; then
                wtl_mode_index=5    # 16BIT LSB
            fi
            param="--input=${stream_path} --outmd5=${output_path} --render 0 --codec $g_codec_index --stream-endian=$g_stream_endian --frame-endian=$g_frame_endian"
        else
            param="--input=${stream_path} --output=${output_path} --render 0 --codec $g_codec_index --stream-endian=$g_stream_endian --frame-endian=$g_frame_endian"
        fi

        param="$param $g_refc_frame_num "

        if [ $g_enable_wtl -eq 0 ]; then
            param="${param} --disable-wtl"
        else
            if [ $g_scaler == "true" ] && [ $is_main10 -eq 1 ]; then
                param="${param} --wtl-format=5"
            else
            param="${param} --wtl-format=${wtl_mode_index}"
            fi
        fi

        if [ $g_cbcr_interleave -eq 1 ]; then
            param="${param} --enable-cbcrinterleave"
            if [ $g_enable_nv21 -eq 1 ]; then
                param="${param} --enable-nv21"
            fi
        fi

        if [ $g_enable_thumbnail -eq 1 ]; then
            param="$param --enable-thumbnail"
        fi
    fi

    str_cmd="${bin_dir}/${refc_bin} ${param}"
    echo "${str_cmd}"
    ${str_cmd}

    if [ "$?" != "0" ]; then
        echo "Failed to ref-c"
        return 1;
    fi


    parse_dec_info_file "display_info.txt" #set g_oriw/g_orih from display_info.txt


    if [ $g_scaler == "true" ]; then
        local fmt_name="${g_yuv_fmt_list[$g_wtl_format]}"
        local padder_opt=0
        local scaler_input="output_orig.yuv"
        if [ "$is_main10" == "1" ]; then
            padder_opt=1
        fi
        yuv_8align_padding $scaler_input "output_padded.yuv" "$padder_opt"
        scaler_input="output_padded.yuv"
        run_refc_scaler $scaler_input "${output_path}" "$fmt_name"
        if [ "$?" != "0" ]; then
            echo "Failed to ref-c"
            return 1;
        fi
    fi

    return 0
}

function run_refc_dec_svac {
    local stream_path="$1"
    local output_path="$2"
    local wtl_mode_index=0
    local output_param=""
    local file_ext=${stream_path##*.}

    debug_msg "run decoder c-model.. param : $stream_path, $output_path"

    if [ "$file_ext" = "mp4" ] || [ "$file_ext" = "mkv" ] || [ "$file_ext" = "avi" ]; then
        cp ${bin_dir}/../../../util/bin/Linux/vcp ${bin_dir} || echo ingore_err
        ${bin_dir}/vcp -i $stream_path output.bin
        stream_path="output.bin"
    fi

    # Not implemented yet
    if [ $g_enable_thumbnail -eq 1 ]; then
        skip_option=1
    fi

    case $g_wtl_format in
    0)  # 8BIT 420
        wtl_mode_index=0;;
    5)  # 16bit MSB
        wtl_mode_index=1;;
    6)  # 16bit LSB
        wtl_mode_index=5;;
    7)  # 32bit MSB
        wtl_mode_index=2;;
    8)  # 32bit LSB
        wtl_mode_index=6;;
    *) echo "Not supported WTL format: $g_wtl_format"
       exit 1
    esac
    output_param="-i ${stream_path} -o $output_path $g_refc_frame_num"

    if [ $g_match_mode -eq 2 ]; then
        if [ $wtl_mode_index -ne 0 ]; then
            wtl_mode_index=1
        fi
        str_opt="-5 -y ${wtl_mode_index} ${output_param}"
    else
        str_opt="-y ${wtl_mode_index} ${output_param}"
    fi

    str_cmd="${bin_dir}/svac_dec ${str_opt}"
    echo "${str_cmd}"
    ${str_cmd}

    if [ "$?" != "0" ]; then
        echo "Failed to ref-c"
        return 1;
    fi
    return 0
}

function run_refc_dec_avc {
    local stream_path="$1"
    local output_path="$2"
    local wtl_mode_index=0
    local file_ext=${stream_path##*.}
    local fmt="${g_yuv_fmt_list[$g_wtl_format]}"

    debug_msg "run decoder c-model.. param : $stream_path, $output_path"

    if [ "$file_ext" = "mp4" ] || [ "$file_ext" = "mkv" ] || [ "$file_ext" = "avi" ]; then
        cp ${bin_dir}/../../../util/bin/Linux/vcp ${bin_dir} || echo ingore_err
        ${bin_dir}/vcp -i $stream_path output.bin
        stream_path="output.bin"
    fi

    if [ "$g_refc_frame_num" == "-m 0" ]; then
        g_refc_frame_num=""
    fi

    str_opt="--codec avc_dec -i ${stream_path} $g_refc_frame_num"
    str_opt="${str_opt} -c" #crop function disabled

    if [ $g_enable_thumbnail -eq 1 ]; then
        str_opt="${str_opt} --thumbnail"
    fi

    if [ "$g_scaler" == "true" ]; then
        str_opt="$str_opt -o output_orig.yuv"
    else
        str_opt="$str_opt -o $output_path"
    fi

    if [ $g_match_mode -eq 2 ]; then
        if [ $wtl_mode_index -ne 5 ]; then
            wtl_mode_index=1
        fi
        if [ $is_main10 -eq 1 ] && [ $g_enable_wtl -eq 0 ]; then
            wtl_mode_index=1
        fi
        str_opt="${str_opt} -5 -y ${wtl_mode_index}"
    elif [ "$g_match_mode" == "3" ]; then
        if [ "$is_main10" == "1" ]; then
            wtl_mode_index=5
        fi
        str_opt="${str_opt} -y ${wtl_mode_index}"
    else #yuv comp
        if [ "$is_main10" == "1" ]; then
            wtl_mode_index=$(get_refc_wtl_idx $fmt)
            if [ $g_scaler == "true" ] && [ $g_enable_csc == "false" ]; then
                wtl_mode_index=5
            fi
        fi
        str_opt="${str_opt} -y ${wtl_mode_index}"
    fi

    str_cmd="${bin_dir}/avc_dec ${str_opt}"
    echo "${str_cmd}"

    ${str_cmd}
    if [ "$?" != "0" ]; then
        echo "Failed to ref-c"
        return 1;
    fi


    parse_dec_info_file "display_info.txt" #set g_oriw/g_orih from display_info.txt


    if [ $g_scaler == "true" ]; then
        local fmt_name="${g_yuv_fmt_list[$g_wtl_format]}"
        local scaler_input_path="output_orig.yuv"
        run_refc_scaler "${scaler_input_path}" "${output_path}" "$fmt_name"
        if [ "$?" != "0" ]; then
            echo "Failed to ref-c"
            return 1;
        fi
    fi
    return 0
}

function run_refc_dec_coda9 {
    local stream_path=$1
    local output_yuv=$2
    local ext=${stream_path##*.}

    debug_msg "run decoder c-model.. param : $stream_path, $output_yuv"

    if [ "$ext" = "rmvb" ]; then
        stream_path=${stream_path%.*}.rvf
    fi

    cmodel_param="-i $stream_path -o $output_yuv"
    ext_param=""
    shopt -s nocasematch
    case "$codec" in
        avc) if [ $g_enable_mvc -eq 1 ]; then
                cmodel="RefMvcDec"
             else
                cmodel="RefAvcDec"
             fi
             ;;
        vc1) cmodel="RefVc1Dec";;
        mp2) cmodel="RefMp2Dec";;
        mp4) cmodel="RefMp4Dec"
             cmodel_param="$cmodel_param -s $g_mp4class";;
        avs) cmodel="RefAvsDec";;
        dv3) cmodel="RefDivDec"
             cmodel_param="$cmodel_param -E host_div3_rtl_cmd.org"
             if [ "$ext" = "ivf" ]; then
                ext_param="-v"
             fi
             ;;
        rvx) cmodel="RefRvxDec";;
        vp8) cmodel="RefVpxDec"
             cmodel_param="$cmodel_param --std 2";;
        tho) cmodel="RefThoDec"
             cmodel_param="$cmodel_param --make_stream --stream_filename temp/host_tho_rtl_cmd.org --stream_endian $g_stream_endian";;
        *)    echo ""; return;;
    esac
    shopt -u nocasematch


    ppu_option=0
    if [ $g_match_mode -eq 1 ]; then
        if [ $g_rotAngle -ne 0 ]; then
            local index=$(($g_rotAngle/90))
            ppu_option=$((0x10 | $index))
        fi
        if [ $g_mirDir -ne 0 ]; then
            ppu_option=$(($ppu_option | 0x10 | ($g_mirDir<<2)))
        fi
        if [ $ppu_option -gt 0 ]; then
            cmodel_param="$cmodel_param -g $ppu_option"
        fi
    fi
    if [ $g_enable_dering -eq 1 ]; then
        cmodel_param="$cmodel_param -p"
    fi
    if [ $g_enable_deblock -eq 1 ]; then
        cmodel_param="$cmodel_param -d"
    fi

    exec_cmd="${bin_dir}/$cmodel $cmodel_param -l $ext_param -c"
    echo "$exec_cmd"

    $exec_cmd

    return $?
}

function run_refc_enc_coda9 {
    local val=0
    local input=$1
    local output=$2
    local postfix=""

    if [ "$g_product_name" = "coda980" ]; then
        postfix="_980_rev"
    fi

    refc_exe="$bin_dir"
    case $codec in
    avc)
        if [ $g_enable_mvc -eq 0 ]; then
            refc_exe="$refc_exe/RefAvcEnc$postfix"
        else
            refc_exe="$refc_exe/RefMvcEnc$postfix"
        fi
        ;;
    mp4)
        refc_exe="$refc_exe/RefMp4Enc$postfix" ;;
    *)
        echo "run_refc_enc: Unknown codec $codec"
        return 1;;
    esac

    refc_param="-i $input -o $output -p $yuv_dir/"

    val=$(($g_rotAngle/90))
    val=$(($val | ($g_mirDir<<2)))
    if [ $val -ne 0 ]; then
        refc_param="$refc_param -g $val"
    fi

    refc_param=$(echo $refc_param | sed -e 's/\\/\//g')

    cmd="$refc_exe $refc_param"
    echo $cmd
    echo ""

    $cmd

    return $?
}

function generate_golden_data_path {
    local codec=$1
    local streamset=$2
    local streamfile=$3

    codec_dir=""
    shopt -s nocasematch
    case "$codec" in
        vp9)  codec_dir="vp9dec";;
        hevc) codec_dir="hvcdec";;
        avc)  codec_dir="avcdec";;
        vc1)  codec_dir="vc1dec";;
        mp2)  codec_dir="mp2dec";;
        mp4)  codec_dir="mp4dec";;
        h263) codec_dir="mp4dec";;
        avs)  codec_dir="avsdec";;
        dv3)  codec_dir="dv3dec";;
        rvx)  codec_dir="rvxdec";;
        vp8)  codec_dir="vp8dec";;
        tho)  codec_dir="thodec";;
        avs2) codec_dir="avs2dec";;
        av1)  codec_dir="av1dec";;
        *)    codec_dir="";;
    esac
    shopt -u nocasematch

    if [ "$is_main10" = "" ]; then is_main10=0; fi

    yuvext=""
    if [ "$g_enable_wtl" == "1" ]; then
        case "$codec" in
        hevc | vp9 | avs2 | svac | avc | av1 )
            if [ $is_main10 -eq 1 ]; then
                if [ $g_wtl_format -eq 0 ]; then
                    yuvext="_bwb_8"
                elif [ $g_wtl_format -eq 1 ]; then
                    # FORMAT_422
                    yuvext="_bwb_8"
                elif [ $g_wtl_format -gt 8 ] && [ $g_wtl_format -lt 13 ]; then
                    # FORMAT_422_16BIT_XXX
                    yuvext=""
                elif [ $g_wtl_format -ge 13 ] && [ $g_wtl_format -le 17 ]; then
                    if [ $g_wtl_format -eq 13 ]; then
                        yuvext="_yuyv_8"
                    else
                        yuvext="_yuyv"
                    fi
                elif [ $g_wtl_format -ge 18 ] && [ $g_wtl_format -le 22 ]; then
                    if [ $g_wtl_format -eq 18 ]; then
                        yuvext="_yvyu_8"
                    else
                        yuvext="_yvyu"
                    fi
                elif [ $g_wtl_format -ge 23 ] && [ $g_wtl_format -le 27 ]; then
                    if [ $g_wtl_format -eq 23 ]; then
                        yuvext="_uyvy_8"
                    else
                        yuvext="_uyvy"
                    fi
                elif [ $g_wtl_format -ge 28 ] && [ $g_wtl_format -le 32 ]; then
                    if [ $g_wtl_format -eq 28 ]; then
                        yuvext="_vyuy_8"
                    else
                        yuvext="_vyuy"
                    fi
                fi
            else
                if [ $g_wtl_format -eq 1 ]; then
                    yuvext=""
                elif [ $g_wtl_format -eq 13 ]; then
                    yuvext="_yuyv_8"
                elif [ $g_wtl_format -eq 18 ]; then
                    yuvext="_yvyu_8"
                elif [ $g_wtl_format -eq 23 ]; then
                    yuvext="_uyvy_8"
                elif [ $g_wtl_format -eq 28 ]; then
                    yuvext="_vyuy_8"
                fi
            fi
            ;;
        *)
            ;;
        esac
    elif [ $g_afbce -eq 1 ]; then
        case "$codec" in
        hevc | vp9)
            if [ $is_main10 -eq 1 ]; then
                yuvext="_bwb_8"
            fi
        esac
    fi


    # CODA9xx & CODA7x features
    local dering_str=""
    if [ $g_enable_dering -eq 1 ]; then
        dering_str="_dr"
    fi
    local deblk_str=""
    if [ $g_enable_deblock -eq 1 ]; then
        deblk_str="_dbk"
    fi
    # END CODA9xx & CODA7x

    streamset_name=$(basename $streamset)
    filename=$(basename $streamfile)

    local target_base="$yuv_dir/$codec_dir"
    local target_path=""

    thumbnail_ext=""
    if [ $g_enable_thumbnail -eq 1 ]; then
        thumbnail_ext="_thumb"
    fi

    local wtl_format_ext=""
    case $g_wtl_format in
        0) wtl_format_ext="";;
        5) wtl_format_ext="_16bit_msb";;
        6) wtl_format_ext="_16bit_lsb";;
        7) wtl_format_ext="_32bit_msb";;
        8) wtl_format_ext="_32bit_lsb";;
    esac

    local rot_index=0
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
    if [ $g_mirDir -eq 1 ] && [ $g_rotAngle -eq 0 ]; then
        rot_index=4
    fi
    if [ $g_rotAngle -eq 180 ] && [ $g_mirDir -eq 2 ]; then
        rot_index=4
    fi
    if [ $g_mirDir -eq 1 ] && [ $g_rotAngle -eq 90 ]; then
        rot_index=5
    fi
    if [ $g_mirDir -eq 2 ] && [ $g_rotAngle -eq 270 ]; then
        rot_index=5
    fi
    if [ $g_mirDir -eq 1 ] && [ $g_rotAngle -eq 180 ]; then
        rot_index=6
    fi
    if [ $g_mirDir -eq 2 ] && [ $g_rotAngle -eq 0 ]; then
        rot_index=6
    fi
    if [ $g_mirDir -eq 1 ] && [ $g_rotAngle -eq 270 ]; then
        rot_index=7
    fi
    if [ $g_mirDir -eq 2 ] && [ $g_rotAngle -eq 90 ]; then
        rot_index=7
    fi

    local target_ext="yuv"

    if [ "$streamset_name" = "" ]; then
        target_path="${target_base}/${filename}${wtl_format_ext}${deblk_str}${thumbnail_ext}${yuvext}_${rot_index}.$target_ext"
    else
        # ex) xxxx_01.cmd -> xxxx.cmd
        temp=`expr match "$streamset_name" '.*\(_[0-9]*\.cmd\)'`
        if [ "$temp" != "" ]; then
            streamset_name="${streamset_name%_[0-9]*\.cmd}.cmd"
        fi
        target_base="${target_base}/${streamset_name}"
        target_path="${target_base}/${filename}${wtl_format_ext}${deblk_str}${dering_str}${thumbnail_ext}${yuvext}_${rot_index}.$target_ext"
    fi

    if [ ! -d $target_base ]; then
        mkdir -p $target_base
    fi

    g_func_ret_str="$target_path"
}

# execute the ref-c program to get the decoded image or md5
# generate_yuv(stream_path, output_yuv_path)
function generate_yuv {
    local src=$1
    local output=$2
    local fmt_name="${g_yuv_fmt_list[$g_wtl_format]}"
    local mode_idx=0

    case "$codec" in
        av1)
            echo "run_refc_dec_av1 $src $output"
            run_refc_dec_av1 $src $output;;
        vp9)
            echo "run_refc_dec_vp9 $src $output"
            run_refc_dec_vp9 $src $output;;
        hevc)
            echo "run_refc_dec_h265 $src $output"
            run_refc_dec_h265 $src $output;;
        avs2)
            echo "run_refc_dec_avs2 $src $output"
            run_refc_dec_avs2 $src $output;;
        svac)
            echo "run_refc_dec_svac $src $output"
            run_refc_dec_svac $src $output;;
        avc)
            if [[ $g_product_name =~ coda9* ]]; then
                echo "run_refc_dec_coda9 $src $output $codec"
                run_refc_dec_coda9 $src $output $codec
            else
                echo "run_refc_dec_avc $src $output"
                run_refc_dec_avc $src $output
            fi
            ;;
        *)
            echo "run_refc_dec_coda9 $src $output $codec"
            run_refc_dec_coda9 $src $output $codec
            ;;
    esac

    if (( $g_match_mode == 1 )); then
        if [ "$g_scaler" != "true" ] && [ "$g_pvric" = "0" ]; then
            if [ "$g_enable_csc" = "true" ]; then
                run_bwb_converter_for_rgb $CSC_RGB444_OUTPUT_PATH $output $fmt_name
            else
                run_bwb_converter $output "bwb_out.yuv" $fmt_name $mode_idx
                if [ -e "./bwb_out.yuv" ];then
                    echo "cp bwb_out.yuv $output"
                    cp bwb_out.yuv $output
                fi
            fi
        fi
    fi

    if [ $? != 0 ]; then
        return 1;
    fi
}

# generate_md5(stream_path, md5_path)
function generate_md5 {
    local src=$1
    local _output_path=$2

    local run=0

    echo "dirname ${_output_path}"
    basedir=`dirname ${_output_path}`
    mkdir -p "${basedir}"

    [ -e $_output_path ] || run=1

    if [ $run -eq 1 ]; then
        case "$codec" in
        vp9)
            run_refc_dec_vp9 ${src} md5_result.txt ;;     # generate linear md5
        hevc)
            run_refc_dec_h265 ${src} md5_result.txt ;;    # generate linear md5
        svac)
            run_refc_dec_svac ${src} md5_result.txt ;;    # generate linear md5
        avs2)
            run_refc_dec_avs2 ${src} md5_result.txt ;;    # generate linear md5
        avc)
            if [[ $g_product_name =~ coda9* ]]; then
                run_refc_dec_coda9 $src md5_result.txt $codec
                local npath=${_output_path%_[0-7].md5}
                chmod 666 md5_result.txt_*.md5
                for (( i=0; i<=7; i++ )); do
                    cp --preserve=mode md5_result.txt_${i}.md5 ${npath}_${i}.md5
                done
                rm md5_result.txt_*.md5
                return
            else
                run_refc_dec_avc  ${src} md5_result.txt     # generate linear md5
            fi
            ;;
        *)
            run_refc_dec_coda9 $src md5_result.txt $codec
            local npath=${_output_path%_[0-7].md5}
            chmod 666 md5_result.txt_*.md5
            for (( i=0; i<=7; i++ )); do
                cp --preserve=mode md5_result.txt_${i}.md5 ${npath}_${i}.md5
            done
            rm md5_result.txt_*.md5
            return
            ;;
        esac
        if [ "$?" != "0" ]; then
            return 1;
        fi
        chmod 666 *.txt
        echo "cp --preserve=mode md5_result.txt $_output_path"
        cp --preserve=mode md5_result.txt $_output_path
        rm md5_result.txt
    fi
}

function yuv_8align_padding {
    local padding_bin_path="../../../design/util/bin/Linux/padder_8x8"
    local input_path=$1
    local output_path=$2
    local padder_opt=$3
    local exe_cmd=""
    local disp_info="display_info.txt"

    exe_cmd="$padding_bin_path $input_path $output_path $disp_info $padder_opt"
    echo "$exe_cmd"

    $exe_cmd

    return $?
}



###############################################################################
# ENCODER FUNCTIONS
###############################################################################
# void c9_parse_cfg(cfg_path, &w, &h, &field_flag)
# desc: parse cfg file to get width, height and field_flag information
function c9_parse_cfg {
    local _cfg_path="$1"
    local _width=$2
    local _height=$3
    local _is_field=$4
    local _name=""
    local _firstchar=""

    shopt -s nocasematch
    while read line; do
    _firstchar=${line:0:1}
    case "$_firstchar" in
        "#") continue;;     # comment
        ";") continue;;     # comment
        "")  continue;;
    esac
    name=$(echo $line | cut -d' ' -f1)
    if [ "$name" = "PICTURE_WIDTH" ]; then
        eval $_width=$(echo $line | cut -d' ' -f2)
    elif [ "$name" = "PICTURE_HEIGHT" ]; then
        eval $_height=$(echo $line | cut -d' ' -f2)
    elif [ "$name" = "INTERLACED_PIC" ]; then
        eval $_is_field=$(echo $line | cut -d' ' -f2)
    fi
    done < $_cfg_path
    shopt -u nocasematch
}



