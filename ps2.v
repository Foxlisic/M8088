/* verilator lint_off WIDTH */

module ps2
(
    input   clock,           // Тактовая частота 25 Мгц
    input   ps_clock,        // Пин, подключенный к проводу CLOCK с PS/2
    input   ps_data,         // Пин DATA

    output reg       done,   // Устанавливается =1, если данные доступны
    output reg [7:0] data    // Принятый байт с PS/2
);

initial begin data = 8'h00; done = 1'b0; end

reg         kbusy   = 1'b0;   // =1 Если идет прием данных с пина (шины) DATA
reg         kdone   = 1'b0;   // =1 Прием сигнала завершен, "фантомный" регистр к `done`
reg [1:0]   klatch  = 2'b00;  // Сдвиговый регистр для отслеживания позитивного и негативного фронта CLOCK
reg [3:0]   kcount  = 1'b0;   // Номер такта CLOCK
reg [9:0]   kin     = 1'b0;   // Сдвиговый регистр для приема данных с DATA
reg [19:0]  kout    = 1'b0;   // Отсчет таймаута для "зависшего" приема данных в случае ошибки

always @(negedge clock) done <= kdone;

always @(posedge clock) begin

    kdone <= 1'b0;

    // Процесс приема сигнала
    if (kbusy) begin

        // Позитивный фронт
        if (klatch == 2'b01) begin

            // Завершающий такт
            if (kcount == 4'hA) begin

                data    <= kin[8:1];
                kbusy   <= 1'b0;
                kdone   <= ^kin[9:1]; // =1 Если четность совпадает

            end

            kcount  <= kcount + 1'b1;
            kin     <= {ps_data, kin[9:1]};

        end

        // Считать "зависший процесс"
        kout <= ps_clock ? kout + 1 : 1'b0;

        // И если прошло более 20 мс, то перевести в состояние ожидания
        if (kout > 25000*20) kbusy <= 1'b0;

    end else begin

        // Обнаружен негативный фронт \__
        if (klatch == 2'b10) begin

            kbusy   <= 1'b1; // Активировать прием данных
            kcount  <= 1'b0; // Сброс двух счетчиков в 0
            kout    <= 1'b0;

        end

    end

    klatch <= {klatch[0], ps_clock};

end

endmodule
