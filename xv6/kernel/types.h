typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;

typedef uint64 pde_t;

struct schedstats {
  uint64 arrival_tick;
  uint64 first_run_tick;
  uint64 running_ticks;
  uint64 waiting_ticks;
  uint64 sleeping_ticks;
};
