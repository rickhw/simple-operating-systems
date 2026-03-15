#!/bin/bash

rm -rf hdd.img
dd if=/dev/zero of=hdd.img bs=512 count=2048
