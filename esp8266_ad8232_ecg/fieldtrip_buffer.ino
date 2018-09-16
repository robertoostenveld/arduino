#include "fieldtrip_buffer.h"

// prepare some of the requests, only the first 8 bytes are specified here
byte put_hdr[32] = {0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
byte put_dat[24] = {0x00, 0x01, 0x01, 0x02, 0x00, 0x00, 0x00, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
byte put_evt[8]  = {0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00};
byte get_hdr[8]  = {0x00, 0x01, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00};
byte get_dat[16] = {0x00, 0x01, 0x02, 0x02, 0x00, 0x00, 0x00, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
byte get_evt[16] = {0x00, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

byte buf[256];

messagedef_t request, response;
headerdef_t  header;
datadef_t    data;

/*********************************************************************************************************************/
int fieldtrip_open_connection(const char *address, int port) {
  int status = 0;
  status = client.connect(address, port);
  if (status == 1) {
    Serial.println("Cnnected to FieldTrip buffer");
    return 1;
  }
  else {
    Serial.print("Failed opening connection to ");
    Serial.print(port);
    Serial.print(" on ");
    Serial.println(address);
    Serial.print("client.connect returned ");
    Serial.println(status);
    return -1;
  }
};

int fieldtrip_close_connection(int s) {
  client.stop();
  return 0;
};

/*********************************************************************************************************************/
int fieldtrip_write_header(int server, uint32_t datatype, unsigned int nchans, float fsample) {
  int status = 0;
  headerdef_t *hdr;
  messagedef_t *response;

  hdr = (headerdef_t *)(put_hdr + 8); // the first 8 bytes are version, command and bufsize
  hdr->nchans     = htonl(nchans);
  hdr->nsamples   = htonl(0);
  hdr->nevents    = htonl(0);
  hdr->fsample    = fsample;
  hdr->data_type  = htonl(datatype);
  hdr->bufsize    = htonl(0);

  Serial.print("put_hdr =  ");
  for (int i = 0; i < sizeof(put_hdr); i++) {
    Serial.print(put_hdr[i]);
    Serial.print(" ");
  }
  Serial.println();

  int n = 0;
  n += client.write(put_hdr, sizeof(put_hdr)) ;
  Serial.print("Wrote ");
  Serial.print(n);
  Serial.println(" bytes");

  // wait for the response to become available
  int timeout = 0;
  while ((client.available() < 8) && (++timeout) < 200)
    delay(10);
  if (timeout == 200)
    Serial.println("Timeout");

  // get the general messagedef_t section
  byte buf[8];
  for (int i = 0; i < 8; i++) {
    buf[i] = client.read();
  }

  Serial.print("response =  ");
  for (int i = 0; i < sizeof(buf); i++) {
    Serial.print(buf[i]);
    Serial.print(" ");
  }
  Serial.println();

  response = (messagedef_t *)buf;
  uint16_t version = ntohs(response->version);
  uint16_t command = ntohs(response->command);
  uint32_t bufsize = ntohl(response->bufsize);

  Serial.print("server returned ");
  Serial.print(version);
  Serial.print(" ");
  Serial.print(command);
  Serial.print(" ");
  Serial.println(bufsize);

  if ((n == sizeof(put_hdr)) && (command == PUT_OK))
    status = 0;
  else
    status = -1;

  return status;
};

/*********************************************************************************************************************/
int fieldtrip_write_data(int server, uint32_t datatype, uint32_t nchans, uint32_t nsamples, byte *buffer) {
  int status = 0;
  datadef_t *dat;
  messagedef_t *request, *response;

  request = (messagedef_t *)(put_dat + 0);
  request->bufsize = htonl(sizeof(datadef_t) + nchans * nsamples * wordsize_from_type[datatype]);

  dat = (datadef_t *)(put_dat + 8); // the first 8 bytes are version, command and bufsize
  dat->nchans     = htonl(nchans);
  dat->nsamples   = htonl(nsamples);
  dat->data_type  = htonl(datatype);
  dat->bufsize    = htonl(nchans * nsamples * wordsize_from_type[datatype]);

  Serial.print("put_dat =  ");
  for (int i = 0; i < sizeof(put_dat); i++) {
    Serial.print(put_dat[i]);
    Serial.print(" ");
  }
  Serial.println();

  int n = 0;
  n += client.write(put_dat, sizeof(put_dat));
  n += client.write(buffer, nchans * nsamples * wordsize_from_type[datatype]);

  Serial.print("Wrote ");
  Serial.print(n);
  Serial.println(" bytes");

  // wait for the response to become available
  int timeout = 0;
  while ((client.available() < 8) && (++timeout) < 200)
    delay(10);
  if (timeout == 200)
    Serial.println("Timeout");

  // get the general messagedef_t section
  byte buf[8];
  for (int i = 0; i < 8; i++) {
    buf[i] = client.read();
  }

  Serial.print("response =  ");
  for (int i = 0; i < sizeof(buf); i++) {
    Serial.print(buf[i]);
    Serial.print(" ");
  }
  Serial.println();

  response = (messagedef_t *)buf;
  uint16_t version = ntohs(response->version);
  uint16_t command = ntohs(response->command);
  uint32_t bufsize = ntohl(response->bufsize);

  Serial.print("server returned ");
  Serial.print(version);
  Serial.print(" ");
  Serial.print(command);
  Serial.print(" ");
  Serial.println(bufsize);

  if ((n == (sizeof(put_dat) + nchans * nsamples * wordsize_from_type[datatype])) && (command == PUT_OK))
    status = 0;
  else
    status = -1;

  return status;
}
