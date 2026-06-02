#include "dependencies.h"
#include "scanner.h"

// Returns a null-terminated string that has the file's contents.
// Needs to be freed.
int main(void)
{
    char* str = read_file("test2.txt");

    Scanner scanner = init_scanner(str);
    scanner_scan_tokens(&scanner);

    printf("%s\n", str);

    for (int i = 0; i < arrlen(scanner.token_list); ++i)
    {
        Token t = scanner.token_list[i];

        printf(
            "[%d:%d:%d]: %-20s '%.*s'\n",
            t.line,
            t.column,
            t.length,
            token_type_name(t.type),
            t.length,
            t.start
        );
    }

    return 0;
}
