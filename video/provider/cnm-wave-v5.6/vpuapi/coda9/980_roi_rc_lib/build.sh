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

echo "-----------------"
echo "build start"
echo "-----------------"
make clean
make 

echo "-----------------"
echo "build arm start"
echo "-----------------"
make BUILD_CONFIGURATION=EmbeddedLinux clean
make BUILD_CONFIGURATION=EmbeddedLinux

echo "-----------------"
echo "build nonos start"
echo "-----------------"
make BUILD_CONFIGURATION=NonOS clean 
make BUILD_CONFIGURATION=NonOS

