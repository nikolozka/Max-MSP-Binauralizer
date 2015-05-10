#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"

#define IN_SIZE 256
#define HRIR_SIZE 200
#define OUT_SIZE (IN_SIZE + HRIR_SIZE - 1)


void *convolver_class;

typedef struct _convolver
{
	t_pxobject l_obj;

	int az, el;	

	int h,t;

	t_double buff[IN_SIZE];

	t_double buff_h[2][IN_SIZE];

	t_double buff_out[2][IN_SIZE];	

	t_double buff_overlap[2][IN_SIZE];
	t_double buff_overlap_prev[2][IN_SIZE];


	t_double hrir_l[25][50][200];
	t_double hrir_r[25][50][200];

	short l_rcon;			
	short l_fcon;			
} t_convolver;

void convolver_dsp(t_convolver *x, t_signal **sp, short *count);
void convolver_dsp64(t_convolver *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void convolver_perform64(t_convolver *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
t_int *convolver_perform(t_int *w);
void convolver_int(t_convolver *x, long n);
void convolver_float(t_convolver *x, double f);
void convolver_clear(t_convolver *x);
void convolver_assist(t_convolver *x, void *b, long m, long a, char *s);
void *convolver_new(double freq, double reso);
void convolver_free(t_pxobject *x);

bool parser();
bool get_hrir(const char *path, double arr[25][50][200]);
bool get_IDT(const char *path, double arr[25][50]);

void convolve(t_convolver *x);
void hrir_update(t_convolver *x);
void overlap(t_convolver *x);
void clear_out(t_convolver *x);



int C74_EXPORT main(void)
{
	t_class *c;

	c = class_new("convolver~", (method)convolver_new, (method)convolver_free,
		sizeof(t_convolver), 0L, A_DEFFLOAT, A_DEFFLOAT, 0);
	class_addmethod(c, (method)convolver_dsp, "dsp", A_CANT, 0);
	class_addmethod(c, (method)convolver_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(c, (method)convolver_assist, "assist", A_CANT, 0);
	class_addmethod(c, (method)convolver_clear, "clear", 0);
	class_addmethod(c, (method)convolver_int, "int", A_LONG, 0);
	class_addmethod(c, (method)convolver_float, "float", A_FLOAT, 0);
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	convolver_class = c;

	return 0;
}

void convolver_free(t_pxobject *x){
	dsp_free(x);
}

void convolver_dsp(t_convolver *x, t_signal **sp, short *count)
{
	x->l_fcon = count[1];	// signal connected to the frequency inlet?
	x->l_rcon = count[2];	// signal connected to the resonance inlet?
	convolver_clear(x);

	dsp_add(convolver_perform, 6, sp[0]->s_vec, sp[3]->s_vec, x, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}


void convolver_dsp64(t_convolver *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
	x->l_fcon = count[1];	// signal connected to the frequency inlet?
	x->l_rcon = count[2];	// signal connected to the resonance inlet?
	convolver_clear(x);
	dsp_add64(dsp64, (t_object*)x, (t_perfroutine64)convolver_perform64, 0, NULL);
}


t_int *convolver_perform(t_int *w)
{
	// assign from parameters
    t_float *in = (t_float *)(w[1]);
    t_float *out = (t_float *)(w[2]);
    t_convolver *x = (t_convolver *)(w[3]);
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


void convolver_perform64(t_convolver *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
	t_double *in = ins[0];

	t_double *outL = outs[0];
	t_double *outR = outs[1];	

	int h, t;
	h = t = 0;

	int frames = sampleframes;
	
	while (sampleframes--){
		x->buff[h] = *in++;
		h++;
	}

	convolve(x);	
	overlap(x);

	while (frames--){

		*outL++ = x->buff_out[0][t];
		*outR++ = x->buff_out[1][t];
		//*outL++ = 0;
		//*outR++ = 0;
		t++;

	}

	
}


void convolve(t_convolver *x){

	for (int n = 0; n < OUT_SIZE; n++){

		t_double L_s, R_s;
				L_s = R_s = 0;

		size_t kmin, kmax, k;
		kmin = (n >= IN_SIZE - 1) ? n - (IN_SIZE - 1) : 0;
		kmax = (n < HRIR_SIZE - 1) ? n : HRIR_SIZE - 1;

		for (k = kmin; k <= kmax; k++){
			L_s += x->buff[n - k] * x->buff_h[0][k];
			R_s += x->buff[n - k] * x->buff_h[1][k];

		}
		
		if (n>=IN_SIZE){
			x->buff_overlap[0][n-IN_SIZE] = L_s*0.5;
			x->buff_overlap[1][n-IN_SIZE] = R_s*0.5;
		}
		else{
			x->buff_out[0][n] = L_s*0.5;
			x->buff_out[1][n] = R_s*0.5;
		}
	}
}

void overlap(t_convolver *x){
	for (int i = 0; i < OUT_SIZE - IN_SIZE; i++){
		x->buff_out[0][i] += x->buff_overlap_prev[0][i];
		x->buff_overlap_prev[0][i] = x->buff_overlap[0][i];
		x->buff_out[1][i] += x->buff_overlap_prev[1][i];
		x->buff_overlap_prev[1][i] = x->buff_overlap[1][i];
	}

}

void hrir_update(t_convolver *x){
	int padding = (IN_SIZE - HRIR_SIZE) / 2;

	for (int i = 0; i < HRIR_SIZE; i++){
			x->buff_h[0][i+padding] = x->hrir_l[x->az][x->el][i];
			x->buff_h[1][i+padding] = x->hrir_r[x->az][x->el][i];
	}
}

void convolver_int(t_convolver *x, long n)
{
	convolver_float(x,(double)n);
}

void convolver_float(t_convolver *x, double f)
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
	hrir_update(x);
}

void convolver_clear(t_convolver *x)
{
	
}


void convolver_assist(t_convolver *x, void *b, long m, long a, char *s)
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



void *convolver_new(double val, double reso)
{
    t_convolver *x = object_alloc(convolver_class);
    dsp_setup((t_pxobject *)x,3);

    outlet_new((t_object *)x, "signal");
	outlet_new((t_object *)x, "signal");

	const char *hrir_l_path = "C:/Home/Project/Binaural/hrir_l.csv";
	const char *hrir_r_path = "C:/Home/Project/Binaural/hrir_r.csv";

	get_hrir(hrir_l_path, x->hrir_l);	
	get_hrir(hrir_r_path, x->hrir_r);
		
	x->az = 13;
	x->el = 9;

	hrir_update(x);

	for (int i = 0; i < OUT_SIZE; i++){
		if (i< HRIR_SIZE){
			x->buff_h[0][i] = 0;
			x->buff_h[1][i] = 0;

		}
		if (i < IN_SIZE){
			x->buff[i] = 0;
		}
		x->buff_out[0][i] = 0;
		x->buff_out[0][i] = 0;
	}

    return (x);

	
}

bool get_hrir(const char *path, double arr[25][50][200]){

	char ch;
	char *ptr;
	char buff[20] = {'0'};
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
			arr[i][j][k] = strtod(buff, &ptr);
			index_buff = 0;		
			j++;
			if (j == 50) {
				j = 0;
				k++;
			}
			if (k == 200){
				k = 0;
				i++;
				i = i % 25;
				
			}
		}			
	}
	
	return true;
}



