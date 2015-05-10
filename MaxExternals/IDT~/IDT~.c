#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"

#define BUFF_SIZE 1024
#define FS 44100


void *IDT_class;

typedef struct _IDT
{
	t_pxobject l_obj;

	int framesize;

	int az, el;	

	int h,tl,tr;

	t_double buff_out[2][BUFF_SIZE];	

	t_double IDT[25][50];

	short l_rcon;			
	short l_fcon;			
} t_IDT;

void IDT_dsp(t_IDT *x, t_signal **sp, short *count);
void IDT_dsp64(t_IDT *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void IDT_perform64(t_IDT *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
t_int *IDT_perform(t_int *w);
void IDT_int(t_IDT *x, long n);
void IDT_float(t_IDT *x, double f);
void IDT_clear(t_IDT *x);
void IDT_assist(t_IDT *x, void *b, long m, long a, char *s);
void *IDT_new(double freq, double reso);
void IDT_free(t_pxobject *x);

bool parser();
bool get_IDT(const char *path, double arr[25][50]);

void IDT_update(t_IDT *x);



int C74_EXPORT main(void)
{
	t_class *c;

	c = class_new("IDT~", (method)IDT_new, (method)IDT_free,
		sizeof(t_IDT), 0L, A_DEFFLOAT, A_DEFFLOAT, 0);
	class_addmethod(c, (method)IDT_dsp, "dsp", A_CANT, 0);
	class_addmethod(c, (method)IDT_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c, (method)IDT_assist, "assist", A_CANT, 0);
	class_addmethod(c, (method)IDT_clear, "clear", 0);
	class_addmethod(c, (method)IDT_int, "int", A_LONG, 0);
	class_addmethod(c, (method)IDT_float, "float", A_FLOAT, 0);
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	IDT_class = c;

	return 0;
}

void IDT_free(t_pxobject *x){
	dsp_free(x);
}

void IDT_dsp(t_IDT *x, t_signal **sp, short *count)
{
	x->l_fcon = count[1];	// signal connected to the frequency inlet?
	x->l_rcon = count[2];	// signal connected to the resonance inlet?
	IDT_clear(x);

	dsp_add(IDT_perform, 6, sp[0]->s_vec, sp[3]->s_vec, x, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}


void IDT_dsp64(t_IDT *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
	x->l_fcon = count[1];	// signal connected to the frequency inlet?
	x->l_rcon = count[2];	// signal connected to the resonance inlet?
	IDT_clear(x);
	dsp_add64(dsp64, (t_object*)x, (t_perfroutine64)IDT_perform64, 0, NULL);
}


t_int *IDT_perform(t_int *w)
{
	// assign from parameters
    t_float *in = (t_float *)(w[1]);
    t_float *out = (t_float *)(w[2]);
    t_IDT *x = (t_IDT *)(w[3]);
    t_float az = x->l_fcon? *(t_float *)(w[4]) : x->az;
    t_float el = x->l_rcon? *(t_float *)(w[5]) : x->el;
    int n = (int)(w[6]);
	t_float val;
    
    
    if (x->l_obj.z_disabled)
    	goto out;
    	
    
    // DSP loop
    
    while (n--) {
    	val = *in++;
    	
    	*out++ = val;

		////
		////DSP Goes Here
		////

    }
    
out:
    return (w+7);
}


void IDT_perform64(t_IDT *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
	t_double *inL = ins[0];
	t_double *inR = ins[1];

	t_double *outL = outs[0];
	t_double *outR = outs[1];	

	
	while (sampleframes--){
		x->buff_out[0][x->h] = *inL++;
		x->buff_out[1][x->h] = *inR++;

		*outL++ = x->buff_out[0][x->tl];
		*outR++ = x->buff_out[1][x->tr];

		x->h++;
		x->h = x->h%BUFF_SIZE;

		if (x->az < 13){
			x->tl = x->h + delay_samples(x) % BUFF_SIZE;
			x->tr = x->h;
		}	
		else{
			x->tr = x->h + delay_samples(x) % BUFF_SIZE;
			x->tl = x->h;
		}
	}
}


void IDT_int(t_IDT *x, long n)
{
	IDT_float(x,(double)n);
}

void IDT_float(t_IDT *x, double f)
{
	long in = proxy_getinlet((t_object *)x);
	
	if (in == 1) {
		if (f < 2)
			x->az = 2;
		else if (f > 22)
			x->az = 22;
		else
			x->az = floor(f);
	} 

	else if (in == 2) {
		if (f < 0)
			x->el = 0;
		else if (f > 49)
			x->el = 49;
		else
			x->el = floor(f);

	}
}

void IDT_clear(t_IDT *x)
{
	
}

int delay_samples(t_IDT *x){
	float out = x->IDT[x->az][x->el] / 1000 / (1 / FS);
	return floor(out);
}


void IDT_assist(t_IDT *x, void *b, long m, long a, char *s)
{
	if (m == 2){
		switch (a) {
		case 0: sprintf(s, "(signal) Output L"); break;
		case 1: sprintf(s, "(signal) Output R"); break;

		}
	}
	else {
		switch (a) {	
		case 0: sprintf(s,"(signal) Input"); break;
		case 1: sprintf(s,"(int) Azimuth"); break;
		case 2: sprintf(s,"(int) Elevation"); break;
		}
	}
}



void *IDT_new(double val, double reso)
{
    t_IDT *x = object_alloc(IDT_class);

    dsp_setup((t_pxobject *)x,4);

    outlet_new((t_object *)x, "signal");
	outlet_new((t_object *)x, "signal");

	const char *IDT_path = "C:/Home/Project/Binaural/IDT.csv";

	get_IDT(IDT_path, x->IDT);
		
	x->az = 13;
	x->el = 9;

	x->h = 0;
	x->tr = 0;
	x->tl = 0;


	for (int i = 0; i < BUFF_SIZE; i++){
		
		x->buff_out[0][i] = 0;
		x->buff_out[0][i] = 0;
	}

    return (x);

	
}


bool get_IDT(const char *path, double arr[25][50]){

	char ch;
	char *ptr;
	char buff[20] = { '0' };
	int index_buff, i, j, k;
	index_buff = 0;
	i = 0;
	j = 0;
	k = 0;

	FILE *fp = NULL;
	fp = fopen(path, "r");

	if (fp == NULL){
		cpost(path, ": ");
		cpost("Error: %d (%s)\n", errno, strerror(errno));
		return false;
	}

	while ((ch = fgetc(fp)) != EOF){
		if (ch != ','){
			buff[index_buff] = ch;
			index_buff++;
		}
		else{
			ptr = &buff[index_buff];
			arr[k][j] = strtod(buff, &ptr);
			index_buff = 0;
			j++;
			if (j == 50){
				j = 0;
				k++;
				k = k % 25;

			}
		}
	}	
	return true;
}




