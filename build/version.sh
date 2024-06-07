#!/bin/bash

pushd "$(dirname "$0")" >/dev/null || exit 1

if command -v git >/dev/null 2>&1 ; then
    gitver="1.6.0"
    SO_VERSION=${gitver%%-*}
    SO_NAME=${SO_VERSION%.*}
fi

rm -f version.mak
if [ -n "${SO_VERSION}" ] ; then
    echo "SO_VERSION=\".$SO_VERSION\"" > version.mak
    echo "SO_NAME=\".$SO_NAME\"" >> version.mak
fi
popd >/dev/null
