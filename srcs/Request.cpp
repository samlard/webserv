#include "Request.hpp"

void Request::clear() {
    method.clear();
    uri.clear();
    version.clear();
    headers.clear();
    body.clear();
}