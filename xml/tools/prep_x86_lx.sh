#!/bin/bash

INFILE="$1"
OUTFILE="$2"
RDP="$3"

echo "* Copying file..."
cp "$INFILE" "$OUTFILE"

echo "* Zeroing out parts of zero page not covered by Linux boot header..."
dd if=/dev/zero conv=notrunc,nocreat bs=1 count=$(( 0x1f1 )) status=none of="$OUTFILE"
dd if=/dev/zero conv=notrunc,nocreat bs=1 count=$(( 0xd70 )) seek=$(( 0x290 )) status=none of="$OUTFILE"

echo "* Setting loadflags..."
echo ff41 | xxd -r -p | \
	dd conv=notrunc,nocreat bs=1 count=2 seek=$(( 0x210 )) status=none of="$OUTFILE"

echo "* Setting commandline pointer..."
echo 00000a00 | xxd -r -p | \
	dd conv=notrunc,nocreat bs=1 count=4 seek=$(( 0x228 )) status=none of="$OUTFILE"

echo "* Setting e820 map..."
echo 02 | xxd -r -p | \
	dd conv=notrunc,nocreat bs=1 count=1 seek=$(( 0x1e8 )) status=none of="$OUTFILE"
echo 0000000000000000 00000a0000000000 01000000 | xxd -r -p | \
	dd conv=notrunc,nocreat bs=1 count=20 seek=$(( 0x2d0 )) status=none of="$OUTFILE"
echo 0000100000000000 0000f00f00000000 01000000 | xxd -r -p | \
	dd conv=notrunc,nocreat bs=1 count=20 seek=$(( 0x2e4 )) status=none of="$OUTFILE"

echo "* Installing ramdisk settings..."
RD_BASE=$(printf %08x "${RDP%:*}")
RD_SIZE=$(printf %08x "${RDP#*:}")

echo "** Base: $RD_BASE   Size: $RD_SIZE"
( echo $RD_BASE; echo $RD_SIZE ) | \
	sed -re 's/(..)(..)(..)(..)/\4\3\2\1/' | \
	xxd -r -p | \
	dd conv=notrunc,nocreat bs=1 count=8 seek=$(( 0x218 )) status=none of="$OUTFILE"

echo "* All done."

exit 0
