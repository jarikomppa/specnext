		.module crt0
		.globl _heap

		.area _HEADER(ABS)
	
		di
		call _main			

		.area _HOME
		.area _CODE
        .area _GSINIT
        .area _GSFINAL	
		.area _DATA
        .area _BSS
        .area _HEAP

       	.area _GSINIT
gsinit:	
       	.area _GSFINAL
       	ret

		.area _DATA
		.area _BSS
		.area _HEAP
_heap::
