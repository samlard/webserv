#include <stdio.h>

int main(void) {
    printf("Content-Type: text/plain\r\n");
    printf("Status: 200 OK\r\n");
    printf("\r\n");
    printf("Hello from C CGI!\n");
    return 0;
}
