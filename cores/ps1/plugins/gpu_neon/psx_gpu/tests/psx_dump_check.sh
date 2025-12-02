if test -z "$1"; then
  echo "$0 <logfile>"
  exit 1
fi

truncate --size 0 $1
for dump in gpu_dumps_all/*
do
  if [ -e $dump/dump3.dump -a -e $dump/list.dump ]; then
    ./psx_gpu $dump/dump3.dump $dump/list.dump -n >> $1
    echo "Mismatches in $dump: $?"
  fi
done
