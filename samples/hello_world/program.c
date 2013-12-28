
#include "shttp.h"
#include <string.h>

#define HELLO_WORLD "<html><body>HELLO WORLD!!!</body></html>"

int test_request_handler(shttp_request_t *req) {
    http_send_response_string(req, 200, "", "text/html", HELLO_WORLD, strlen(HELLO_WORLD), NULL, NULL);
    return 0;
}


int main(int argc, char** argv) {
    shttp_t *server = shttp_create(nil);
    shttp_listen("0.0.0.0", 8000);
    shttp_add_handler(server, "/", test_request_handler);
    shttp_run(server);
    return 0;
}
