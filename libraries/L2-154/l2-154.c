#include "l2-154.h"


l2addr_154 *l2addr_154_broadcast;
addr2_t addr2_broadcast = CONST16 (0xff, 0xff) ;


/*
 * We assume a fixed size MAC header which includes:
 * 2 bytes : FCF (with the intra-PAN bit set)
 * 1 byte  : Sequence number
 * 2 bytes : destination address
 * 2 bytes : destination PANID
 * 2 bytes : source address (without PANID since the intra-PAN bit is set)
 * and no auxiliary security header field
 *
 * The MAC footer includes a 2 bytes checksum.
 *
 * Hence, the MSDU (MAC service Data Unit) length is 127 - header - fcs
 */

#define	I154_SIZE_HEADER	(2+1+2+2+2)
#define	I154_SIZE_FCS		2		// CRC-16 checksum at the end



void freel2addr_154(l2addr_154 *addr) {
	free(addr);
}


l2addr_154 *init_l2addr_154_char(const char *a)
{
	l2addr_154 *addr = (l2addr_154 *)malloc(sizeof(struct l2addr_154));
	if (addr == NULL)
		printf("Memory allocation failed\n");
    int i = 0 ;
    uint8_t b = 0 ;
    uint8_t buf [I154_ADDRLEN] ;

    /*
     * General loop, when 8-byte addresses will be supported
     */

    while (*a != '\0' && i < I154_ADDRLEN)
    {
	if (*a == ':')
	{
	    buf [i++] = b ;
	    b = 0 ;
	}
	else if (isxdigit (*a))
	{
	    uint8_t x ;
	    char c ;

	    c = tolower (*a) ;
	    x = isdigit (c) ? (c - '0') : (c - 'a' + 10) ;
	    b = (b << 4) + x ;
	}
	else
	{
	    for (i = 0 ; i < I154_ADDRLEN ; i++)
		buf [i] = 0 ;
	    break ;
	}
	a++ ;
    }
    if (i < I154_ADDRLEN)
	buf [i] = b ;
	
	addr-> addr_ = CONST16 (buf [0], buf [1]) ;
    return addr;
}


void setBroasdcastAddr(void) {
	l2addr_154_broadcast = init_l2addr_154_char("ff:ff");	
}





l2addr_154 *init_l2addr_154_addr(const l2addr_154 *x){
	l2addr_154 *addr =(l2addr_154 *)malloc(sizeof(struct l2addr_154));
	if (addr == NULL)
		printf("Memory allocation failed\n");
	addr -> addr_ = x->addr_ ;
	return addr;
}

void copyAddr(l2addr_154 *x, const l2addr_154 *y) {
	x->addr_ = y->addr_;
}


bool isEqualAddr(const l2addr_154 *x, const l2addr_154 *y) {
	l2addr_154 *oe = (l2addr_154 *)y;
	return x->addr_ == oe->addr_;
}

void printAddr (const l2addr_154 *x)  {
	printf("%x",BYTE_LOW(x->addr_));
	printf(" : " );
	printf("%x",BYTE_HIGH(x->addr_) );
}


void printAddr_2 (const addr2_t x)  {
	printf("%x",BYTE_LOW(x));
	printf(" : " );
	printf("%x",BYTE_HIGH(x) );
}




/******************************************************************************
 * l2net_154 methods
 */

// void startL2_154 (l2net_154 *l2, l2addr_154 *a, channel_t chan, panid_t panid) {
	
// 	l2->myaddr_ = a ->addr_;
// 	conmsg = (ConMsg * ) malloc (sizeof(ConMsg));
//     setAddr2 ( l2->myaddr_) ;
//     setChannel ( chan) ;
//     setPanid ( panid) ;
//     setMsgbufsize(10);
//     setBroasdcastAddr();
//     l2->mtu_ = I154_MTU ;

//     l2->curframe_ = NULL;   // no currently received frame

//     start () ;

// }


l2net_154* startL2_154 ( l2addr_154 *a, channel_t chan, panid_t panid) {
	l2net_154 *l2 = (l2net_154 *) malloc (sizeof(l2net_154));
	if (l2 == NULL)
		printf("Memory allocation failed\n");
	l2->myaddr_ = a ->addr_;

	conmsg = (ConMsg * ) malloc (sizeof(ConMsg));
	if (conmsg == NULL)
		printf("Memory allocation failed\n");
    setAddr2 ( l2->myaddr_) ;
    setChannel ( chan) ;
    setPanid ( panid) ;
    setMsgbufsize(10);
    setBroasdcastAddr();
    l2->mtu_ = I154_MTU ;

    l2->curframe_ = NULL;   // no currently received frame

    start () ;
    return l2;

}

size_t maxpayload (l2net_154 *l2) {
	return l2->mtu_ - (I154_SIZE_HEADER + I154_SIZE_FCS) ;
}


bool send (l2net_154 *l2, l2addr_154 *dest, const uint8_t *data, size_t len) {
	bool success = false;

	if (len <= l2->mtu_ - (I154_SIZE_HEADER + I154_SIZE_FCS))
		success = sendto ( ( dest)->addr_, data, len) ;
	return success;
}



/**
 * @brief Receive a packet from the IEEE 802.15.4 network
 *	
 * This method queries the ZigMsg library in order. The received
 * packet is kept by the ZigMsg library in a private buffer.
 * Minimal decoding is done here in order to decide if it is
 * a valid packet (i.e. uses only 16-bits address and an Intra-PAN
 * bit).
 *
 * See the `l2net::l2_recv_t` enumeration for return values.
 */

l2_recv_t recv (l2net_154 *l2) 
{
    l2_recv_t r ;
    if (l2->curframe_ != NULL) {
    	skip_received();
    }

    l2->curframe_ = get_received();
    if (l2->curframe_ != NULL
	    && l2->curframe_->frametype == Z_FT_DATA
	    && Z_GET_DST_ADDR_MODE (l2->curframe_->fcf) == Z_ADDRMODE_ADDR2
	    && Z_GET_SRC_ADDR_MODE (l2->curframe_->fcf) == Z_ADDRMODE_ADDR2
	    && Z_GET_INTRA_PAN (l2->curframe_->fcf)
	    )
    {
    		
    	if (l2->curframe_->dstaddr != l2->myaddr_ && l2->curframe_->dstaddr != addr2_broadcast)
	    	r = RECV_WRONG_DEST ;
	    else{
	    	r = RECV_OK ;
	    }
    }else r = RECV_EMPTY ;

    return r;
}


/**
 * @brief Returns the broadcast IEEE 802.15.4 address
 *
 * The broadcast IEEE 802.15.4 address is located in a global variable.
 * This method returns its address.
 *
 * @return address of an existing l2addr_154 object (do not free it)
 */

l2addr_154 *bcastaddr (void) {
	return l2addr_154_broadcast;
}


/**
 * @brief Returns the source address of the received frame
 *
 * This methods creates a new l2addr_154 and initializes it with
 * the source address from the currently received frame.
 *
 * @return address of a new l2addr_154 object (to delete after use)
 */

l2addr_154 *get_src (l2net_154 *l2)
{
    l2addr_154 *a = (l2addr_154 *)malloc(sizeof(struct l2addr_154));
    if (a == NULL)
		printf("Memory allocation failed\n");
    a->addr_ = l2->curframe_->srcaddr ;
    return a ;
}



/**
 * @brief Returns the destination address of the received frame
 *
 * This methods creates a new l2addr_154 and initializes it with
 * the destination address from the currently received frame.
 *
 * @return address of a new l2addr_154 object (to delete after use)
 */

l2addr_154 *get_dst (l2net_154 *l2)
{
    l2addr_154 *a = (l2addr_154 *)malloc(sizeof(struct l2addr_154));
    if (a == NULL)
		printf("Memory allocation failed\n");
    a->addr_ = l2->curframe_->dstaddr ;
    return a ;
}



/**
 * @brief Returns the address of the received payload
 *
 * This methods returns the address of the payload inside the
 * received frame (payload is not copied).
 *
 * @return address inside an existing buffer (do not free it)
 */

uint8_t *get_payload (l2net_154 *l2, int offset) 
{
    return l2->curframe_->payload ;
}



/**
 * @brief Returns the payload length
 *
 * This methods returns the original length of the received
 * frame. Even if the frame has been truncated on reception,
 * the payload returned is the true payload.
 *
 * @return original length
 */

size_t get_paylen (l2net_154 *l2) 
{
    return l2->curframe_->paylen ;
}



/**
 * @brief Dump some parts of a frame
 *
 * This methods prints a part of the received frame.
 */

void dump_packet (l2net_154 *l2, size_t start, size_t maxlen)
{
    size_t i, n ;

    n = start + maxlen ;
    if (l2->curframe_->rawlen < n)
		n = l2->curframe_->rawlen ;

    for (i = start ; i < n ; i++)
    {
		if (i > start)
		    printf (' ') ;
		printf("%x", (l2->curframe_->rawframe [i] >> 4) & 0xf) ;
		printf("%x", (l2->curframe_->rawframe [i]) & 0xf) ;
    }
    printf("\n");
}


void setMTU(l2net_154 *l2, size_t mtu) {
	l2->mtu_ = mtu;
}

size_t getMTU(l2net_154 *l2) {
	return l2->mtu_;
}