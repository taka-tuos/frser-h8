// Modified by taka-tuos
// original: "Arduino Mega flasher by fritz"
// https://web.archive.org/web/20120801025050/http://www.coldelectrons.com/blog/wp-content/uploads/2010/05/flashprg.txt

#include "frser.h"

#define FLASH_ADDR 0x40000

// Using an Arduino Mega for:
//      * Bit-bang parallel bus for flashing roms
//
/********************************************************************/
/*                                                                  */
/* global variables                                                 */
/*                                                                  */
/********************************************************************/

#define S_BUSTYPE           0x01 //PARALLEL BUS ONLY
#define S_IFACE             0x0001 //SERPROG INTERFACE VERSION
#define S_PROGNAME          "H8/3048F-SERPROG"
#define S_SERBUF            128 //SIZE OF SERIAL BUFFER IN RAM
#define S_CHIPSIZE          18 // NUMBER OF ADDRESS LINES
#define S_OPBUF             4096 // SIZE OF OPERATION BUFFER
#define S_WRNMAXLEN         1 // MAX WE CAN WRITE AT ONCE
#define S_RDNMAXLEN         262144 // MAX SIZE OF THE ADDRESSABLE SPACE?

#define MAP_NOP             1 << 0
#define MAP_Q_IFACE         1 << 1
#define MAP_Q_CMDMAP        1 << 2
#define MAP_Q_PGMNAME       1 << 3
#define MAP_Q_SERBUF        1 << 4
#define MAP_Q_BUSTYPE       1 << 5
#define MAP_Q_CHIPSIZE      1 << 6
#define MAP_Q_OPBUF         1 << 7
#define MAP_Q_WRNMAXLEN     1 << 0
#define MAP_R_BYTE          1 << 1
#define MAP_R_NBYTES        1 << 2
#define MAP_O_INIT          1 << 3
#define MAP_O_WRITEB        1 << 4
#define MAP_O_WRITEN        0 << 5
#define MAP_O_DELAY         1 << 6
#define MAP_O_EXEC          1 << 7
#define MAP_SYNCNOP         1 << 0
#define MAP_Q_RDNMAXLEN     1 << 1
#define MAP_S_BUSTYPE       0 << 2
#define S_CMDMAP0           MAP_NOP         |\
                            MAP_Q_IFACE     |\
                            MAP_Q_CMDMAP    |\
                            MAP_Q_PGMNAME   |\
                            MAP_Q_SERBUF    |\
                            MAP_Q_BUSTYPE   |\
                            MAP_Q_CHIPSIZE  |\
                            MAP_Q_OPBUF
#define S_CMDMAP1           MAP_Q_WRNMAXLEN |\
                            MAP_R_BYTE      |\
                            MAP_R_NBYTES    |\
                            MAP_O_INIT      |\
                            MAP_O_WRITEB    |\
                            MAP_O_WRITEN    |\
                            MAP_O_DELAY     |\
                            MAP_O_EXEC
#define S_CMDMAP2           MAP_SYNCNOP     |\
                            MAP_Q_RDNMAXLEN |\
                            MAP_S_BUSTYPE  

#define S_ACK               0x06
#define S_NAK               0x15
#define S_CMD_NOP           0x00 //No operation
#define S_CMD_Q_IFACE       0x01 //Query interface version
#define S_CMD_Q_CMDMAP      0x02 //Query supported commands bitmap
#define S_CMD_Q_PGMNAME     0x03 //Query programmer name
#define S_CMD_Q_SERBUF      0x04 //Query Serial Buffer Size
#define S_CMD_Q_BUSTYPE     0x05 //Query supported bustypes
#define S_CMD_Q_CHIPSIZE    0x06 //Query supported chipsize (2^n format)
#define S_CMD_Q_OPBUF       0x07 //Query operation buffer size
#define S_CMD_Q_WRNMAXLEN   0x08 //Query Write to opbuf: Write-N maximum lenght
#define S_CMD_R_BYTE        0x09 //Read a single byte
#define S_CMD_R_NBYTES      0x0A //Read n bytes
#define S_CMD_O_INIT        0x0B //Initialize operation buffer
#define S_CMD_O_WRITEB      0x0C //Write opbuf: Write byte with address
#define S_CMD_O_WRITEN      0x0D //Write to opbuf: Write-N
#define S_CMD_O_DELAY       0x0E //Write opbuf: udelay
#define S_CMD_O_EXEC        0x0F //Execute operation buffer
#define S_CMD_SYNCNOP       0x10 //Special no-operation that returns NAK+ACK
#define S_CMD_Q_RDNMAXLEN   0x11 //Query read-n maximum length
#define S_CMD_S_BUSTYPE     0x12 //Set used bustype(s).

/********************************************************************/
/*                                                                  */
/* Pins used                                                        */
/*                                                                  */
/********************************************************************/


/********************************************************************/
/*                                                                  */
/* Functions                                                        */
/*                                                                  */
/********************************************************************/

volatile byte* flashptr;

byte chip_readb(unsigned long address) {
    return flashptr[address];
}

void chip_writeb(byte b, unsigned long address) {
    flashptr[address] = b;
}

// a blocking serial input, so I don't have to sprinkle my code
// full of conditionals
byte poll_serial() {
    byte b;
    while (rs_rx_buff() == 0) {
        // wait until a byte comes in
    }
    b = rs_getc();
    return b;
}

void send_serial(byte b) {
    rs_putc(b);
}

void send_serial_string(char *s) {
    for(byte *p = (byte *)s; *p; p++) send_serial(*p);
}

#define lowByte(d) ((d) & 0xff)
#define highByte(d) (((d) >> 8) & 0xff)

/********************************************************************/
/*                                                                  */
/* Main Functions                                                   */
/*                                                                  */
/********************************************************************/


// Init
void setup() {
    // main.cの方であらかた済ませてある

    // アドレス設定
    flashptr = (volatile byte *)((MDCR.BYTE & 7) == 5 ? 0x40000 : 0x400000);
}

// Main
void loop() {
    byte b, cb;
    unsigned long i;
    unsigned long addr = 0;
    unsigned long length = 0;
    volatile unsigned char opbuffer[S_OPBUF];
    unsigned int x, opptr = 0;

    // main main loop
    while(1) {
        b = poll_serial();
        switch(b) {
            case S_CMD_NOP:
                send_serial(S_ACK);
                break;
            case S_CMD_Q_IFACE:
                send_serial(S_ACK);
                send_serial(S_IFACE);
                send_serial(0);
                break;
            case S_CMD_Q_CMDMAP:
                send_serial(S_ACK);
                send_serial(S_CMDMAP0);
                send_serial(S_CMDMAP1);
                send_serial(S_CMDMAP2);
                for (i=0; i<29; i++) {
                    send_serial(0);
                }
                break;
            case S_CMD_Q_PGMNAME:
                send_serial(S_ACK);
                send_serial_string(S_PROGNAME);
                break;
            case S_CMD_Q_SERBUF:
                send_serial(S_ACK);
                send_serial(S_SERBUF);
                send_serial(0);
                break;
            case S_CMD_Q_BUSTYPE:
                send_serial(S_ACK);
                send_serial(S_BUSTYPE);
                break;
            case S_CMD_Q_CHIPSIZE:
                send_serial(S_ACK);
                send_serial(S_CHIPSIZE);
                break;
            case S_CMD_Q_OPBUF:
                send_serial(S_ACK);
                send_serial(lowByte(S_OPBUF));
                send_serial(highByte(S_OPBUF));
                break;
            case S_CMD_Q_WRNMAXLEN:
                send_serial(S_ACK);
                send_serial(lowByte(S_WRNMAXLEN));
                send_serial(highByte(S_WRNMAXLEN));
                send_serial(0);
                break;
            case S_CMD_R_BYTE:
                addr = poll_serial();
                addr |= ((DWORD)poll_serial()) << 8;
                addr |= ((DWORD)poll_serial()) << 16;
                cb = chip_readb(addr);
                send_serial(S_ACK);
                send_serial(cb);
                break;
            case S_CMD_R_NBYTES:
                addr = poll_serial();
                addr |= ((DWORD)poll_serial()) << 8;
                addr |= ((DWORD)poll_serial()) << 16;
                length = poll_serial();
                length |= ((DWORD)poll_serial()) << 8;
                length |= ((DWORD)poll_serial()) << 16;
                send_serial(S_ACK);
                for (i=0; i<length; i++) {
                    cb = chip_readb(addr + i);
                    send_serial(cb);
                }
                break;
            case S_CMD_O_INIT:
                for (x=0; x<S_OPBUF; x++) {
                    opbuffer[x] = 0;
                }
                opptr = 0;
                send_serial(S_ACK);
                break;
            case S_CMD_O_WRITEB:
                opbuffer[opptr++] = S_CMD_O_WRITEB;
                opbuffer[opptr++] = poll_serial();
                opbuffer[opptr++] = poll_serial();
                opbuffer[opptr++] = poll_serial();
                opbuffer[opptr++] = poll_serial();
                send_serial(S_ACK);
                break;
            case S_CMD_O_WRITEN:
//                opbuffer[opptr++] = S_CMD_O_WRITEN;
//                opbuffer[opptr++] = poll_serial();
//                opbuffer[opptr++] = poll_serial();
//                opbuffer[opptr++] = poll_serial();
//                cb = poll_serial();
//                length = long(cb);
//                opbuffer[opptr++] = cb;
//                cb = poll_serial();
//                length |= long(cb) << 8;
//                opbuffer[opptr++] = cb;
//                cb = poll_serial();
//                length |= long(cb) << 16;
//                opbuffer[opptr++] = cb;
//                for (i=0; i<length; i++) {
//                    opbuffer[opptr++] = poll_serial();
//                }
//                send_serial(S_ACK, BYTE);
                send_serial(S_NAK);
                break;
            case S_CMD_O_DELAY:
                opbuffer[opptr++] = S_CMD_O_DELAY;
                opbuffer[opptr++] = poll_serial();
                opbuffer[opptr++] = poll_serial();
                opbuffer[opptr++] = poll_serial();
                opbuffer[opptr++] = poll_serial();
                send_serial(S_ACK);
                break;
            case S_CMD_O_EXEC:
                i = 0;
                while (i<opptr) {
                    switch (opbuffer[i++]) {
                        case S_CMD_O_WRITEB:
                            addr = opbuffer[i++];
                            addr |= ((DWORD)opbuffer[i++]) << 8;
                            addr |= ((DWORD)opbuffer[i++]) << 16;
                            cb = opbuffer[i++];
                            chip_writeb(cb, addr);
                            break;
                        case S_CMD_O_WRITEN:
                            addr = opbuffer[i++];
                            addr |= ((DWORD)opbuffer[i++]) << 8;
                            addr |= ((DWORD)opbuffer[i++]) << 16;
                            length = opbuffer[i++];
                            length |= ((DWORD)opbuffer[i++]) << 8;
                            length |= ((DWORD)opbuffer[i++]) << 16;
                            for (x=0; x<length; x++) {
                                chip_writeb(opbuffer[i++], addr + x);
                            }
                            break;
                        case S_CMD_O_DELAY:
                            length = opbuffer[i++];
                            length |= ((DWORD)opbuffer[i++]) << 8;
                            length |= ((DWORD)opbuffer[i++]) << 16;
                            //delayMicroseconds(length);
                            break;
                    }
                }
                opptr = 0;
                send_serial(S_ACK);
                break;
            case S_CMD_SYNCNOP:
                send_serial(S_NAK);
                send_serial(S_ACK);
                break;
            case S_CMD_Q_RDNMAXLEN:
                send_serial(S_ACK);
                send_serial(0x00);
                send_serial(0x00);
                send_serial(0x00);
                break;
            case S_CMD_S_BUSTYPE:
                send_serial(S_NAK);
                break;
        }
    }
}