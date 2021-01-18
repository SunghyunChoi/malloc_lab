#include <stdio.h>

int main()
{
    int** temp ;
    int x = 1;
    temp = &x;
    printf("%p %p", temp, temp+1);
}