#include "user.h"
#include "fcntl.h"
#include "types.h"
#include "stat.h"
int gcd(int a, int b)
{
    if (a == 0)
        return b;
    return gcd(b % a, a);
}

int lcm(int a, int b)
{
    return (a / gcd(a, b)) * b;
}

int main(int argc,char* argv[])
{
  int temp = 0;
  if (argc > 1)
  {
    int temp = atoi(argv[1]);
    for (int i = 2; i < argc; i++){
        temp = lcm(temp,atoi(argv[i]));
    }
  }
  int fd = open("lcm_result.txt", O_CREATE | O_RDWR);
  char str[50];
  if (temp != 0)
  {
    int temp2 = temp;
    int num = 0;
    while (temp2)
    {
      temp2 /= 10;
      num += 1;
    }
    temp2 = temp;
    for(int i = 0; i < num; i++)
    {
      str[num - i - 1] = '0' + (temp2 % 10);
      temp2 /= 10;
    }
    write(fd, str, num);
  }
  else
  {
    str[0] = '0';
    write(fd, str, 1);
  }
  close(fd);
}
