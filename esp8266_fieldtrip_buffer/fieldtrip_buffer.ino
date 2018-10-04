#include "fieldtrip_buffer.h"

#define DEBUG

/*******************************************************************************
   OPEN CONNECTION
   returns file descriptor that should be >0 on success
 *******************************************************************************/
int fieldtrip_open_connection(const char *address, int port) {
  int status;
  status = client.connect(address, port);
  if (status == 1)
    return 1;
  else
    return -1;
};

/*******************************************************************************
   CLOSE CONNECTION
   returns 0 on success
 *******************************************************************************/
int fieldtrip_close_connection(int s) {
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

  byte msg[32];
  for (int i = 0; i < 32; i++)
    msg[i] = 0;

  request = (messagedef_t *)(msg + 0);
  request->version = VERSION;
  request->command = PUT_HDR_NORESPONSE;
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

  if (n == sizeof(messagedef_t) + sizeof(headerdef_t))
    return 0;
  else
    return -1;
};

/*******************************************************************************
   WRITE DATA
   returns 0 on success
 *******************************************************************************/
int fieldtrip_write_data(int server, uint32_t datatype, uint32_t nchans, uint32_t nsamples, byte *buffer) {
  int status;
  messagedef_t *request  = NULL;
  messagedef_t *response = NULL;
  datadef_t    *data   = NULL;

  byte msg[24];
  for (int i = 0; i < 32; i++)
    msg[i] = 0;

  request = (messagedef_t *)(msg + 0);
  request->version = VERSION;
  request->command = PUT_DAT_NORESPONSE;
  request->bufsize = sizeof(datadef_t) + nchans * nsamples * wordsize_from_type[datatype];

  data = (datadef_t *)(msg + sizeof(messagedef_t)); // the first 8 bytes are version, command and bufsize
  data->nchans     = nchans;
  data->nsamples   = nsamples;
  data->data_type  = datatype;
  data->bufsize    = nchans * nsamples * wordsize_from_type[datatype];

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
  n += client.write(buffer, nchans * nsamples * wordsize_from_type[datatype]);
  client.flush();

#ifdef DEBUG
  Serial.print("Wrote ");
  Serial.print(n);
  Serial.println(" bytes");
#endif

  if (n == sizeof(messagedef_t) + sizeof(datadef_t) + nchans * nsamples * wordsize_from_type[datatype])
    return 0;
  else
    return -1;
};
