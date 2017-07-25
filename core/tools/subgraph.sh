#!/bin/bash

DOTFILE="$1"
shift

STARTSYMS=$(echo $@)

NEWSSYMS="$STARTSYMS"
while true; do
	OLDSSYMS="$NEWSSYMS"

	SREGEX="(${NEWSSYMS// /|})"

	( for S in $NEWSSYMS; do echo $S; done; \
	grep -E "^[[:space:]]*$SREGEX[[:space:]]*->" "$DOTFILE" | sed -r -e 's/.*->[[:space:]]*//' -e 's/[[:space:]]*;$//' ) | sort -u > /tmp/syms

	NEWSSYMS=$(< /tmp/syms)
	NEWSSYMS=$(echo $NEWSSYMS)

	if [ "$OLDSSYMS" = "$NEWSSYMS" ]; then break; fi
done

SREGEX="(${NEWSSYMS// /|})"

grep -E "^[[:space:]]*$SREGEX|^strict|^}" "$DOTFILE" > "${DOTFILE%.dot}_${STARTSYMS// /_}.dot"

#echo $NEWSSYMS

exit 0
