#!/usr/bin/python3

import os
import sys

qemubinary = None
qemuopts = [ "QEMU", "-monitor", "tcp::45455,server,nowait", "-serial", "stdio", "-display", "none" ]

try:
	builddir = sys.argv[1]
except:
	print("Need a build directory")
	sys.exit(1)

if len(sys.argv) > 2:
	extraopts = sys.argv[2:]
else:
	extraopts = []

def xml_get(query):
	p = os.popen("tools/xmlq -sf -o " + builddir + " " + query)

	result = p.readline().strip()

	return result

try:
	arch = xml_get("platform:arch")
	if arch == "x86":
		qemubinary = "qemu-system-i386"
	elif arch == "arm":
		qemubinary = "qemu-system-arm"
	elif arch == "arm64":
		qemubinary = "qemu-system-aarch64"
	else:
		raise Exception("Unknown arch, cannot run QEMU")
except Exception as e:
	print(e)
	sys.exit(1)

try:
	plat = xml_get("platform:board")
	if plat == "x86pc":
		qemuopts += [ "-M", "pc", "-cpu", "phenom", "-m", "512M" ]
	elif plat == "vexpress_a9":
		qemuopts += [ "-M", "vexpress-a9", "-m", "512M" ]
	else:
		raise Exception("Unknown platform, cannot run QEMU")
except Exception as e:
	print(e)
	sys.exit(1)

try:
	imgtype = xml_get(":image")
	if imgtype == "raw":
		qemuopts += [ "-kernel", builddir + "/image" ]
	elif imgtype == "multiboot":
		qemuopts += [ "-kernel", builddir + "/mbimage" ]
	elif imgtype == "uboot":
		qemuopts += [ "-kernel", builddir + "/uimage" ]
	else:
		raise Exception("Unknown image type, cannot run QEMU")
except Exception as e:
	print(e)
	sys.exit(1)

try:
	os.execvp(qemubinary, qemuopts + extraopts)
	# print(qemubinary)
	# print(qemuopts)
	# print(extraopts)
except:
	print("Error while exec()ing QEMU")
	sys.exit(1)
