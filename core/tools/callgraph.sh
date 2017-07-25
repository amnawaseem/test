#!/bin/bash

function canon() {
	echo "$1" | sed -e 's/\.isra\.*//'
}

BUILDDIR="$1"

echo "strict digraph cfg {"

FNLIST=$(objdump -d "$BUILDDIR/phidias.elf" | grep '<.*>:$' | sed -e 's/.*<//' -e 's/>://')

for f in $FNLIST; do
	echo "  $(canon $f);"
done

for f in $FNLIST; do
	# echo "TRACE FOR $f"
	TARGETS=$(objdump -d "$BUILDDIR/phidias.elf" | sed -n -e '/<'"${f}"'>:$/,/^$/ p' | grep "[^:]	b[	.l].*<" | sed -e 's/.*<//' -e 's/>.*//')
	TARGETS=$(echo $TARGETS)
	REALTARGETS=" "
	f=$(canon $f)
	for t in $TARGETS; do
		t=$(canon $t)
		if [ "${t%%+*}" == "$f" ]; then
			continue
		fi
		if [ "${REALTARGETS/ $t //}" == "$REALTARGETS" ]; then
			REALTARGETS="$REALTARGETS$t "
		fi
	done
	# echo "// $TARGETS ### $REALTARGETS"
	for rt in $REALTARGETS; do
		echo "  $f -> $rt;"
	done
done

cat - <<EOH
  __boot_nonsecure -> __boot_paged;
  __boot_paged -> main;
  interrupt_handler -> uart_irq;
  interrupt_handler -> timer_interrupt;
  interrupt_handler -> emulate_timer_cp14_interrupt;
EOH

echo "}"
