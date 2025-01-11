#!/bin/bash
gdb -ex "file build/t0.elf" -ex "target remote localhost:1234"
