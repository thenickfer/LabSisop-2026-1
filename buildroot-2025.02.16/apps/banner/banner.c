#include <stdio.h>

int main()
{
    const char *banner = 
        "\x1b[36m====================================\n"
        "|\x1b[0m           \x1b[35mOS DO NICOLAS\x1b[0m          \x1b[36m|\n"
        "====================================\n"
        "|\x1b[0m	      \x1b[35mBem-Vindo\x1b[0m            \x1b[36m|\n"
        "====================================\n\x1b[0m";

    printf("%s", banner);
    return 0;
}
