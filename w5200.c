#include "w5200.h"
#include "w5200_config.h"
#include "stm32f0xx_spi.h"

void W5200_Init(){
  for ( int addr = 1; addr <= 0x12; addr++ ){
   W5200_Write(addr, reg_conf[addr-1]);
  }
  //W5200_BurstWrite(0x0001, reg_conf, sizeof(reg_conf));
}

void W5200_BurstWrite(int addr, char* data, int datalen){
  char OP = 0x80;
  char header[4] = { ((addr & 0xFF00) >> 8), (addr & 0x00FF), (((datalen & 0xFF00) >> 8) | OP), (datalen & 0x00FF) };
  SPI_SSOutputCmd(SPI1, ENABLE);
  for ( int j = 0; j < 4; j++){
    SPI_SendData8(SPI1, header[j]);
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY));
  }
  for ( int j = 0; j < datalen; j++ ){
    SPI_SendData8(SPI1, data[j]);
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY));
  }
  SPI_SSOutputCmd(SPI1, DISABLE);
}

void W5200_Write(int addr, char data){
  char OP = 0x80;
  int datalen = 1;
  char header[4] = { ((addr & 0xFF00) >> 8), (addr & 0x00FF), (((datalen & 0xFF00) >> 8) | OP), (datalen & 0x00FF) };
  SPI_SSOutputCmd(SPI1, ENABLE);
  for ( int j = 0; j < 4; j++){
    SPI_SendData8(SPI1, header[j]);
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY));
  }
  SPI_SendData8(SPI1, data);
  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY));
  SPI_SSOutputCmd(SPI1, DISABLE);
}

int W5200_OpenSocket( int num, int protocol, int port, char* ip){
  int socket = num << 8;
  W5200_Write( S_MR + socket, (protocol & 0x0F) );
  
  W5200_Write( PORTL + socket, (port & 0xFF) );
  W5200_BurstWrite( DESTIP + socket, ip, 4 );
  W5200_Write( PDESTL + socket, port & 0xff );
  
  
  W5200_Write( S_CR + socket, OPEN );
  W5200_Write( S_CR + socket, CONNECT );
  return 0;
}