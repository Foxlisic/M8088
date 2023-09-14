<?php

/*
 * Конвертирование из Bin -> MIF файл
 * Аргумент 1: Размер памяти (256k = 262144)
 * Аргумент 2: bin-файл
 * Аргумент 3: Куда выгрузить
 */

$size = (int) $argv[1];
$data = file_get_contents($argv[2]);
$len  = strlen($data);
if ($size < 1024) $size *= 1024;

if (empty($size)) { echo "size required\n"; exit(1); }

$out = [
    "WIDTH=8;",
    "DEPTH=$size;",
    "ADDRESS_RADIX=HEX;",
    "DATA_RADIX=HEX;",
    "CONTENT BEGIN",
];

$a = 0;

// RLE-кодирование
while ($a < $len) {

    // Поиск однотонных блоков
    for ($b = $a + 1; $b < $len && $data[$a] == $data[$b]; $b++);

    // Если найденный блок длиной до 0 до 2 одинаковых символов
    if ($b - $a < 3) {
        for ($i = $a; $i < $b; $i++) $out[] = sprintf("  %X: %02X;", $a++, ord($data[$i]));
    } else {
        $out[] = sprintf("  [%X..%X]: %02X;", $a, $b - 1, ord($data[$a]));
        $a = $b;
    }
}

if ($len < $size) $out[] = sprintf("  [%X..%X]: 00;", $len, $size-1);
$out[] = "END;";
$pb = join("\n", $out);

// Сохранить информацию
if (isset($argv[3])) file_put_contents($argv[3], $pb); else echo $pb;