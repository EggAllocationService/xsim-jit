#!/bin/sh

if ! [ -f tests/test.xo ]; then
  ./xas tests/test.xas tests/test.xo > /dev/null
fi 

echo ======================================================
echo ====================== TEST $1 ========================
echo ======================================================
if timeout 10 tests/test.$1.sh ./$2/$3 > tests/test.$1.out; then 
  if diff -w tests/test.$1.out tests/test.$1.expected > /dev/null; then
    echo PASSED
  else
    echo FAILED
    echo ======
    echo ____Your_output____ __Expected_Output___
   #echo +++++++++++++++++++ ++++++++++++++++++++
    diff -y -W 80 tests/test.$1.out tests/test.$1.expected 
    echo =====================
    echo Your Output:
    echo ++++++++++++++++
    cat tests/test.$1.out
    echo =====================
    echo Expected Output:
    echo ++++++++++++++++
    cat tests/test.$1.expected
    exit 1
  fi
elif [ $? -eq 124 ]; then
  echo TIMEOUT
  exit 1
else 
  echo Abnormal program termination: the program crashed
  echo Exit code $?
  exit 1
fi
