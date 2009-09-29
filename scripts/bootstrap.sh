#!/bin/bash
#
# This file is a part of LEMON, a generic C++ optimization library.
#
# Copyright (C) 2003-2009
# Egervary Jeno Kombinatorikus Optimalizalasi Kutatocsoport
# (Egervary Research Group on Combinatorial Optimization, EGRES).
#
# Permission to use, modify and distribute this software is granted
# provided that this copyright notice appears in all copies. For
# precise terms see the accompanying LICENSE file.
#
# This software is provided "AS IS" with no warranty of any kind,
# express or implied, and with no claim as to its suitability for any
# purpose.


if [ ! -f ~/.lemon-bootstrap ]; then
    echo 'Create ~/.lemon-bootstrap'.
    cat >~/.lemon-bootstrap <<EOF
#
# Default settings for bootstraping the LEMON source code repository
#
EOF
fi

source ~/.lemon-bootstrap
if [ -f ../../../.lemon-bootstrap ]; then source ../../../.lemon-bootstrap; fi
if [ -f ../../.lemon-bootstrap ]; then source ../../.lemon-bootstrap; fi
if [ -f ../.lemon-bootstrap ]; then source ../.lemon-bootstrap; fi
if [ -f ./.lemon-bootstrap ]; then source ./.lemon-bootstrap; fi


function augment_config() { 
    if [ "x${!1}" == "x" ]; then
        eval $1=$2
        echo Add "'$1'" to '~/.lemon-bootstrap'.
        echo >>~/.lemon-bootstrap
        echo $3 >>~/.lemon-bootstrap
        echo $1=$2 >>~/.lemon-bootstrap
    fi
}

augment_config LEMON_INSTALL_PREFIX /usr/local \
    "# LEMON installation prefix"

augment_config COIN_OR_PREFIX /usr/local/coin-or \
    "# COIN-OR installation root prefix (used for CLP/CBC)"


function ask() {
echo -n "$1 [$2]? "
read _an
if [ "x$_an" == "x" ]; then
    ret="$2"
else
    ret=$_an
fi
}

function yesorno() {
    ret='rossz'
    while [ "$ret" != "y" -a "$ret" != "n" -a "$ret" != "yes" -a "$ret" != "no" ]; do
        ask "$1" "$2"
    done
    if [ "$ret" != "y" -a "$ret" != "yes" ]; then
        return 1
    else
        return 0
    fi
}

if yesorno "External build" "n"
then
    CONFIGURE_PATH=".."
else
    CONFIGURE_PATH="."
    if yesorno "Autoreconf" "y"
    then
        AUTORE=yes
    else
        AUTORE=no
    fi
fi

if yesorno "Optimize" "n" 
then
    opt_flags=' -O2'
else
    opt_flags=''
fi

if yesorno "Stop on warning" "y" 
then
    werror_flags=' -Werror'
else
    werror_flags=''
fi

cxx_flags="CXXFLAGS=-ggdb$opt_flags$werror_flags"

if yesorno "Use COIN" "n"
then
    coin_flag="--with-coin=$COIN_OR_PREFIX"
else
    coin_flag=""
fi


if [ "x$AUTORE" == "xyes" ]; then
    autoreconf -vif;
fi
${CONFIGURE_PATH}/configure --prefix=$LEMON_INSTALL_PREFIX \
"$cxx_flags" \
$coin_flag \
$*
