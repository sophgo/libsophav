//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
// 
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
// 
// The entire notice above must be reproduced on all authorized copies.
// 
// Description  : 
//-----------------------------------------------------------------------------

#include <stdint.h>
#include "util_float.h"

#ifndef OPT_FLOAT
#include <math.h>
FLOAT_T FINIT(int32_t sig, int32_t exp) {return sig * pow(2.0, exp);}
FLOAT_T FADD(FLOAT_T x, FLOAT_T y) {return x + y;}
FLOAT_T FSUB(FLOAT_T x, FLOAT_T y) {return x - y;}
FLOAT_T FMUL(FLOAT_T x, FLOAT_T y) {return x * y;}
FLOAT_T FDIV(FLOAT_T x, FLOAT_T y) {return x / y;}
FLOAT_T FLOG2(FLOAT_T x) {return log(x) / log(2.0);}
FLOAT_T FPOW2(FLOAT_T x) {return pow(2.0, x);}
int32_t FINT(FLOAT_T x) {return (int32_t)(floor(x + 0.5));}
FLOAT_T FMIN(FLOAT_T x, FLOAT_T y) {return (x<y) ? x : y;}
FLOAT_T FMAX(FLOAT_T x, FLOAT_T y) {return (x>y) ? x : y;}
FLOAT_T FMINMAX(FLOAT_T min, FLOAT_T max, FLOAT_T x) {return FMAX(min, FMIN(max, x));}
#else //OPT_FLOAT
static int32_t int32_min(int32_t x, int32_t y)
{
    return (x<y) ? x : y;
}

// static int32_t int32_max(int32_t x, int32_t y)
// {
//     return (x>y) ? x : y;
// }

static int32_t int32_min_max(int32_t min, int32_t max, int32_t x)
{
    return (x<min) ? min : (x>max) ? max : x;
}

static uint32_t count_sign_bit(int32_t x)
{
    uint32_t n = 0;

    if (x < 0)
        x = ~x;

    if (x==0)
        return 32;

    if ((x>>16)==0) {n+=16; x<<=16;}
    if ((x>>24)==0) {n+= 8; x<<= 8;}
    if ((x>>28)==0) {n+= 4; x<<= 4;}
    if ((x>>30)==0) {n+= 2; x<<= 2;}
    if ((x>>31)==0) {n+= 1;}

    return n;
}

static FLOAT_T normalize(int32_t sig, int32_t exp)
{
    FLOAT_T f;
    uint32_t shift = count_sign_bit(sig) - 1; //count sign bit except the last one

    sig <<= shift;  //left align
    exp -= shift;
    sig >>= 16;     //make sig 16bit
    exp += 16;

    //make exponent 16bit
    exp = int32_min_max(INT16_MIN, INT16_MAX, exp);
    if (sig == 0)
        exp = INT16_MIN;

    f.sig = (int16_t)sig;
    f.exp = (int16_t)exp;

    return f;
}

FLOAT_T FINIT(int32_t sig, int32_t exp)
{
    return normalize(sig, exp);
}

//max error 0.006%
FLOAT_T FADD(FLOAT_T x, FLOAT_T y)
{
	int32_t sig1 = x.sig << 15;
	int32_t sig2 = y.sig << 15;
	int32_t exp;
	int32_t shift = x.exp - y.exp;

	//align exponents
	if (shift >= 0)
	{
		exp = x.exp - 15;
		sig2 >>= int32_min(shift, 31);
	}
	else
	{
		exp = y.exp - 15;
		sig1 >>= int32_min(-shift, 31);
	}

	return normalize(sig1 + sig2, exp);
}

//max error 0.006%
FLOAT_T FSUB(FLOAT_T x, FLOAT_T y)
{
	int32_t sig1 = x.sig << 15;
	int32_t sig2 = y.sig << 15;
	int32_t exp;
	int32_t shift = x.exp - y.exp;

	//align exponents
	if (shift >= 0)
	{
		exp = x.exp - 15;
		sig2 >>= int32_min(shift, 31);
	}
	else
	{
		exp = y.exp - 15;
		sig1 >>= int32_min(-shift, 31);
	}

	return normalize(sig1 - sig2, exp);
}

//max error 0.006%
FLOAT_T FMUL(FLOAT_T x, FLOAT_T y)
{
    int32_t sig = x.sig * y.sig;
    int32_t exp = x.exp + y.exp;
    return normalize(sig, exp);
}

//max error 0.006%
FLOAT_T FDIV(FLOAT_T x, FLOAT_T y)
{
    int32_t sig;
    int32_t exp;

    if (y.sig == 0)
    {
        sig = x.sig;
        exp = INT16_MAX;
    }
    else
    {
        sig = (x.sig << 16) / y.sig;
        exp = x.exp - y.exp - 16;
    }
    return normalize(sig, exp);
}

// C. S. Turner,  "A Fast Binary Logarithm Algorithm", 
// IEEE Signal Processing Mag., pp. 124,140, Sep. 2010.
//max error 0.6%
FLOAT_T FLOG2(FLOAT_T x)
{
    const int precision = 24; //number of fractional bit

    if (x.sig <= 0)
    {
        return normalize(-1, INT16_MAX); //-infinitive
    }
    else if (x.exp+14 > (INT32_MAX>>precision) || x.exp+14 < (INT32_MIN>>precision))
    {   //log2(a*2^n)=log2(a)+n ~= n when abs(n) is much larger than log2(a)
        return normalize(x.exp, 0); 
    }
    else 
    {
        int i;
        int32_t y = (x.exp + 14) << precision;
        int32_t b = 1 << (precision-1);
        uint32_t sig = (uint32_t)x.sig << 1;  //Q1.15

        for (i=0; i<precision; i++)
        {
            sig = (sig * sig) >> 15;
            if (sig >= (2u<<15))
            {
                sig >>= 1;
                y += b;
            }
            b >>= 1;
        }
        return normalize(y, -precision);
    }
}

//max error 0.02%
FLOAT_T FPOW2(FLOAT_T x)
{
    //Maclaurin series of 2^x ~= sum[((ln2)^n)/(n!)*(x^n)] where n=0~4
    //coeff = (ln2)^n)/(n!)
    const FLOAT_T coeff[5] = {
        {22713, -15},
        {31487, -17},
        {29100, -19},
        {20171, -21},
        {22370, -24}
    };

    //x = sig*2^exp = int_x + frac_x
    //2^x = 2^frac_x * 2^int_x
    if (x.exp >= 0) //frac_x is 0
    {
        return normalize(1, x.sig << int32_min(x.exp, 16));
    }
    else
    {
        int i;
        FLOAT_T int_x;
        FLOAT_T frac_x;
        FLOAT_T frac_xn;   //n power of frac_x
        FLOAT_T sum = normalize(1, 0);

        int_x = normalize(x.sig >> int32_min(-x.exp, 16), 0);
        frac_x = FSUB(x, int_x);
        frac_xn = frac_x;

        for (i=0; i<5; i++) 
        {
            FLOAT_T tmp = FMUL(coeff[i], frac_xn);
            sum = FADD(sum, tmp);
            frac_xn = FMUL(frac_xn, frac_x);
        }
        return normalize(sum.sig, sum.exp + (int_x.sig>>-int_x.exp));
    }
}

//max error 0.5
int32_t FINT(FLOAT_T x)
{
    int32_t val;

    if (x.exp >= 0)
    {
        val = x.sig << int32_min(x.exp, 16);
    }
    else if (x.exp > -32)
    {
        int32_t shift = -x.exp; // shift=1~31
        int32_t round = 1 << (shift - 1);
        val = (x.sig + round) >> shift;
    }
    else //x.exp <= -32
    {
        val = 0;
    }

    return val;
}

FLOAT_T FMIN(FLOAT_T x, FLOAT_T y)
{
    FLOAT_T tmp = FSUB(x, y);
    if (tmp.sig < 0)
        return x;
    else
        return y;
}

FLOAT_T FMAX(FLOAT_T x, FLOAT_T y)
{
    FLOAT_T tmp = FSUB(x, y);
    if (tmp.sig > 0)
        return x;
    else
        return y;
}

FLOAT_T FMINMAX(FLOAT_T min, FLOAT_T max, FLOAT_T x)
{
    return FMIN(max, FMAX(min, x));
}
#endif //OPT_FLOAT

#if 0
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static double rand_double(void) //uniform in (0,1), period = 2^31-2
{
    static double seed = 1.0;
    const double a = 16807.0;	// 7^5		
    const double m = 2147483647.0;	// 2^31-1

    seed = fmod(a * seed, m); 
    return (seed / m); 
}
static int rand_int(int min, int max)
{
    assert(min<=max);
    max = (int)((max - min + 1.0) * rand_double());
    return min + max;
}
static FLOAT_T float_rand(int min_sig, int max_sig, int min_exp, int max_exp, double *val)
{
    int sig = rand_int(min_sig, max_sig);
    int exp = rand_int(min_exp, max_exp);
    *val =  (double)sig * pow(2.0, exp);
    return FINIT(sig, exp);
}
static void print_err_ratio(FLOAT_T x, double ideal, double *max_err)
{
#ifdef OPT_FLOAT
    double my = (double)x.sig * pow(2.0, x.exp);
#else
    double my = x;
#endif
    double err = fabs(my/ideal - 1.0) * 100;
    if (err > *max_err)
    {
        *max_err = err;
        printf("ideal=%+e my=%+e err=%f%%\n", ideal, my, err);
    }
}
static void print_err_diff(double my, double ideal, double *max_err)
{
    double err = fabs(my - ideal);
    if (err > *max_err)
    {
        *max_err = err;
        printf("ideal=%+e my=%+e err=%f\n", ideal, my, err);
    }
}
int main(int argc, char **argv)
{
    int cnt;
    int CNT = 10000000;
    FLOAT_T f[3], my;
    double d[3], ideal, max_err;

    printf("------- add  -------\n");
    for (max_err=0,cnt=0; cnt<CNT; cnt++)
    {
        f[0] = float_rand(INT16_MIN, INT16_MAX, -64, 64, &d[0]);
        f[1] = float_rand(INT16_MIN, INT16_MAX, -64, 64, &d[1]);
        my = FADD(f[0], f[1]);
        ideal = d[0] + d[1];
        print_err_ratio(my, ideal, &max_err);
    }
    printf("------- sub  -------\n");
    for (max_err=0,cnt=0; cnt<CNT; cnt++)
    {
        f[0] = float_rand(INT16_MIN, INT16_MAX, -64, 64, &d[0]);
        f[1] = float_rand(INT16_MIN, INT16_MAX, -64, 64, &d[1]);
        my = FSUB(f[0], f[1]);
        ideal = d[0] - d[1];
        print_err_ratio(my, ideal, &max_err);
    }
    printf("------- mul  -------\n");
    for (max_err=0,cnt=0; cnt<CNT; cnt++)
    {
        f[0] = float_rand(INT16_MIN, INT16_MAX, -64, 64, &d[0]);
        f[1] = float_rand(INT16_MIN, INT16_MAX, -64, 64, &d[1]);
        my = FMUL(f[0], f[1]);
        ideal = d[0] * d[1];
        print_err_ratio(my, ideal, &max_err);
    }
    printf("------- div  -------\n");
    for (max_err=0,cnt=0; cnt<CNT; cnt++)
    {
        f[0] = float_rand(INT16_MIN, INT16_MAX, -64, 64, &d[0]);
        f[1] = float_rand(INT16_MIN, INT16_MAX, -64, 64, &d[1]);
        my = FDIV(f[0], f[1]);
        ideal = d[0] / d[1];
        print_err_ratio(my, ideal, &max_err);
    }
    printf("------- log2  -------\n");
    for (max_err=0,cnt=0; cnt<CNT; cnt++)
    {
        f[0] = float_rand(0, INT16_MAX, -64, 64, &d[0]);
        my = FLOG2(f[0]);
        ideal = log(d[0])/log(2);
        print_err_ratio(my, ideal, &max_err);
    }
    printf("------- pow2  -------\n");
    for (max_err=0,cnt=0; cnt<CNT; cnt++)
    {
        f[0] = float_rand(INT16_MIN, INT16_MAX, -64, -9, &d[0]); //max(f[0])=64
        my = FPOW2(f[0]);
        ideal = pow(2, d[0]);
        print_err_ratio(my, ideal, &max_err);
    }
    printf("------- int  -------\n");
    for (max_err=0,cnt=0; cnt<CNT; cnt++)
    {
        f[0] = float_rand(INT16_MIN, INT16_MAX, -64, 16, &d[0]);
        d[1] = FINT(f[0]);
        print_err_diff(d[1], d[0], &max_err);
    }
    printf("------- min,max  -------\n");
    for (max_err=0,cnt=0; cnt<CNT; cnt++)
    {
        f[0] = float_rand(INT16_MIN, INT16_MAX, -64, 64, &d[0]);
        f[1] = float_rand(INT16_MIN, INT16_MAX, -64, 64, &d[1]);
        my = FMIN(f[0], f[1]);
        ideal = (d[0] < d[1]) ? d[0] : d[1];
        print_err_ratio(my, ideal, &max_err);
        my = FMAX(f[0], f[1]);
        ideal = (d[0] > d[1]) ? d[0] : d[1];
        print_err_ratio(my, ideal, &max_err);
    }
    return 0;
}
#endif
 
