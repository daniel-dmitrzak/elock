// Addresses of registers
#define S_MR            0x4000
#define S_CR            0x4001
#define S_SR            0x4003
#define PORTL           0x4005
#define PORTH           0x4004
#define DESTIP          0x400C
#define DPORTL          0x4011
#define DPORTH          0x4010

// Commands
#define OPEN            0x01
#define LISTEN          0x02
#define CONNECT         0x04
#define DISCON          0x08
#define CLOSE           0x10
#define SEND            0x20
#define SEND_MAC        0x21
#define SEND_KEEP       0x22
#define RECV            0x30

// Other Vals
#define TOS_VAL         0xF4
#define TTL_VAL         64

void W5200_Init();
void W5200_BurstWrite(int, char*, int);
void W5200_Write(int, char);
int W5200_OpenSocket( int, int, int, char*);