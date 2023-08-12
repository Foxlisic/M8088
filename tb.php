<?php

$tb = file_get_contents("tb.bin");
for ($a = 0; $a < strlen($tb); $a++) {
    $r[] = sprintf("%02X", ord($tb[$a]));
}
file_put_contents("tb.hex", join("\n", $r));
