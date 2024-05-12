#!/bin/bash

filenames=$(ls)

for filename in $filenames
do
  if [ "${filename##*.}" == "c" ]; then
    mv ${filename} ${filename%%.*}.l24
  fi
done
