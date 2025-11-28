#!/bin/bash

cd build/bin; rm -f press.log*;\
    sudo perf record -F 999 -g -- ./test_consumer_reload_press_zlog 0 200 10000 test_consumer_press_zlog.conf; \
    sudo chmod 777 perf.data ; perf script | /local/mnt/workspace/project/FlameGraph/stackcollapse-perf.pl | \
        /local/mnt/workspace/project/FlameGraph/flamegraph.pl > profile_cons.svg; \
    sudo chmod 777 press.log*; \
    cd -

cd build/bin; rm -f press.log*;\
    sudo perf record -F 999 -g -- ./test_consumer_reload_press_zlog 0 200 10000 test_press_zlog.conf; \
    sudo chmod 777 perf.data ; perf script | /local/mnt/workspace/project/FlameGraph/stackcollapse-perf.pl | \
        /local/mnt/workspace/project/FlameGraph/flamegraph.pl > profile.svg; \
    sudo chmod 777 press.log*; \
    cd -
