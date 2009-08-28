#!/bin/bash
#
# Ghetto script to make sure libsoundtouch4c >= 0.4.0 is installed.
#

if [[ -f /usr/local/include/soundtouch4c.h ]]
then
    SOUNDTOUCH4C=/usr/local/include/soundtouch4c.h
fi
if [[ -f /usr/include/soundtouch4c.h ]]
then
    SOUNDTOUCH4C=/usr/include/soundtouch4c.h
fi

if [[ -z $SOUNDTOUCH4C ]]
then
    echo '----------------------------------------------------------------------'
    echo 'Please install libsoundtouch4c from www.lobstertech.com'
    echo '----------------------------------------------------------------------'
    exit 1
fi

if [[ $(grep '^ \* Version:' $SOUNDTOUCH4C) == "" ]]
then
    echo '----------------------------------------------------------------------'
    echo 'Please install libsoundtouch4c version 0.4.0 or greater'
    echo '----------------------------------------------------------------------'
    exit 1
fi

exit 0
