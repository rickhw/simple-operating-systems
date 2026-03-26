#include "stdio.h"

int main(int argc, char** argv) {
    print("\n[ECHO Program] Start printing arguments...\n");

    if (argc <= 1) {
        print("  (No arguments provided)\n");
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        print("  ");
        print(argv[i]);
        print("\n");
    }

    print("[ECHO Program] Done.\n");
    return 0;
}
