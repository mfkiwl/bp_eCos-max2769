STARTUP(vectors.o)
ENTRY(__exception_handlers)
INPUT(extras.o)
GROUP( libtarget.a libgcc.a libsupc++.a )
MEMORY
{
    ram0 (rwx) : ORIGIN = (0x00100000), LENGTH = ((1024 * 2) - (8) - (2)) * 1024
    ram1 (rwx) : ORIGIN = ((0x00100000) + ((1024 * 2) - (8) - (2)) * 1024), LENGTH = (2) * 1024
}
SECTIONS
{
    .debug_aranges 0 : { *(.debug_aranges) } .debug_pubnames 0 : { *(.debug_pubnames) } .debug_info 0 : { *(.debug_info) } .debug_abbrev 0 : { *(.debug_abbrev) } .debug_line 0 : { *(.debug_line) } .debug_frame 0 : { *(.debug_frame) } .debug_str 0 : { *(.debug_str) } .debug_loc 0 : { *(.debug_loc) } .debug_macinfo 0 : { *(.debug_macinfo) } .note.arm.ident 0 : { KEEP (*(.note.arm.ident)) } /DISCARD/ 0 : { *(.fini_array*) }
    .rom_vectors (0x00100000) : { __rom_vectors_vma = ABSOLUTE(.); . = .; KEEP (*(.vectors)) } > ram0 __rom_vectors_lma = LOADADDR(.rom_vectors);
    .rel.text : { *(.rel.text) *(.rel.text.*) *(.rel.gnu.linkonce.t*) } > ram0 .rela.text : { *(.rela.text) *(.rela.text.*) *(.rela.gnu.linkonce.t*) } > ram0 .rel.data : { *(.rel.data) *(.rel.data.*) *(.rel.gnu.linkonce.d*) } > ram0 .rela.data : { *(.rela.data) *(.rela.data.*) *(.rela.gnu.linkonce.d*) } > ram0 .rel.rodata : { *(.rel.rodata) *(.rel.rodata.*) *(.rel.gnu.linkonce.r*) } > ram0 .rela.rodata : { *(.rela.rodata) *(.rela.rodata.*) *(.rela.gnu.linkonce.r*) } > ram0 .rel.got : { *(.rel.got) } > ram0 .rela.got : { *(.rela.got) } > ram0 .rel.ctors : { *(.rel.ctors) } > ram0 .rela.ctors : { *(.rela.ctors) } > ram0 .rel.dtors : { *(.rel.dtors) } > ram0 .rela.dtors : { *(.rela.dtors) } > ram0 .rel.init : { *(.rel.init) } > ram0 .rela.init : { *(.rela.init) } > ram0 .rel.fini : { *(.rel.fini) } > ram0 .rela.fini : { *(.rela.fini) } > ram0 .rel.bss : { *(.rel.bss) } > ram0 .rela.bss : { *(.rela.bss) } > ram0 .rel.plt : { *(.rel.plt) } > ram0 .rela.plt : { *(.rela.plt) } > ram0 .rel.dyn : { *(.rel.dyn) } > ram0
    .ARM.extab ALIGN (0x4) : { PROVIDE (__stext = ABSOLUTE(.)); _stext = ABSOLUTE(.) ; . = .; } > ram0 /DISCARD/ 0 : { *(.ARM.extab* .gnu.linkonce.armextab.*) } . = ALIGN(8); . = ALIGN(8); __exidx_start = ABSOLUTE(.); .ARM.exidx ALIGN(8) : AT ((LOADADDR (.ARM.extab) + SIZEOF (.ARM.extab) + (8) - 1) & ~ ((8) - 1)) { . = .; } > ram0 __exidx_end = ABSOLUTE(.); /DISCARD/ 0 : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } .text ALIGN(8) : AT ((LOADADDR (.ARM.exidx) + SIZEOF (.ARM.exidx) + (8) - 1) & ~ ((8) - 1)) { *(.text*) *(.gnu.warning) *(.gnu.linkonce.t.*) *(.init) *(.glue_7) *(.glue_7t) __CTOR_LIST__ = ABSOLUTE (.); KEEP (*(SORT (.ctors*))) __CTOR_END__ = ABSOLUTE (.); __DTOR_LIST__ = ABSOLUTE (.); KEEP (*(SORT (.dtors*))) __DTOR_END__ = ABSOLUTE (.); } > ram0 _etext = .; PROVIDE (__etext = .);
    .fini ALIGN (0x4) : { . = .; *(.fini) } > ram0
    .rodata ALIGN (0x4) : { . = .; *(.rodata*) *(.gnu.linkonce.r.*) } > ram0
    .rodata1 ALIGN (0x4) : { . = .; *(.rodata1) } > ram0
    .got ALIGN (0x4) : { . = .; *(.got.plt) *(.got) _GOT1_START_ = ABSOLUTE (.); *(.got1) _GOT1_END_ = ABSOLUTE (.); _GOT2_START_ = ABSOLUTE (.); *(.got2) _GOT2_END_ = ABSOLUTE (.); } > ram0
    .fixup ALIGN (0x4) : { . = .; *(.fixup) } > ram0
    .gcc_except_table ALIGN (0x4) : { . = .; } > ram0 /DISCARD/ 0 : { *(.gcc_except_table*) }
    .data ALIGN (0x4) : AT ((LOADADDR (.gcc_except_table) + SIZEOF (.gcc_except_table) + (4) - 1) & ~ ((4) - 1)) { __ram_data_start = ABSOLUTE (.); *(.data*) *(.data1) *(.gnu.linkonce.d.*) . = ALIGN (4); KEEP(*( SORT (.ecos.table.*))) ; . = ALIGN (4); __init_array_start__ = ABSOLUTE (.); KEEP (*(SORT (.init_array.*))) KEEP (*(SORT (.init_array))) __init_array_end__ = ABSOLUTE (.); *(.dynamic) *(.sdata*) *(.gnu.linkonce.s.*) . = ALIGN (4); *(.2ram.*) } > ram0 __rom_data_start = LOADADDR (.data); __ram_data_end = .; PROVIDE (__ram_data_end = .); _edata = .; PROVIDE (edata = .); PROVIDE (__rom_data_end = LOADADDR (.data) + SIZEOF(.data));
    .bss ALIGN (0x4) : { __bss_start = ABSOLUTE (.); *(.scommon) *(.dynsbss) *(.sbss*) *(.gnu.linkonce.sb.*) *(.dynbss) *(.bss*) *(.gnu.linkonce.b.*) *(COMMON) __bss_end = ABSOLUTE (.); } > ram0
    __heap1 = ALIGN (0x40);
    .fixed_vectors ((0x00100000) + ((1024 * 2) - (8) - (2)) * 1024) : { . = .; KEEP (*(.fixed_vectors)) } > ram1
    . = ALIGN(4); _end = .; PROVIDE (end = .);
}
