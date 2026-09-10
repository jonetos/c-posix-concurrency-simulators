#ifndef STRUCTS
#define STRUCTS

struct message {
    long type;
    int pid;
    char text[100];
};

struct fluid {
    int counter;
    int flow_rate;
};

#endif //STRUCTS