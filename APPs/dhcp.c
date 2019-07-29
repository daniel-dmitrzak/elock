/*
********************************************************************************
File Include Section
********************************************************************************
*/
#include "stm32f10x.h"
#include "config.h"
#include "socket.h"
#include "w5200.h"
#include "util.h"
#include "dhcp.h"
#include <stdio.h> 
#include <string.h>


/*
********************************************************************************
Define Part
********************************************************************************
*/

/*
********************************************************************************
Local Variable Declaration Section
********************************************************************************
*/

extern u8 txsize[MAX_SOCK_NUM];
extern u8 rxsize[MAX_SOCK_NUM];

extern CONFIG_MSG Config_Msg;

uint8_t DHCP_SIP[4];
uint8_t DHCP_SHA[6];
uint8_t OLD_SIP[4];

char dhcp_state;
char retry_count;

un_l2cval lease_time;
uint32_t my_time, next_time;

uint8_t DHCP_timeout;
uint32_t DHCP_XID;


//RIP_MSG * MSG;
RIP_MSG  MSG; 

char HOST_NAME[14];
uint8_t Cip[4] = {0,0,0,0};

//extern void (*jump_function)(void);	
/*
********************************************************************************
Function Implementation Part
********************************************************************************
*/
/*
*********************************************************************************************************
*              SEND DHCP DISCOVER
*
* Description: This function sends DHCP DISCOVER message to DHCP server.
* Arguments  : s - is a socket number.
* Returns    : None.
* Note       : 
*********************************************************************************************************
*/
void send_DHCP_DISCOVER(uint8_t s)
{
	uint16_t i;
	uint8_t ip[4];
	uint16_t k = 0;

	MSG.op = DHCP_BOOTREQUEST;
	MSG.htype = DHCP_HTYPE10MB;
	MSG.hlen = DHCP_HLENETHERNET;
	MSG.hops = DHCP_HOPS;
	MSG.xid = DHCP_XID;
	MSG.secs = DHCP_SECS;
	MSG.flags = DHCP_FLAGSBROADCAST;

	MSG.ciaddr[0] = 0;
	MSG.ciaddr[1] = 0;
	MSG.ciaddr[2] = 0;
	MSG.ciaddr[3] = 0;

	MSG.yiaddr[0] = 0;
	MSG.yiaddr[1] = 0;
	MSG.yiaddr[2] = 0;
	MSG.yiaddr[3] = 0;

	MSG.siaddr[0] = 0;
	MSG.siaddr[1] = 0;
	MSG.siaddr[2] = 0;
	MSG.siaddr[3] = 0;

	MSG.giaddr[0] = 0;
	MSG.giaddr[1] = 0;
	MSG.giaddr[2] = 0;
	MSG.giaddr[3] = 0;

	MSG.chaddr[0] = Config_Msg.Mac[0];
	MSG.chaddr[1] = Config_Msg.Mac[1];
	MSG.chaddr[2] = Config_Msg.Mac[2];
	MSG.chaddr[3] = Config_Msg.Mac[3];
	MSG.chaddr[4] = Config_Msg.Mac[4];
	MSG.chaddr[5] = Config_Msg.Mac[5];

	for (i = 6; i < 16; i++) MSG.chaddr[i] = 0;
	for (i = 0; i < 64; i++) MSG.sname[i] = 0;
	for (i = 0; i < 128; i++) MSG.file[i] = 0;

	// MAGIC_COOKIE
	MSG.OPT[k++] = 0x63;
	MSG.OPT[k++] = 0x82;
	MSG.OPT[k++] = 0x53;
	MSG.OPT[k++] = 0x63;

	// Option Request Param
	MSG.OPT[k++] = dhcpMessageType;
	MSG.OPT[k++] = 0x01;
	MSG.OPT[k++] = DHCP_DISCOVER;
	
	// Client identifier
	MSG.OPT[k++] = dhcpClientIdentifier;
	MSG.OPT[k++] = 0x07;
	MSG.OPT[k++] = 0x01;
	MSG.OPT[k++] = Config_Msg.Mac[0];
	MSG.OPT[k++] = Config_Msg.Mac[1];
	MSG.OPT[k++] = Config_Msg.Mac[2];
	MSG.OPT[k++] = Config_Msg.Mac[3];
	MSG.OPT[k++] = Config_Msg.Mac[4];
	MSG.OPT[k++] = Config_Msg.Mac[5];
	
	// host name
	MSG.OPT[k++] = hostName;
	MSG.OPT[k++] = 14; // length of hostname
	MSG.OPT[k++] = HOST_NAME[0];
	MSG.OPT[k++] = HOST_NAME[1];
	MSG.OPT[k++] = HOST_NAME[2];
	MSG.OPT[k++] = HOST_NAME[3];
	MSG.OPT[k++] = HOST_NAME[4];
	MSG.OPT[k++] = HOST_NAME[5];
	MSG.OPT[k++] = HOST_NAME[6];
	MSG.OPT[k++] = HOST_NAME[7];
	MSG.OPT[k++] = HOST_NAME[8];
	MSG.OPT[k++] = HOST_NAME[9];
	MSG.OPT[k++] = HOST_NAME[10];
	MSG.OPT[k++] = HOST_NAME[11];
	MSG.OPT[k++] = HOST_NAME[12];
	MSG.OPT[k++] = HOST_NAME[13];


	MSG.OPT[k++] = dhcpParamRequest;
	MSG.OPT[k++] = 0x06;	// length of request
	MSG.OPT[k++] = subnetMask;
	MSG.OPT[k++] = routersOnSubnet;
	MSG.OPT[k++] = dns;
	MSG.OPT[k++] = domainName;
	MSG.OPT[k++] = dhcpT1value;
	MSG.OPT[k++] = dhcpT2value;
	MSG.OPT[k++] = endOption;

	for (i = k; i < OPT_SIZE; i++) MSG.OPT[i] = 0;

	// send broadcasting packet
	for (i = 0; i < 4; i++) ip[i] = 255;

	#ifdef DHCP_DEBUG
		printf("\r\n> send DHCP_DISCOVER");
	#endif
	
	sendto(s, (uint8_t *)(&MSG.op), RIP_MSG_SIZE, ip, DHCP_SERVER_PORT);
}

/*
*********************************************************************************************************
*              SEND DHCP REQUEST
*
* Description: This function sends DHCP REQUEST message to DHCP server.
* Arguments  : s - is a socket number.
* Returns    : None.
* Note       : 
*********************************************************************************************************
*/
void send_DHCP_REQUEST(uint8_t s, uint8_t *Cip, uint8_t *d_addr)
{
	int i;
	uint8_t ip[10];
	uint16_t k = 0;

	MSG.op = DHCP_BOOTREQUEST;
	MSG.htype = DHCP_HTYPE10MB;
	MSG.hlen = DHCP_HLENETHERNET;
	MSG.hops = DHCP_HOPS;
	MSG.xid = DHCP_XID;
	MSG.secs = DHCP_SECS;
	//MSG.flags = DHCP_FLAGSBROADCAST;
	if (d_addr[0] == 0xff) 	MSG.flags = DHCP_FLAGSBROADCAST;
	else MSG.flags = 0;
	
	MSG.ciaddr[0] = Cip[0];
	MSG.ciaddr[1] = Cip[1];
	MSG.ciaddr[2] = Cip[2];
	MSG.ciaddr[3] = Cip[3];

	MSG.yiaddr[0] = 0;
	MSG.yiaddr[1] = 0;
	MSG.yiaddr[2] = 0;
	MSG.yiaddr[3] = 0;

	MSG.siaddr[0] = 0;
	MSG.siaddr[1] = 0;
	MSG.siaddr[2] = 0;
	MSG.siaddr[3] = 0;

	MSG.giaddr[0] = 0;
	MSG.giaddr[1] = 0;
	MSG.giaddr[2] = 0;
	MSG.giaddr[3] = 0;

	MSG.chaddr[0] = Config_Msg.Mac[0];
	MSG.chaddr[1] = Config_Msg.Mac[1];
	MSG.chaddr[2] = Config_Msg.Mac[2];
	MSG.chaddr[3] = Config_Msg.Mac[3];
	MSG.chaddr[4] = Config_Msg.Mac[4];
	MSG.chaddr[5] = Config_Msg.Mac[5];

	for (i = 6; i < 16; i++) MSG.chaddr[i] = 0;
	for (i = 0; i < 64; i++) MSG.sname[i] = 0;
	for (i = 0; i < 128; i++) MSG.file[i] = 0;

	// MAGIC_COOKIE 
	MSG.OPT[k++] = 0x63;
	MSG.OPT[k++] = 0x82;
	MSG.OPT[k++] = 0x53;
	MSG.OPT[k++] = 0x63;

	// Option Request Param.
	MSG.OPT[k++] = dhcpMessageType;
	MSG.OPT[k++] = 0x01;
	MSG.OPT[k++] = DHCP_REQUEST;

	MSG.OPT[k++] = dhcpClientIdentifier;
	MSG.OPT[k++] = 0x07;
	MSG.OPT[k++] = 0x01;
	MSG.OPT[k++] = Config_Msg.Mac[0];
	MSG.OPT[k++] = Config_Msg.Mac[1];
	MSG.OPT[k++] = Config_Msg.Mac[2];
	MSG.OPT[k++] = Config_Msg.Mac[3];
	MSG.OPT[k++] = Config_Msg.Mac[4];
	MSG.OPT[k++] = Config_Msg.Mac[5];

	if (d_addr[0] == 0xff) {
		MSG.OPT[k++] = dhcpRequestedIPaddr;
		MSG.OPT[k++] = 0x04;
		MSG.OPT[k++] = Config_Msg.Lip[0];
		MSG.OPT[k++] = Config_Msg.Lip[1];
		MSG.OPT[k++] = Config_Msg.Lip[2];
		MSG.OPT[k++] = Config_Msg.Lip[3];
	
		MSG.OPT[k++] = dhcpServerIdentifier;
		MSG.OPT[k++] = 0x04;
		MSG.OPT[k++] = DHCP_SIP[0];
		MSG.OPT[k++] = DHCP_SIP[1];
		MSG.OPT[k++] = DHCP_SIP[2];
		MSG.OPT[k++] = DHCP_SIP[3];
	}

	// host name
	MSG.OPT[k++] = hostName;
	MSG.OPT[k++] = 14; // length of hostname
	MSG.OPT[k++] = HOST_NAME[0];
	MSG.OPT[k++] = HOST_NAME[1];
	MSG.OPT[k++] = HOST_NAME[2];
	MSG.OPT[k++] = HOST_NAME[3];
	MSG.OPT[k++] = HOST_NAME[4];
	MSG.OPT[k++] = HOST_NAME[5];
	MSG.OPT[k++] = HOST_NAME[6];
	MSG.OPT[k++] = HOST_NAME[7];
	MSG.OPT[k++] = HOST_NAME[8];
	MSG.OPT[k++] = HOST_NAME[9];
	MSG.OPT[k++] = HOST_NAME[10];
	MSG.OPT[k++] = HOST_NAME[11];
	MSG.OPT[k++] = HOST_NAME[12];
	MSG.OPT[k++] = HOST_NAME[13];
	
	MSG.OPT[k++] = dhcpParamRequest;
	MSG.OPT[k++] = 0x08;
	MSG.OPT[k++] = subnetMask;
	MSG.OPT[k++] = routersOnSubnet;
	MSG.OPT[k++] = dns;
	MSG.OPT[k++] = domainName;
	MSG.OPT[k++] = dhcpT1value;
	MSG.OPT[k++] = dhcpT2value;
	MSG.OPT[k++] = performRouterDiscovery;
	MSG.OPT[k++] = staticRoute;
	MSG.OPT[k++] = endOption;

	for (i = k; i < OPT_SIZE; i++) MSG.OPT[i] = 0;

	// send broadcasting packet
	for (i = 0; i < 4; i++) ip[i] = d_addr[i];

#ifdef DHCP_DEBUG
	printf("\r\n> send DHCP_Request");
#endif
	
	sendto(s, (uint8_t *)(&MSG.op), RIP_MSG_SIZE, ip, DHCP_SERVER_PORT);

}

/*
*********************************************************************************************************
*              SEND DHCP DHCPDECLINE
*
* Description: This function sends DHCP RELEASE message to DHCP server.
* Arguments  : s - is a socket number.
* Returns    : None.
* Note       : 
*********************************************************************************************************
*/
void send_DHCP_DECLINE(uint8_t s)
{
	int i;
	uint8_t ip[10];
	uint16_t k = 0;

	MSG.op = DHCP_BOOTREQUEST;
	MSG.htype = DHCP_HTYPE10MB;
	MSG.hlen = DHCP_HLENETHERNET;
	MSG.hops = DHCP_HOPS;
	MSG.xid = DHCP_XID;
	MSG.secs = DHCP_SECS;
	MSG.flags = 0;

	MSG.ciaddr[0] = 0;
	MSG.ciaddr[1] = 0;
	MSG.ciaddr[2] = 0;
	MSG.ciaddr[3] = 0;

	MSG.yiaddr[0] = 0;
	MSG.yiaddr[1] = 0;
	MSG.yiaddr[2] = 0;
	MSG.yiaddr[3] = 0;

	MSG.siaddr[0] = 0;
	MSG.siaddr[1] = 0;
	MSG.siaddr[2] = 0;
	MSG.siaddr[3] = 0;

	MSG.giaddr[0] = 0;
	MSG.giaddr[1] = 0;
	MSG.giaddr[2] = 0;
	MSG.giaddr[3] = 0;

	MSG.chaddr[0] = Config_Msg.Mac[0];
	MSG.chaddr[1] = Config_Msg.Mac[1];
	MSG.chaddr[2] = Config_Msg.Mac[2];
	MSG.chaddr[3] = Config_Msg.Mac[3];
	MSG.chaddr[4] = Config_Msg.Mac[4];
	MSG.chaddr[5] = Config_Msg.Mac[5];

	for (i = 6; i < 16; i++) MSG.chaddr[i] = 0;
	for (i = 0; i < 64; i++) MSG.sname[i] = 0;
	for (i = 0; i < 128; i++) MSG.file[i] = 0;

	// MAGIC_COOKIE
	MSG.OPT[k++] = 0x63;
	MSG.OPT[k++] = 0x82;
	MSG.OPT[k++] = 0x53;
	MSG.OPT[k++] = 0x63;

	// Option Request Param.
	MSG.OPT[k++] = dhcpMessageType;
	MSG.OPT[k++] = 0x01;
	MSG.OPT[k++] = DHCP_DECLINE;

	MSG.OPT[k++] = dhcpClientIdentifier;
	MSG.OPT[k++] = 0x07;
	MSG.OPT[k++] = 0x01;
	MSG.OPT[k++] = Config_Msg.Mac[0];
	MSG.OPT[k++] = Config_Msg.Mac[1];
	MSG.OPT[k++] = Config_Msg.Mac[2];
	MSG.OPT[k++] = Config_Msg.Mac[3];
	MSG.OPT[k++] = Config_Msg.Mac[4];
	MSG.OPT[k++] = Config_Msg.Mac[5];

	MSG.OPT[k++] = dhcpRequestedIPaddr;
	MSG.OPT[k++] = 0x04;
	MSG.OPT[k++] = Config_Msg.Lip[0];
	MSG.OPT[k++] = Config_Msg.Lip[1];
	MSG.OPT[k++] = Config_Msg.Lip[2];
	MSG.OPT[k++] = Config_Msg.Lip[3];

	MSG.OPT[k++] = dhcpServerIdentifier;
	MSG.OPT[k++] = 0x04;
	MSG.OPT[k++] = DHCP_SIP[0];
	MSG.OPT[k++] = DHCP_SIP[1];
	MSG.OPT[k++] = DHCP_SIP[2];
	MSG.OPT[k++] = DHCP_SIP[3];

	MSG.OPT[k++] = endOption;

	for (i = k; i < OPT_SIZE; i++) MSG.OPT[i] = 0;

	//send broadcasting packet
	ip[0] = 255;
	ip[1] = 255;
	ip[2] = 255;
	ip[3] = 255;

//	printf("> send DHCP_Decline\r\n");

	sendto(s, (uint8_t *)(&MSG.op), RIP_MSG_SIZE, ip, DHCP_SERVER_PORT);
}

/*
*********************************************************************************************************
*              PARSE REPLY MSG
*
* Description: This function parses the reply message from DHCP server.
* Arguments  : s      - is a socket number.
*              length - is a size data to receive.
* Returns    : None.
* Note       : 
*********************************************************************************************************
*/
char parseDHCPMSG(uint8_t s, uint16_t length)
{
	uint8_t svr_addr[6];
	uint16_t  svr_port;

	uint16_t i, len;
	uint8_t * p;
	uint8_t * e;
	uint8_t type, opt_len;

	len = recvfrom(s, (uint8_t *)&MSG.op, length, svr_addr, &svr_port);
	
	if (svr_port == DHCP_SERVER_PORT) {

		for (i = 0; i < 6; i++)
			if (MSG.chaddr[i] != Config_Msg.Mac[i]) {
				type = 0;
				goto PARSE_END;
			}

		for (i = 0; i < 4; i++) {
			Config_Msg.Lip[i] = MSG.yiaddr[i];
		}
		
		type = 0;
		p = (uint8_t *)(&MSG.op);
		p = p + 240;
		e = p + (len - 240);

		while ( p < e ) {
			switch ( *p ) {

			case endOption :
				goto PARSE_END;
       			case padOption :
				p++;
				break;
			case dhcpMessageType :
				p++;
				p++;
				type = *p++;
				break;
			case subnetMask :
				p++;
				p++;
				for (i = 0; i < 4; i++)	 Config_Msg.Sub[i] = *p++;
				break;
			case routersOnSubnet :
				p++;
				opt_len = *p++;       
				for (i = 0; i < 4; i++)	Config_Msg.Gw[i] = *p++;
				for (i = 0; i < (opt_len-4); i++) p++;
				break;
			case dns :
				p++;                  
				opt_len = *p++;       
				for (i = 0; i < 4; i++)	Config_Msg.DNS_Server_IP[i] = *p++;
				if (opt_len >= 8) {
					for (i = 0; i < 4; i++) Config_Msg.DNS_Server_IP[i] = *p++;
				}
				for (i = 0; i < (opt_len-8); i++) p++;
				break;
				
			case dhcpIPaddrLeaseTime :
				p++;
				opt_len = *p++;
				lease_time.cVal[3] = *p++;
				lease_time.cVal[2] = *p++;
				lease_time.cVal[1] = *p++;
				lease_time.cVal[0] = *p++;
				break;

			case dhcpServerIdentifier :
				p++;
				opt_len = *p++;
				DHCP_SIP[0] = *p++;
				DHCP_SIP[1] = *p++;
				DHCP_SIP[2] = *p++;
				DHCP_SIP[3] = *p++;
				break;

			default :
				p++;
				opt_len = *p++;
				p += opt_len;
				break;
			} // switch
		} // while
	} // if

PARSE_END :
	return	type;
}

/*
*********************************************************************************************************
*              CHECK DHCP STATE
*
* Description: This function checks the state of DHCP.
* Arguments  : None.
* Returns    : None.
* Note       : 
*********************************************************************************************************
*/
void check_DHCP_state(void) 
{
	uint16_t len, i;
	uint8_t type, flag;
	uint8_t d_addr[4];
	
	type = 0;
	
	if ((len = getSn_RX_RSR(SOCK_DHCP)) > 0) {
		type = parseDHCPMSG(SOCK_DHCP, len);
	}
	
	switch ( dhcp_state ) {
		case STATE_DHCP_DISCOVER :
			if (type == DHCP_OFFER) {
				
				#ifdef DHCP_DEBUG
					printf("\r\n< Receive DHCP_OFFER");
				#endif
				
				for (i = 0; i < 4; i++) d_addr[i] = 0xff;
				send_DHCP_REQUEST(SOCK_DHCP, Cip, d_addr);
				
				dhcp_state = STATE_DHCP_REQUEST;
			} else check_Timeout();
		break;

		case STATE_DHCP_REQUEST :
			if (type == DHCP_ACK) {
				my_time = 0;
				
				#ifdef DHCP_DEBUG
					printf("\r\n< Receive DHCP_ACK");
				#endif
				
				iinchip_init();
				Set_network();

				my_time = 0;
				next_time = my_time + DHCP_WAIT_TIME;
				retry_count = 0;
				dhcp_state = STATE_DHCP_LEASED;
			} else if (type == DHCP_NAK) {
				//printf("\r\n< Receive DHCP_NACK");
				my_time = 0;
				next_time = my_time + DHCP_WAIT_TIME;
				retry_count = 0;
				dhcp_state = STATE_DHCP_DISCOVER;
			} else check_Timeout();
		break;

		case STATE_DHCP_LEASED :
			if ((lease_time.lVal != 0xffffffff) && ((lease_time.lVal/2) < my_time)) {
				
				#ifdef DHCP_DEBUG
				printf("\r\nRenewal IP address ");
				#endif
				
				socket(SOCK_DHCP, Sn_MR_UDP, DHCP_CLIENT_PORT, 0x00);

				type = 0;
				for (i = 0; i < 4; i++)	OLD_SIP[i] = Config_Msg.Lip[i];
				for (i = 0; i < 4; i++)	d_addr[i] = DHCP_SIP[i];
				
				DHCP_XID++;
				send_DHCP_REQUEST(SOCK_DHCP, Config_Msg.Lip, d_addr);
				dhcp_state = STATE_DHCP_REREQUEST;

				//my_time = 0;
				next_time = my_time + DHCP_WAIT_TIME;
			}
		break;

		case STATE_DHCP_REREQUEST :
			if (type == DHCP_ACK) {
				retry_count = 0;
				flag = 0;
				for (i = 0; i < 4; i++)	{
					if (OLD_SIP[i] != Config_Msg.Lip[i]) {
						flag = 1;
						break;
					}
				}
				
				// change to new IP address
				if (flag) {
					iinchip_init();
					Set_network();
				}
				
				
				#ifdef DHCP_DEBUG
					printf("\r\nACK");
				#endif

				my_time = 0;
				next_time = my_time + DHCP_WAIT_TIME;

				dhcp_state = STATE_DHCP_LEASED;
			} else if (type == DHCP_NAK) {
				my_time = 0;
				next_time = my_time + DHCP_WAIT_TIME;
				retry_count = 0;
				dhcp_state = STATE_DHCP_DISCOVER;
				//printf("state : STATE_DHCP_DISCOVER\r\n");
				#ifdef DHCP_DEBUG
					printf("\r\nNAK");
				#endif
			} else {
				#ifdef DHCP_DEBUG
					printf("\r\ncheck_Timeout");
				#endif
				check_Timeout();
			}
				
		break;

		case STATE_DHCP_RELEASE :
		break;

		default :
		break;
	}
}

/*
*********************************************************************************************************
*              CHECK TIMEOUT
*
* Description: This function checks the timeout of DHCP in each state.
* Arguments  : None.
* Returns    : None.
* Note       : 
*********************************************************************************************************
*/
void check_Timeout(void)
{
	uint8_t i, d_addr[4];


	if (retry_count < MAX_DHCP_RETRY) {
		if (next_time < my_time) {
			my_time = 0;
			next_time = my_time + DHCP_WAIT_TIME;
			retry_count++;
			switch ( dhcp_state ) {
				case STATE_DHCP_DISCOVER :
					//printf("<<timeout>> state : STATE_DHCP_DISCOVER\r\n");
					send_DHCP_DISCOVER(SOCK_DHCP);
				break;
		
				case STATE_DHCP_REQUEST :
//					printf("<<timeout>> state : STATE_DHCP_REQUEST\r\n");

					for (i = 0; i < 4; i++) d_addr[i] = 0xff;
					send_DHCP_REQUEST(SOCK_DHCP, Cip, d_addr);
				break;

				case STATE_DHCP_REREQUEST :
//					printf("<<timeout>> state : STATE_DHCP_REREQUEST\r\n");
					
					for (i = 0; i < 4; i++)	d_addr[i] = DHCP_SIP[i];
					send_DHCP_REQUEST(SOCK_DHCP, Config_Msg.Lip, d_addr);
					
				break;
		
				default :
				break;
			}
		}
	} else {
		my_time = 0;
		next_time = my_time + DHCP_WAIT_TIME;
		retry_count = 0;

		DHCP_timeout = 1;

		/* open a socket in IP RAW mode for DHCP */
		socket(SOCK_DHCP, Sn_MR_UDP, DHCP_CLIENT_PORT, 0x00);
		send_DHCP_DISCOVER(SOCK_DHCP);
		dhcp_state = STATE_DHCP_DISCOVER;
	}
}

/*
*********************************************************************************************************
*              Get an IP from the DHCP server.
*
* Description: 
* Arguments  : None.
* Returns    : None.
* Note       : 
*********************************************************************************************************
*/
unsigned char getIP_DHCPS(void)
{
	uint8_t ip[4];
	
	DHCP_XID = 0x12345678;

	setSHAR(Config_Msg.Mac);

	// SRC IP
	ip[0] = 0;
	ip[1] = 0;
	ip[2] = 0;
	ip[3] = 0;
	setSIPR(ip);
	setGAR(ip);
	setSUBR(ip);

	setRCR(3);
	sysinit(txsize, rxsize);

	sprintf(HOST_NAME,"WIZ120SR%02x%02x%02x",Config_Msg.Mac[3],Config_Msg.Mac[4],Config_Msg.Mac[5]);	

	wait_10ms(500);
	
	socket(SOCK_DHCP, Sn_MR_UDP, DHCP_CLIENT_PORT, 0x00);
	
	send_DHCP_DISCOVER(SOCK_DHCP);
	
	dhcp_state = STATE_DHCP_DISCOVER;
	DHCP_timeout = 0;
	my_time = 0;
	next_time = my_time + DHCP_WAIT_TIME;
	retry_count = 0;

	while (dhcp_state != STATE_DHCP_LEASED)
	{

		if (DHCP_timeout == 1) return(0);
		check_DHCP_state();
		
	}
	
	return 1;
}

