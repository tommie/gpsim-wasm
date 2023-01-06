#!/bin/sh

# Regression Test Script for invoking specific targets in Makefiles

USAGE="Usage: `basename $0` DIRECTORY TARGET"

if [ $# -lt 2 ] ; then
  echo "$USAGE"
  exit 1
fi

# this is the directory where the test will be performed
cd $1

LOGFILE="$2results.log"
touch $LOGFILE

# If gpsim is crashing during the regression tests, then uncomment
# the 'echo' to see which regression test was invoked:
# echo "make $1 $2"

# Run the simulation and save the results
${MAKE:-make} $2 > $LOGFILE


grep "FAILED" $LOGFILE
if [ $? -eq 0 ] ; then
  echo "!!! FAILED $1/make $2"
fi
grep "PASSED" $LOGFILE

