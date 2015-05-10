
#define ARDUINO_WAIT_TIME 25
#define DATA_LENGTH 64
#define ARRAY_SIZE 6
#define TIMEOUT 40
#define ELMAX 50
#define M_PI 3.14159265358979323846

#include "ext.h"
#include "ext_obex.h"

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

HANDLE hSerial;
DCB dcbSerialParams;
bool connected;
COMSTAT status; //Get various information about the connection
DWORD errors; //Keep track of last error
bool isConnected = false;

double values[ARRAY_SIZE] = { 0 };
double offset[ARRAY_SIZE] = { 0 };
double offset_values[ARRAY_SIZE] = { 0 };

typedef struct arduino_accel_gyro{

	t_object d_obj;

	void *m_outlet1;
	void *m_outlet2;
	void *m_outlet3;

	long m_in;

	void *m_proxy;

	double az, el;

} t_arduino_accel_gyro;

t_arduino_accel_gyro *x;

void *arduino_accel_gyro_new(t_symbol *s, long argc, t_atom *argv);
void arduino_accel_gyro_free(t_arduino_accel_gyro *x);
void arduino_accel_gyro_bang(t_arduino_accel_gyro *x);
int Serial_setup(int baudRate, int waitTime);
int Serial_ReadData(char *buffer, unsigned int nbChar);
void Serial_flush();
int serial_close();
void postToMax(t_arduino_accel_gyro *x);
void getFloats(char *message, double *values);
int request();
void arduino_accel_gyro_delay(t_arduino_accel_gyro *x);
void getIndices();
double pvaldeg(double angle);

static t_class *s_arduino_accel_gyro_class = NULL;

int Serial_setup(int baudRate, int waitTime){
	hSerial = CreateFile(
		"\\\\.\\COM3", GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hSerial == INVALID_HANDLE_VALUE){
		post("Error: Couldn't open serial");
		return 0;
	}

	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if (GetCommState(hSerial, &dcbSerialParams) == 0){
		post("Error getting device state");
		CloseHandle(hSerial);
		return 0;
	}

	dcbSerialParams.BaudRate = CBR_115200;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;

	if (SetCommState(hSerial, &dcbSerialParams) == 0){
		post("Error setting device parameters");
		CloseHandle(hSerial);
		return 0;
	}

	post("OK");
	isConnected = true;
	return 1;
}

int Serial_ReadData(char *buffer, unsigned int nbChar){
	DWORD bytesRead; //#bytes to read
	unsigned int toRead;//real #bytes to read
	ClearCommError(hSerial, &errors, &status);//get status of serial port	
	if (status.cbInQue>0)//check queue for data
	{
		if (status.cbInQue>nbChar){ toRead = nbChar; }
		else{ toRead = status.cbInQue; }
		if (ReadFile(hSerial, buffer, toRead, &bytesRead, NULL) && bytesRead != 0){ return bytesRead; } //retrun read bytes on success
	}
	return -1;
}

void Serial_flush(){
	PurgeComm(hSerial, PURGE_RXCLEAR); 
	PurgeComm(hSerial, PURGE_TXCLEAR);
}

void postToMax(t_arduino_accel_gyro *x){
	outlet_float(x->m_outlet1, x->az);
	outlet_float(x->m_outlet2, offset_values[1]);
	outlet_float(x->m_outlet3, x->el);
}

void getFloats(char *message, double *values){

	int index = 0;

	const char s[2] = ",";
	char *token;

	token = strtok(message, s);

	while (token != NULL)
	{
		values[index] = atof(token);
		index++;
		token = strtok(NULL, s);

	}
}

int C74_EXPORT main(void)
{
	t_class *c = class_new(	"arduino_accel_gyro", (method)arduino_accel_gyro_new, (method)arduino_accel_gyro_free, sizeof(t_arduino_accel_gyro), (method)0L, A_GIMME, 0);
	class_addmethod(c, (method)arduino_accel_gyro_bang, "bang", A_LONG, 0);

	class_register(CLASS_BOX, c);
	s_arduino_accel_gyro_class = c;
	return 0;
}


void *arduino_accel_gyro_new(t_symbol *s, long argc, t_atom *argv)
{
	x = (t_arduino_accel_gyro *)object_alloc(s_arduino_accel_gyro_class);

	long attrstart = (long)attr_args_offset(argc, argv);
	attr_args_process(x, argc, argv);

	x->m_proxy = proxy_new((t_object *)x, 1, &x->m_in);

	x->m_outlet3 = floatout(x);
	x->m_outlet2 = floatout(x);
	x->m_outlet1 = floatout(x);

	offset_values[0] = values[0] = offset[0] = 0;
	offset_values[1] = values[1] = offset[1] = 0;
	offset_values[2] = values[2] = offset[2] = 0;

	x->az = 0;
	x->el = 0;	

	Serial_setup(CBR_115200, TIMEOUT);

	return x;
}

void arduino_accel_gyro_bang(t_arduino_accel_gyro *x)
{
	switch (proxy_getinlet((t_object *)x)) {
	case 0:
		if (!isConnected){
			Serial_setup(CBR_115200, TIMEOUT);
		}
		else if (proxy_getinlet((t_object *)x) == 0){
			Serial_flush();
			if (request()){
				Sleep(ARDUINO_WAIT_TIME);
				char incomingData[DATA_LENGTH] = "";
				int readResult = 0;
				readResult = Serial_ReadData(incomingData, DATA_LENGTH);
				getFloats(incomingData, values);
				getIndices();
				postToMax(x);
			}
		}
		break;
	case 1:
		offset[0] = values[0];
		offset[1] = values[1];
		offset[2] = values[2];
		break;
	}
}

int request(){

	DWORD dword = 1;
	char a = 'a';

	int bytes;
	if (!WriteFile(hSerial, &a, 1, &bytes, NULL))
	{
		post("Error: Couldn't write to serial");
		serial_close();
		return 0;
	}
	return 1;
	
}
	
void arduino_accel_gyro_free(t_arduino_accel_gyro *x)
{
	serial_close();
}

int serial_close(){
	post("Closing serial port...");
	if (CloseHandle(hSerial) == 0)
	{
		post("Error\n");
		return 1;
	}
	post("OK\n");
	isConnected = false;
	return 0;
}

void getIndices(){
	offset_values[0] = values[0] - offset[0];
	offset_values[1] = values[1] - offset[1];
	offset_values[2] = values[2] - offset[2];

	x->az = pvaldeg(offset_values[0]);
	x->el = pvaldeg(offset_values[2]);

	if (x->az > 80) x->az = 80;
	if (x->az < -80) x->az = -80;

	int elv;

	int azimuths[] = {-80, -65, -55, -45, -40, -35, -30, -25, -20, -15, -10 -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 55, 65, 80};
	int azm=0;
	int min = abs(azimuths[0]-x->az);
	int a = 0;

	for (int i = 1; i < 25; i++){
		a = abs(azimuths[i] - x->az);
		if (a < min){
			min = a;
			azm = i;
		}
	}

	double val = (x->el + 45) / 5.625 + 1;

	elv = (int)val;

	elv = max(elv, 1);

	elv = min(elv, ELMAX);

	x->el = elv;
	x->az = azm;
	

	
}

double pvaldeg(double angle){

	double dtr =  M_PI / 180;	
	angle = atan2(sin(angle * dtr), cos(angle * dtr)) / dtr;
	if (angle < -90) angle = angle + 360;
	return angle;

}










