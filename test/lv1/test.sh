#!/bin/bash

files=$(ls)
for filename in $files
do
  if [ "${filename##*.}" = "l24" ]; then
    ../../build/bin/l24 ${filename} >> /dev/null
    if [ $(echo $?) != 0 ]; then
      echo "runtime error"
      rm output.S
      rm output
      exit 1
    fi

    gcc -L../../lib -lsysy "output.S" -o "output"

    tmp_file=$(mktemp /tmp/${filename%%.*}.output)

    if [ -e ${filename%%.*}.in ]; then
      ./output < ${filename%%.*}.in > "$tmp_file"
    else
      ./output
    fi

    echo $? >> "$tmp_file"

    if [ $(diff "$tmp_file" ${filename%%.*}.out) ]; then
      echo "result of ${filename} is wrong"
      rm output.S
      rm output
      exit 1
    else
      echo "test ${filename} success"
    fi
    rm "$tmp_file"
  fi
done
rm output.S
rm output
