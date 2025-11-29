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
    rm -f zlog.txt*
    eval "$asan_pre ./test_dzlog_conf -f test_consumer_static_file_single.conf -n 10 -m 10 --threadN=10"
}

test_multi_thread_record()
{
    rm -f zlog.txt*
    eval "$asan_pre ./test_dzlog_conf -f test_consumer_static_file_single.conf -n 10 -m 10 --threadN=10 -r"
}

test_multi_thread_reload()
{
    rm -f zlog.txt*
    cmd="$valgrind_cmd $asan_pre ./test_dzlog_conf -f test_consumer_static_file_single.conf -n 1000 -m 10 --threadN=10 --reloadcnt=10 --reloadms=400 \
        -l test_consumer_static_file_single.conf \
        -l test_consumer_static_file_single.conf \
        -l test_static_file_single.conf \
        -l test_dynamic_file.conf"
    echo "run cmd:"
    echo $cmd
    eval $cmd
}

test_multi_thread_recordms()
{
    rm -f zlog.txt*
    eval "$asan_pre ./test_dzlog_conf -f test_consumer_static_file_single.conf -n 2 --threadN=10 -r --recordms=100"
}

test_simple()
{
    rm -f zlog.txt*
    eval "$asan_pre ./test_dzlog_conf -f test_consumer_static_file_single.conf -n 10"
}

asan_pre=""
valgrind_cmd=""
while getopts "t:a::v" opt; do
  case $opt in
    t)
      testname="$OPTARG"
      ;;
    v)
      valgrind_cmd="valgrind --track-fds=yes"
      ;;
    a)
      if [[ -z "${OPTARG}" ]]; then
          asan_pre="LD_PRELOAD=/usr/lib/gcc/x86_64-linux-gnu/11/libasan.so ASAN_OPTIONS=detect_leaks=1"
      else
          asan_pre="LD_PRELOAD=$OPTARG ASAN_OPTIONS=detect_leaks=1"
      fi
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
  esac
done

cd build/bin
$testname
cd - > /dev/null
