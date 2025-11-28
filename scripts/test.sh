#!/bin/bash

perf_wrap()
{
    rm -f $2
    sudo perf record -F 999 -g -- $1
    sudo chmod 777 perf.data
    perf script | /local/mnt/workspace/project/FlameGraph/stackcollapse-perf.pl | \
        /local/mnt/workspace/project/FlameGraph/flamegraph.pl > $2
    sudo chmod 777 press.log*
}

cons_press="rm -f press.log*; ./test_consumer_reload_press_zlog 0 200 50 test_consumer_press_zlog.conf"

norm_pess="rm -f press.log*; ./test_consumer_reload_press_zlog 0 200 50 test_press_zlog.conf"

cd build/bin
perf_wrap "$cons_press" profile_cons.svg
perf_wrap "$norm_pess" profile.svg
cd -
