SCHISM -- Static Configurator for Hypervisors Including Scenario Metadata
=========================================================================

SCHISM is (c) by Jan Nordholz.

SCHISM is licensed under the GNU General Public License, version 2 (GPLv2).
A copy of this license is contained in this repository; see LICENSE.txt.

=========================================================================

SCHISM is a hypervisor-implementation-agnostic build system for statically
configurable hypervisors. It provides a mechanism for specifying a desired
scenario for a given platform and architecture as an XML document. The
build system then allocates resources, applies memory placement strategies
etc. to complete the specification, and integrates the final data set into
the hypervisor image. During processing, the XML document is iteratively
transformed and validated to conform to the respective stage's XML schema.

For a lengthy explanation of the concept (and the benefits of using fully
static hypervisor setups in general), you are invited to look into my
dissertation at <doi:10.14279/depositonce-5938>.

The following list provides a high-level overview. Stages with an asterisk
are the few ones that depend on the hypervisor implementation, the CBI
(Configuration Binary Interface); these call out to the appropriate
CBI-specific tool (e.g. generate_config_phidias for the Phidias CBI).

Stage	Deps	Binary		Description
-------------------------------------------------------------------------------------

(-1*)	[]	precompilation	generate build configuration from XML data

(0)	[-1]	(COMPILER)	compile hypervisor binary

(1)	[]	expand		expand and complete XML:
(1a)	[]			* import <arch> and <board> into platform node
(1b)	[]			* add implicit attributes: <guest cpumap>...
(1c)	[]			* add missing <memreq> nodes from default <hypervisor>

(2)	[1]	reparent	reparent nodes and complete memory model:
(2a)	[]			* determine default (i.e. largest) <memory> node
(2b)	[]			* add <map> nodes for <memreq> nodes with forced mapping
(2c)	[]			* reparent all <memreq> nodes to <memory>
(2d)	[]			* validate <map> permission flags

(3*)	[2]	generate_config	generate stub configuration data (empty, but complete)
		(COMPILER)

(4)	[2]	measure		import measurable components into XML (new <memreq> and <map> nodes):
(4a)	[0]			* measure core (rx, r, rw, rws)
(4b)	[3]			* measure stub configuration (r, rw, rws)
(4c)	[]			* apply specified size / estimate size of pagetables
(4d)	[]			* measure payload files

(5)	[4]	layout_memory	layout physical and virtual memory space

(6)	[5]	pagetables	generate pagetables:
(6a)	[]			* generate PTINIT (choose memreq/map by XML attribute)
(6b)	[]			* generate PTCOREs
(6c)	[]			* compare total size against estimate

(7*)	[6]	generate_config	generate real configuration data
		(COMPILER)

(8)	[7]	combine		concatenate final image:
				* build ( CORE ++ CONFIG ++ PAGETABLES ++ BLOB )
				* wrap into desired format (mkimage / uefi pe / ...)

=========================================================================

The source code consists of the following modules:

 * include/schism/, lib/

   A convenient wrapper library around the low-level XML accessor library
   (libxml2), which also provides useful shortcuts for frequently used
   SCHISM-specific XML operations, such as manangement of address spaces of
   guests and the hypervisor itself.

 * schema/

   XML schema sequence. As the XML schemata for the transformation stages
   only differ marginally, the chosen storage strategy is to have the initial
   XSD (00.xsd) and a series of patch files (??.diff) which can be applied
   iteratively to yield the subsequent XSDs.

   See below for the mapping between XML filename and XSD.

 * src/

   CBI-agnostic transformation stages.

 * src/cbi/phidias/

   CBI-specific transformation stages for the Phidias hypervisor.

=========================================================================

The source code only depends on a standard libc and libxml2. In addition,
compiling the CBI tools for your hypervisor of choice usually requires access
to headers exported by the hypervisor - look for build instructions in the
hypervisor source code to learn how to fulfil those requirements.

The build process is expected to run out-of-tree, i.e. the target directory
is specified using the "O=" make variable. There is no "build directory
initialization" process yet; just create an empty one and copy a prepared
scenario.xml into it. The build process will take it from there.

The dependency chain is linked like this (the authoritative source is of
course Makefile.output):

Stage	Inputs				Outputs
-------------------------------------------------------------------------------
(-1)	scenario.xml			Makeconf
(0)	Makeconf			include/config.h, hypervisor ELF
(1)	scenario.xml			scenario_expanded.xml
(2)	scenario_expanded.xml		scenario_reparented.xml
(3)	scenario_reparented.xml		scenario_config.c, scenario_config.xo
(4)	scenario_reparented.xml,	scenario_measured.xml
	hypervisor ELF,
	scenario_config.xo
(5)	scenario_measured.xml		scenario_p_laidout.xml,
					scenario_v_laidout.xml
(6)	scenario_v_laidout.xml		scenario_pagetables.xml, pagetables
(7)	scenario_pagetables.xml		scenario_config_real.c,
					scenario_config_real.xo
(8)	scenario_pagetables.xml,	final image
	hypervisor ELF,
	scenario_config_real.xo,
	pagetables, blob files

=========================================================================

The schemata are used for validation of the original and the transformed
XML files. They are integrated into the SCHISM support library, so it is
unnecessary to install the schema files to a particular location.

XML File Name				Schema
-------------------------------------------------------------------------------
scenario.xml				00
scenario_expanded.xml			01
scenario_reparented.xml			02
scenario_measured.xml			03
scenario_p_laidout.xml			04
scenario_v_laidout.xml			05
scenario_pagetables.xml			06
