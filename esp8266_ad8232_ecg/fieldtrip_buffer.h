#ifndef __BUFFER_H_
#define __BUFFER_H_

WiFiClient client;

// definition of simplified interface functions, not all of them are implemented
int fieldtrip_start_server(int port);
int fieldtrip_open_connection(const char *hostname, int port);
int fieldtrip_close_connection(int s);
int fieldtrip_read_header(int server, uint32_t *datatype, uint32_t *nchans, float *fsample, uint32_t *nsamples, uint32_t *nevents);
int fieldtrip_read_data(int server, uint32_t begsample, uint32_t endsample, byte *buffer);
int fieldtrip_write_header(int server, uint32_t datatype, uint32_t nchans, float fsample);
int fieldtrip_write_data(int server, uint32_t datatype, uint32_t nchans, uint32_t nsamples, byte *buffer);
int fieldtrip_wait_data(int server, uint32_t nsamples, uint32_t nevents, uint32_t milliseconds);

// define the version of the message packet
#define VERSION    (uint16_t)0x0001

// these define the commands that can be used, which are split over the two available bytes
#define PUT_HDR    (uint16_t)0x0101 /* decimal 257 */
#define PUT_DAT    (uint16_t)0x0102 /* decimal 258 */
#define PUT_EVT    (uint16_t)0x0103 /* decimal 259 */
#define PUT_OK     (uint16_t)0x0104 /* decimal 260 */
#define PUT_ERR    (uint16_t)0x0105 /* decimal 261 */

#define GET_HDR    (uint16_t)0x0201 /* decimal 513 */
#define GET_DAT    (uint16_t)0x0202 /* decimal 514 */
#define GET_EVT    (uint16_t)0x0203 /* decimal 515 */
#define GET_OK     (uint16_t)0x0204 /* decimal 516 */
#define GET_ERR    (uint16_t)0x0205 /* decimal 517 */

#define FLUSH_HDR  (uint16_t)0x0301 /* decimal 769 */
#define FLUSH_DAT  (uint16_t)0x0302 /* decimal 770 */
#define FLUSH_EVT  (uint16_t)0x0303 /* decimal 771 */
#define FLUSH_OK   (uint16_t)0x0304 /* decimal 772 */
#define FLUSH_ERR  (uint16_t)0x0305 /* decimal 773 */

#define WAIT_DAT   (uint16_t)0x0402 /* decimal 1026 */
#define WAIT_OK    (uint16_t)0x0404 /* decimal 1027 */
#define WAIT_ERR   (uint16_t)0x0405 /* decimal 1028 */

#define PUT_HDR_NORESPONSE (uint16_t)0x0501 /* decimal 1281 */
#define PUT_DAT_NORESPONSE (uint16_t)0x0502 /* decimal 1282 */
#define PUT_EVT_NORESPONSE (uint16_t)0x0503 /* decimal 1283 */

// these are used in the data_t and event_t structure
#define DATATYPE_CHAR    (uint32_t)0
#define DATATYPE_UINT8   (uint32_t)1
#define DATATYPE_UINT16  (uint32_t)2
#define DATATYPE_UINT32  (uint32_t)3
#define DATATYPE_UINT64  (uint32_t)4
#define DATATYPE_INT8    (uint32_t)5
#define DATATYPE_INT16   (uint32_t)6
#define DATATYPE_INT32   (uint32_t)7
#define DATATYPE_INT64   (uint32_t)8
#define DATATYPE_FLOAT32 (uint32_t)9
#define DATATYPE_FLOAT64 (uint32_t)10

uint32_t wordsize_from_type[] = { 1, 1, 2, 4, 8, 1, 2, 4, 8, 4, 8};

// a packet that is sent over the network should contain the following
typedef struct {
  uint16_t version;   // see VERSION
  uint16_t command;   // see PUT_xxx, GET_xxx and FLUSH_xxx
  uint32_t bufsize;   // size of the buffer in bytes
}
messagedef_t; // 8 bytes

// the header definition is fixed, it can be followed by additional header chunks
typedef struct {
  uint32_t  nchans;
  uint32_t  nsamples;
  uint32_t  nevents;
  float     fsample;
  uint32_t  data_type;
  uint32_t  bufsize;   // size of the buffer in bytes
}
headerdef_t; // 24 bytes

// the data definition is fixed, it should be followed by additional data
typedef struct {
  uint32_t nchans;
  uint32_t nsamples;
  uint32_t data_type;
  uint32_t bufsize;   // size of the buffer in bytes
}
datadef_t; // 16 bytes

// the event definition is fixed, it should be followed by additional data
typedef struct {
  uint32_t type_type;   /* usual would be DATATYPE_CHAR */
  uint32_t type_numel;  /* length of the type string */
  uint32_t value_type;
  uint32_t value_numel;
  int32_t  sample;
  int32_t  offset;
  int32_t  duration;
  uint32_t bufsize;     /* size of the buffer in bytes */
} eventdef_t; // 64 bytes

#endif _BUFFER_H_
