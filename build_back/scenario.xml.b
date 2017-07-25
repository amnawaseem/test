<?xml version="1.0" encoding="UTF-8" ?>
<!DOCTYPE scenario SYSTEM "xml/dtd">
<scenario version="1.00" cbi="phidias" image="raw">
  <platform board="hikey" arch="arm64" />
  <!-- Boot this setup with:
	./hisi-idt.py -d /dev/ttyUSB1 \-\-img1 .../l-loader/l-loader.bin
	fastboot boot $BUILD/hikey/image
   -->
  <hypervisor ncpus="8" load_base="0x10008000" exec_base="0xf4000000" ccprefix="aarch64-linux-gnu-">
    <feature name="debugger" value="yes" />
    <feature name="tracer" value="yes" />
    <feature name="driver:uart" value="pl011" />
    <feature name="driver:timer" value="arm_generic" />
    <feature name="driver:clock" value="arm_generic" />
    <feature name="driver:irq" value="gic" />
    <memreq id="trace" size="0x40000" flags_demand="rwg" flags_prevent="xdus" cpumap="*" />
    <memreq id="cca0" size="0x100000" flags_prevent="" flags_demand=""/>
    <memreq id="cca1" size="0x100000" flags_prevent="" flags_demand=""/>
    <memreq id="xcore" size="0x40000" flags_demand="rwgs" flags_prevent="xdu" />
    <address_space type="mmu" format="arm64:g4io40">
      <map xref="serial" flags="w" />
      <map xref="irqc" flags="w" />
      <map xref="gpio4j" flags="w" />
      <map xref="timers" flags="w" />
      <map xref="thermal" flags="w" />
      <map xref="CATCHALL" flags="w" />
    </address_space>
  </hypervisor>
  <guest id="linux1" ncpus="1" cpumap="[0]">
    <memreq id="linux1_main" size="0x40000000" flags_demand="rw" flags_prevent="xdus" />
    <memreq id="linux1_arch" size="0x1000" flags_demand="rwg" flags_prevent="xdus" cpumap="*" />
    <memreq id="linux1_sram" base="0xfff80000" size="0x11000" flags_prevent="" flags_demand="" on="sram" />
    <vdev id="linux1_uart" type="serial" frontend="pl011" master="master">
      <emulate base="0xf7113000" size="0x1000" />
    </vdev>
    <vdev id="linux1_gic" type="irq_controller" frontend="arm_gic_virtext" master="master">
      <emulate base="0xf6801000" size="0x1000" />
    </vdev>
    <vdev id="linux1_sp804" type="timer" frontend="sp804">
      <emulate base="0xf8008000" size="0x1000" />
    </vdev>
    <vdev id="linux1_armcp14" type="timer" frontend="armcp14" />
    <vdev id="linux1_mmio_mediactrl" type="memory32">
      <emulate base="0xf4410000" size="0x1000" />
      <value type="default_mask_mem_r" value="0xffffffff" />
    </vdev>
    <vdev id="linux1_mmio1" type="memory32">
      <emulate base="0xf7010000" size="0x24000" />
      <value type="default_mask_mem_r" value="0xffffffff" />
    </vdev>
    <vdev id="linux1_mmio_dwmmc" type="memory32">
      <emulate base="0xf723d000" size="0x3000" />
      <value type="default_mask_mem_r" value="0xffffffff" />
    </vdev>
    <vdev id="linux1_mmio_xrange" type="memory32">
      <emulate base="0xf7500000" size="0x300000" />
      <value type="default_mask_mem_r" value="0xffffffff" />
    </vdev>
    <vdev id="linux1_mmio_aoctrl" type="memory32">
      <emulate base="0xf7800000" size="0x2000" />
      <value type="default_mask_mem_r" value="0xffffffff" />
    </vdev>
    <vdev id="linux1_mmio3" type="memory32">
      <emulate base="0xf8011000" size="0x5000" />
      <value type="default_mask_mem_r" value="0xffffffff" />
    </vdev>
    <vdev id="linux1_mmio_uart1" type="memory32">
      <emulate base="0xf7111000" size="0x1000" />
      <value type="default_mask_mem_r" value="0xffffffff" />
    </vdev>
    <vdev id="linux1_mmio_uart2" type="memory32">
      <emulate base="0xf7112000" size="0x1000" />
      <value type="default_mask_mem_r" value="0xffffffff" />
    </vdev>
    <vdev id="linux1_mmio_uart4" type="memory32">
      <emulate base="0xf7114000" size="0x1000" />
      <value type="default_mask_mem_r" value="0xffffffff" />
    </vdev>
    <vdev id="linux1_mmio4" type="memory32">
      <emulate base="0xf7100000" size="0x8000" />
      <value type="default_mask_mem_r" value="0xffffffff" />
    </vdev>
    <vdev id="linux1_mmio2" type="memory32">
      <emulate base="0xf8000000" size="0x8000" />
      <value type="default_mask_mem_r" value="0xffffffff" />
    </vdev>
    <address_space type="mmu" format="arm64:g4io40n">
      <map xref="linux1_main" base="0x00000000" flags="rwx" />
      <map xref="linux1_sram" base="0xfff80000" flags="rwx" />
      <map xref="cca1" base="0xfee00000" flags="rs" />
      <map xref="cca0" base="0xfef00000" flags="rws" />
      <map xref="irqc" base="0xf6802000" subsize="0x1000" offset="0x6000" flags="rw" />
    </address_space>
    <init arch_page="linux1_arch">
      <copy xref="linux_kernel" dref="linux1_main" offset="0x80000" /> <!-- 0x80000 -->
      <copy xref="linux_ramdisk" dref="linux1_main" offset="0xa000000" />
      <copy xref="linux_dtb" dref="linux1_main" offset="0x8000" />
    </init>
    <entry bp_xref="linux1_main" bp_offset="0x80000" />
    <sched class="wfq" />
  </guest>

  <files>
    <!-- file id="linux_kernel" href="/work/hikey/Image-3.18.0-linaro-hikey" / -->
    <file id="linux_kernel" href="/home/amna/Thesis/Linux_4.11-rc2/linux-4.11-rc2
/arch/arm64/boot/Image" />
    <file id="linux_ramdisk" href="/home/amna/Thesis/Linux_4.11-rc2/initramfs.igz" />
    <file id="linux_dtb" href="/home/amna/Thesis/Phidias_access/hi6220new.dtb" />

  </files>
</scenario>
