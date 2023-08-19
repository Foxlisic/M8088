// synopsys translate_off
`timescale 1 ps / 1 ps
// synopsys translate_on
module mcom (clock, a0, a1, d0, d1, w0, w1, q0, q1);

input           clock;
input    [16:0] a0;
input    [16:0] a1;
input    [ 7:0] d0;
input    [ 7:0] d1;
output   [ 7:0] q0;
output   [ 7:0] q1;
input           w0;
input           w1;

`ifndef ALTERA_RESERVED_QIS
// synopsys translate_off
`endif
    tri1      clock;
    tri0      w0;
    tri0      w1;
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_on
`endif
altsyncram altsyncram_component
(
    .address_a        (a0),
    .address_b        (a1),
    .clock0           (clock),
    .data_a           (d0),
    .data_b           (d1),
    .wren_a           (w0),
    .wren_b           (w1),
    .q_a              (q0),
    .q_b              (q1),
    .aclr0            (1'b0),
    .aclr1            (1'b0),
    .addressstall_a   (1'b0),
    .addressstall_b   (1'b0),
    .byteena_a        (1'b1),
    .byteena_b        (1'b1),
    .clock1           (1'b1),
    .clocken0         (1'b1),
    .clocken1         (1'b1),
    .clocken2         (1'b1),
    .clocken3         (1'b1),
    .eccstatus        (),
    .rden_a           (1'b1),
    .rden_b           (1'b1)
);
defparam
    altsyncram_component.address_reg_b            = "CLOCK0",
    altsyncram_component.clock_enable_input_a     = "BYPASS",
    altsyncram_component.clock_enable_input_b     = "BYPASS",
    altsyncram_component.clock_enable_output_a    = "BYPASS",
    altsyncram_component.clock_enable_output_b    = "BYPASS",
    altsyncram_component.indata_reg_b             = "CLOCK0",
    altsyncram_component.init_file                = "mcom.mif",
    altsyncram_component.intended_device_family   = "Cyclone V",
    altsyncram_component.lpm_type                 = "altsyncram",
    altsyncram_component.numwords_a               = 131072,
    altsyncram_component.numwords_b               = 131072,
    altsyncram_component.operation_mode           = "BIDIR_DUAL_PORT",
    altsyncram_component.outdata_aclr_a           = "NONE",
    altsyncram_component.outdata_aclr_b           = "NONE",
    altsyncram_component.outdata_reg_a            = "UNREGISTERED",
    altsyncram_component.outdata_reg_b            = "UNREGISTERED",
    altsyncram_component.power_up_uninitialized   = "FALSE",
    altsyncram_component.ram_block_type           = "M10K",
    altsyncram_component.read_during_write_mode_mixed_ports = "DONT_CARE",
    altsyncram_component.read_during_write_mode_port_a = "NEW_DATA_NO_NBE_READ",
    altsyncram_component.read_during_write_mode_port_b = "NEW_DATA_NO_NBE_READ",
    altsyncram_component.widthad_a                = 17,
    altsyncram_component.widthad_b                = 17,
    altsyncram_component.width_a                  = 8,
    altsyncram_component.width_b                  = 8,
    altsyncram_component.width_byteena_a          = 1,
    altsyncram_component.width_byteena_b          = 1,
    altsyncram_component.wrcontrol_wraddress_reg_b = "CLOCK0";

endmodule