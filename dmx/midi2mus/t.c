#include <fcntl.h>

main()
{
  open("fart",O_WRONLY|O_CREAT|O_TRUNC,0666);
}
