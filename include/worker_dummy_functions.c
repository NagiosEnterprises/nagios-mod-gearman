char * nebtype2str(__attribute__((__unused__)) int i) {
    return strdup("UNKNOWN");
}

char * nebcallback2str(__attribute__((__unused__)) int i) {
    return strdup("UNKNOWN");
}

char * eventtype2str(__attribute__((__unused__)) int i) {
    return strdup("UNKNOWN");
}

void logit(__attribute__((__unused__)) int, __attribute__((__unused__)) int, __attribute__((__unused__))const char *, ...);
void logit(__attribute__((__unused__)) int type, __attribute__((__unused__)) int display, __attribute__((__unused__))const char * data, ...) {
    return;
}
