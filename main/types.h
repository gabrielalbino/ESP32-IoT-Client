#ifndef types_h
#define types_h

typedef struct {
  int type;
  char name[50];
  int status;
} DeviceIO;

typedef struct {
  char id[50];
  char name[50];
  float temperature;
  float humidity;
  DeviceIO input;
  DeviceIO output;
} Device;

#endif