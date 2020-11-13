#!/usr/bin/env bash

MOD_NPCCODEBOX_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )/" && pwd )"

source $MOD_NPCCODEBOX_ROOT"/conf/conf.sh.dist"

if [ -f $MOD_NPCCODEBOX_ROOT"/conf/conf.sh" ]; then
    source $MOD_NPCCODEBOX_ROOT"/conf/conf.sh"
fi 