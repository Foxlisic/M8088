module de0
(
    // Reset
    input              RESET_N,

    // Clocks
    input              CLOCK_50,
    input              CLOCK2_50,
    input              CLOCK3_50,
    inout              CLOCK4_50,

    // DRAM
    output             DRAM_CKE,
    output             DRAM_CLK,
    output      [1:0]  DRAM_BA,
    output      [12:0] DRAM_ADDR,
    inout       [15:0] DRAM_DQ,
    output             DRAM_CAS_N,
    output             DRAM_RAS_N,
    output             DRAM_WE_N,
    output             DRAM_CS_N,
    output             DRAM_LDQM,
    output             DRAM_UDQM,

    // GPIO
    inout       [35:0] GPIO_0,
    inout       [35:0] GPIO_1,

    // 7-Segment LED
    output      [6:0]  HEX0,
    output      [6:0]  HEX1,
    output      [6:0]  HEX2,
    output      [6:0]  HEX3,
    output      [6:0]  HEX4,
    output      [6:0]  HEX5,

    // Keys
    input       [3:0]  KEY,

    // LED
    output      [9:0]  LEDR,

    // PS/2
    inout              PS2_CLK,
    inout              PS2_DAT,
    inout              PS2_CLK2,
    inout              PS2_DAT2,

    // SD-Card
    output             SD_CLK,
    inout              SD_CMD,
    inout       [3:0]  SD_DATA,

    // Switch
    input       [9:0]  SW,

    // VGA
    output      [3:0]  VGA_R,
    output      [3:0]  VGA_G,
    output      [3:0]  VGA_B,
    output             VGA_HS,
    output             VGA_VS
);

// Z-state
assign DRAM_DQ = 16'hzzzz;
assign GPIO_0  = 36'hzzzzzzzz;
assign GPIO_1  = 36'hzzzzzzzz;

// LED OFF
assign HEX0 = 7'b1111111;
assign HEX1 = 7'b1111111;
assign HEX2 = 7'b1111111;
assign HEX3 = 7'b1111111;
assign HEX4 = 7'b1111111;
assign HEX5 = 7'b1111111;

// Генератор частот
// -----------------------------------------------------------------------------

wire locked;
wire clock_25;

pll PLL0
(
    .clkin     (CLOCK_50),
    .m25       (clock_25),
    .m50       (clock_50),
    .m100      (clock_100),
    .locked    (locked)
);

// Внутрисхемная память
// -----------------------------------------------------------------------------

wire [12:0] m1_a_video;
wire [7:0]  m1_i_video, m1_i, m2_i, m3_i;

// Области памяти
wire        is_m_common = address < 20'h20000; // 128K
wire        is_m_bios   = address > 20'hF0000; // 64K
wire        is_m_video  = address >= 20'hB8000 && address < 20'hBA000;

// Память видеоадаптера
mga M1_GA
(
    .clock  (clock_100),
    .a0     (address[12:0]),
    .d0     (out),
    .w0     (we && is_m_video),
    .q0     (m1_i),
    .a1     (m1_a_video),
    .q1     (m1_i_video)
);

// Обшая память 128К
mcom M2_COMM
(
    .clock  (clock_100),
    .a0     (address[16:0]),
    .d0     (out),
    .w0     (we && is_m_common),
    .q0     (m2_i)
);

// БИОС 64К :: Пока что так
mbios M2_BIOS
(
    .clock  (clock_100),
    .a0     (address[15:0]),
    .q0     (m3_i)
);

// Роутер памяти
wire [7:0]  in =

    is_m_bios   ? m3_i :    // BIOS 64К (MIRROR 32k)
    is_m_common ? m2_i :    // COMMON
    is_m_video  ? m1_i :    // VIDEO 4K
    8'h00;                  // OTHER

// Видеопроцессор
// -----------------------------------------------------------------------------

wire [10:0] cursor;

ga VIDEO
(
    .clock      (clock_25),
    .r          (VGA_R),
    .g          (VGA_G),
    .b          (VGA_B),
    .hs         (VGA_HS),
    .vs         (VGA_VS),
    .address    (m1_a_video),
    .data       (m1_i_video),
    .cursor     (cursor)
);

// Интерфейс процессора
// -----------------------------------------------------------------------------

wire [19:0] address;
wire [ 7:0] out, port_i, port_o, irq_in;
wire        we, port_w, port_r, irq;
wire [15:0] port_a;

c8088 CPU86
(
    // Основной контур для процессора
    .clock          (clock_25),
    .ce             (1'b1),
    .reset_n        (locked),
    .address        (address),
    .in             (in),
    .out            (out),
    .we             (we),

    // Порты ввода-вывода
    .port_a         (port_a),
    .port_w         (port_w),
    .port_r         (port_r),
    .port_i         (port_i),
    .port_o         (port_o),

    // PIC: Программируемый контроллер прерываний
    .irq            (irq),
    .irq_in         (irq_in)
);

endmodule

`include "../c8088.v"
`include "../ga.v"
