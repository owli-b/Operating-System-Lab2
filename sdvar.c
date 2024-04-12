#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

// #include <stdlib.h> //exit
// #include <stdio.h>
// #include <math.h> //pow, sqrt

#define SIZE 7

double pow(double base, double exponent)
{
    double result = 1.0;
    for (int i = 0; i < exponent; i++)
    {
        result *= base;
    }
    return result;
}

double sqrt(double x)
{
    if (x < 0)
        return -1;
    double guess = 1.0;
    double epsilon = 0.00001;
    while (x - (guess * guess) > epsilon)
    {
        guess = guess + epsilon;
    }
    return guess;
}

int calculate_lenth(char str[])
{
    int count = 0;
    for (int i = 0; str[i] != '\0'; i++)
    {
        count++;
    }
    return count;
}

void writeint(int input, int fd)
{
    int j = 0;
    char temp[20];
    if (input == 0)
    {
        char temp2 = 48;
        write(fd, &temp2, 1);
        return;
    }
    if (input < 0)
    {
        char temp3 = '-';
        write(fd, &temp3, 1);
        input = -1 * input;
    }
    while (input > 0)
    {
        temp[j] = input % 10;
        input = input / 10;
        j++;
    }
    for (int i = j - 1; i >= 0; i--)
    {
        char temp2 = temp[i] + 48;
        write(fd, &temp2, 1);
    }
    return;
}

int main(int argc, char *argv[])
{
    int int_list[SIZE];
    float sum = 0.0, average;
    int lower[SIZE];
    int higher[SIZE];
    int lower_count = 0, higher_count = 0;
    float variance_higher = 0.0, standard_deviation_lower = 0.0;

    // check len input list lower than 7
    if (argc > SIZE + 1)
    {
        printf(2, "The number of inputs must lower that 7\n");
        exit();
    }

    else
    {
        // check all nubers and conver them to int
        for (int i = 1; i < argc; i++)
        {
            int result = 0;
            int len = calculate_lenth(argv[i]);
            for (int j = 0; j < len; j++)
            {
                if (argv[i][j] >= '0' && argv[i][j] <= '9')
                {
                    result = result * 10 + (argv[i][j] - '0');
                }
                else
                {
                    printf(2, "Only numbers allowed here!\n");
                    exit();
                }
            }
            int_list[i - 1] = result;
        }

        // calculate average
        for (int t = 0; t < SIZE; t++)
        {
            sum += int_list[t];
        }

        average = sum / SIZE;

        // seprate the list with number
        for (int x = 0; x < SIZE; x++)
        {
            if (int_list[x] <= average)
            {
                lower[lower_count++] = int_list[x];
            }
            else
            {
                higher[higher_count++] = int_list[x];
            }
        }

        // calculate variance for higher list
        float higher_average = 0.0;
        for (int i = 0; i < higher_count; i++)
        {
            higher_average += higher[i];
        }
        higher_average /= higher_count;

        for (int i = 0; i < higher_count; i++)
        {
            variance_higher += pow(higher[i] - higher_average, 2);
        }
        variance_higher /= higher_count;

        // calculate standard deviation for lower list
        float lower_average = 0.0;
        for (int i = 0; i < lower_count; i++)
        {
            lower_average += lower[i];
        }
        lower_average /= lower_count;

        for (int i = 0; i < lower_count; i++)
        {
            standard_deviation_lower += pow(lower[i] - lower_average, 2);
        }
        standard_deviation_lower = sqrt(standard_deviation_lower / lower_count);

        // write in file
        unlink("sdvar_result.txt");
        int fd;
        fd = open("sdvar_result.txt", O_CREATE | O_RDWR);
        char sp = ' ';
        char newl = '\n';
        if (fd >= 0)
        {
            int av = average;
            writeint(av, fd);
            write(fd, &sp, 1);
            int st = standard_deviation_lower;
            writeint(st, fd);
            write(fd, &sp, 1);
            int va = variance_higher;
            writeint(va, fd);
            write(fd, &newl, 1);
            exit();
        }
        else
        {
            printf(2, "Error opening file for writing.\n");
            exit();
        }
    }
}
