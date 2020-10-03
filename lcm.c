#include "user.h"
#include "fcntl.h"
#include "types.h"
#include "stat.h"
int gcdx(int a, int b)
{
    if (a == 0)
        return b;
    return gcdx(b % a, a);
}

int lcmx(int a, int b)
{
    return (a / gcdx(a, b)) * b;
}

int main(int argc,char* argv[])
{
  int temp = 0;
  if (argc > 1)
  {
    temp = atoi(argv[1]);
    for (int i = 2; i < argc; i++){
        temp = lcmx(temp,atoi(argv[i]));
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
      temp2 = temp2 / 10;
      num++;
    }
    int temp3 = temp;
    for(int i = 0; i < num; i++)
    {
      str[num - i - 1] = '0' + (temp3 % 10);
      temp3 = temp3 / 10;
    }
    str[num] = '\n';
    write(fd, str, num + 1);

  }
  else
  {
    str[0] = '0';
    str[1] = '\n';
    write(fd, str, 2);
  }
  close(fd);
  exit();
}
