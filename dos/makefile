VINC=/usr/share/verilator/include

all:
	fasm dos.asm tb.bin
	php ../av/mifer.php 16k tb.bin ../de0/mbios.mif
	../av/tb tb.bin
