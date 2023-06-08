#include "types.h"
#include "stat.h"
#include "user.h"

// 0   1   2     3
// ln -h [old] [new]
// ln -s [old] [new]
int
main(int argc, char *argv[])
{
  if(argc != 4){
    printf(2, "Usage: ln -op old new\n");
    exit();
  }
  if(link(argv[2], argv[3]) < 0)
    printf(2, "link %s %s: failed\n", argv[2], argv[3]);
  exit();
}
