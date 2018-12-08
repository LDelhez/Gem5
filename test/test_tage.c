
#include <stdio.h>

#define N 40

float A[N*N];

int main() {

        puts("START");
        for (int i = 0; i < N; i++ ) {
                for (int j = 0; j < N; j++ ) {
                        if (i == j)
                                A[i+j*N]++;
                }
        }
}
