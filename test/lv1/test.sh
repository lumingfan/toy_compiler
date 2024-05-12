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

    gcc "output.S" -o "output"
    ./output
    if [ $(echo $?) != $(cat ${filename%%.*}.out) ]; then
      echo "result of ${filename} is wrong"
      rm output.S
      rm output
      exit 1
    else
      echo "test ${filename} success"
    fi
  fi
done
rm output.S
rm output
