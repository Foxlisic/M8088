all:
	fasm tb.asm tb.bin && php tb.php && rm tb.bin
	iverilog -g2005-sv -o tb.qqq tb.v c8086.v
	vvp tb.qqq -o tb.vcd > vvp.log
	rm tb.qqq
wav:
	gtkwave tb.gtkw
