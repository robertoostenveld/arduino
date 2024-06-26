#include "fieldtrip_buffer.h"

WiFiClient client;

#undef DEBUG

/*******************************************************************************
   OPEN CONNECTION
   returns file descriptor that should be >0 on success
 *******************************************************************************/
int fieldtrip_open_connection(const char *address, int port) {
#ifdef DEBUG
  Serial.println("fieldtrip_open_connection");
#endif
  if (client.connect(address, port))
    return 1;
  else
    return -1;
};

/*******************************************************************************
   CLOSE CONNECTION
   returns 0 on success
 *******************************************************************************/
int fieldtrip_close_connection(int s) {
#ifdef DEBUG
  Serial.println("fieldtrip_close_connection");
#endif
  client.stop();
  return 0;
};

/*******************************************************************************
   WRITE HEADER
   returns 0 on success
 *******************************************************************************/
int fieldtrip_write_header(int server, uint32_t datatype, uint32_t nchans, float fsample) {
  int status;
  messagedef_t *request  = NULL;
  messagedef_t *response = NULL;
  headerdef_t  *header   = NULL;

#ifdef DEBUG
  Serial.println("fieldtrip_write_header");
#endif

  byte msg[8+24];
  for (int i = 0; i < 8+24; i++)
    msg[i] = 0;

  request = (messagedef_t *)(msg + 0);
  request->version = VERSION;
  request->command = PUT_HDR;
  request->bufsize = sizeof(headerdef_t);

  header = (headerdef_t *)(msg + sizeof(messagedef_t)); // the first 8 bytes are version, command and bufsize
  header->nchans     = nchans;
  header->nsamples   = 0;
  header->nevents    = 0;
  header->fsample    = fsample;
  header->data_type  = datatype;
  header->bufsize    = 0;

#ifdef DEBUG
  Serial.print("msg =  ");
  for (int i = 0; i < sizeof(messagedef_t) + sizeof(headerdef_t); i++) {
    Serial.print(msg[i]);
    Serial.print(" ");
  }
  Serial.println();
#endif

  int n = 0;
  n += client.write(msg, sizeof(messagedef_t) + sizeof(headerdef_t));
  client.flush();

#ifdef DEBUG
  Serial.print("Wrote ");
  Serial.print(n);
  Serial.println(" bytes");
#endif

  if (n != sizeof(messagedef_t) + sizeof(headerdef_t))
    status++;

  // read and ignore whatever response we get
  while (client.available())
    client.read();

  return status;
};

/*******************************************************************************
   WRITE DATA
   returns 0 on success
 *******************************************************************************/
int fieldtrip_write_data(int server, uint32_t datatype, uint32_t nchans, uint32_t nsamples, byte *buffer) {
  int status = 0;
  messagedef_t *request  = NULL;
  messagedef_t *response = NULL;
  datadef_t    *data   = NULL;

#ifdef DEBUG
  Serial.println("fieldtrip_write_data");
#endif

  byte msg[8+16];
  for (int i = 0; i < 8+16; i++)
    msg[i] = 0;

  request = (messagedef_t *)(msg + 0);
  request->version = VERSION;
  request->command = PUT_DAT;
  request->bufsize = sizeof(datadef_t) + nchans * nsamples * wordsize_from_type(datatype);

  data = (datadef_t *)(msg + sizeof(messagedef_t)); // the first 8 bytes are version, command and bufsize
  data->nchans     = nchans;
  data->nsamples   = nsamples;
  data->data_type  = datatype;
  data->bufsize    = nchans * nsamples * wordsize_from_type(datatype);

#ifdef DEBUG
  Serial.print("msg =  ");
  for (int i = 0; i < sizeof(messagedef_t) + sizeof(datadef_t); i++) {
    Serial.print(msg[i]);
    Serial.print(" ");
  }
  Serial.println();
#endif

  int n = 0;
  n += client.write(msg, sizeof(messagedef_t) + sizeof(datadef_t));
  client.flush();
  n += client.write(buffer, nchans * nsamples * wordsize_from_type(datatype));
  client.flush();

#ifdef DEBUG
  Serial.print("Wrote ");
  Serial.print(n);
  Serial.println(" bytes");
#endif

  if (n != sizeof(messagedef_t) + sizeof(datadef_t) + nchans * nsamples * wordsize_from_type(datatype))
    status++;

  // read and ignore whatever response we get
  while (client.available())
    client.read();

  return status;
};

int wordsize_from_type(uint32_t datatype) {
  int wordsize = 0;
  switch (datatype) {
    case DATATYPE_CHAR:
      wordsize = 1;
      break;
    case DATATYPE_UINT8:
      wordsize = 1;
      break;
    case DATATYPE_UINT16:
      wordsize = 2;
      break;
    case DATATYPE_UINT32:
      wordsize = 4;
      break;
    case DATATYPE_UINT64:
      wordsize = 8;
      break;
    case DATATYPE_INT8:
      wordsize = 1;
      break;
    case DATATYPE_INT16:
      wordsize = 2;
      break;
    case DATATYPE_INT32:
      wordsize = 4;
      break;
    case DATATYPE_INT64:
      wordsize = 8;
      break;
    case DATATYPE_FLOAT32:
      wordsize = 4;
      break;
    case DATATYPE_FLOAT64:
      wordsize = 8;
      break;
    }
  return wordsize;
};
