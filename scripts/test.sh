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

test_press_perf()
{
    cons_press="rm -f press.log*; ./test_consumer_reload_press_zlog 0 200 50 test_consumer_press_zlog.conf"
    norm_pess="rm -f press.log*; ./test_consumer_reload_press_zlog 0 200 50 test_press_zlog.conf"

    cd build/bin
    perf_wrap "$cons_press" profile_cons.svg
    perf_wrap "$norm_pess" profile.svg
    cd -
}

test_multi_thread()
{
    cd build/bin
    rm -f zlog.txt*
    eval "$asan_pre ./test_dzlog_conf -f test_consumer_static_file_single.conf -n 10 -m 10"
    cd -
}

asan_pre=""
while getopts "t:a" opt; do
  case $opt in
    t)
      testname="$OPTARG"
      ;;
    a)
      asan_pre="LD_PRELOAD=/usr/lib/gcc/x86_64-linux-gnu/11/libasan.so ASAN_OPTIONS=detect_leaks=1"
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
  esac
done

$testname
