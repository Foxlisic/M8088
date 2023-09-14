#include "obj_dir/Vc8088.h"
#include "obj_dir/Vga.h"
#include "obj_dir/Vps2.h"
#include "font.h"
#include "tb.h"

int main(int argc, char** argv) {

    Verilated::commandArgs(argc, argv);

    TB* tb = new TB(argc, argv);
    while (tb->main()) tb->frame();
    return tb->destroy();
}
