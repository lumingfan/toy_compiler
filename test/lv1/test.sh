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
      ./output > "$tmp_file"
    fi

    echo $? >> "$tmp_file"

    diff "$tmp_file" ${filename%%.*}.out
    if [ $(echo $?) != 0 ]; then
      echo "result of ${filename} is wrong"
      rm output.S
      rm output
      rm "$tmp_file"
      exit 1
    else
      echo "test ${filename} success"
    fi
    rm "$tmp_file"
  fi
done
rm output.S
rm output
