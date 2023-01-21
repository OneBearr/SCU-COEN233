#include <stdio.h>
#include <stdlib.h>

int main() {
    char age[10];
    printf("Enter your age: ");
    fgets(age, 10, stdin);
    printf("You are %s years old\n", age);

    return 0;
}