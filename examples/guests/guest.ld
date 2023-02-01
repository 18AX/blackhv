OUTPUT_FORMAT(binary)
ENTRY(_start)

SECTIONS
{
    . = 0x7C00;

	.boot :
	{
		*(.boot*)
	}

	.text :
	{
		*(.text*)
	}

	.rodata :
	{
		*(.rodata) *(.rodata.*)
	}

	.data :
	{
		*(.data) *(.data.*)
	}

	.bss :
	{
		*(.bss) *(.bss.*) *(COMMON)
	}
}
