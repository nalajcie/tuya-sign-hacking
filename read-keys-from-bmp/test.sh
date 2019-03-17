#!/bin/sh

# input
TEST_BMP="test.bmp"
TEST_APPID="3fjrekuxank9eaej3gcx"

# output
TEST_KEY_HEX="76617939673539673967393971663372747170746D6333656D686B616E776B78"
TEST_KEY_STR="vay9g59g9g99qf3rtqptmc3emhkanwkx"

# run once to show logs
LOGS="$(./read_keys "$TEST_APPID" "$TEST_BMP")"
echo "$LOGS"

# get the keys
KEY_HEX=$(echo "$LOGS" | grep KEY | grep hex | cut -d ':' -f 2 | tr -d ' ')
KEY_STR=$(echo "$LOGS" | grep KEY | grep str | cut -d ':' -f 2 | tr -d ' ')

#check the keys
if [ ! -z "$KEY_HEX" ] && [ "$KEY_HEX" = "$TEST_KEY_HEX" ]; then
    echo "HEX KEY matches"
else
    echo "HEX KEY fail"
    echo "$KEY_HEX != $TEST_KEY_HEX"
    exit 1
fi
if [ ! -z "$KEY_STR" ] && [ "$KEY_STR" = "$TEST_KEY_STR" ]; then
    echo "STR KEY matches"
else
    echo "STR KEY fail"
    echo "$KEY_STR != $TEST_KEY_STR"
    exit 1
fi
