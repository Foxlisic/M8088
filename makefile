VINC=/usr/share/verilator/include

all: ica tbc
	./tb tb.bin
ica:
	fasm tb.asm tb.bin && php tb.php
	iverilog -g2005-sv -o tb.qqq tb.v c8088.v
	vvp tb.qqq -o tb.vcd > vvp.log
	rm tb.qqq
tbc:
	verilator -cc c8088.v
	verilator -cc ga.v
	cd obj_dir && make -f Vc8088.mk
	cd obj_dir && make -f Vga.mk
	g++ -o tb -I$(VINC) tb.cc $(VINC)/verilated.cpp obj_dir/Vc8088__ALL.a obj_dir/Vga__ALL.a -lSDL2
wav:
	gtkwave tb.gtkw
