#include <bmcv_cmodel.h>

unsigned char saturate_cast_uchar(int a){
	return (unsigned char)((unsigned)a <= UCHAR_MAX ? a : a > 0 ? UCHAR_MAX : 0);
}

int alignSize(int a, int b){
	return (a + (1 << b) - 1) >> b << b;
}

int cvFloor( double value )
{
    int i = (int)value;
    return i - (i > value);
}

int cvround( double value )
{
#if ((defined _MSC_VER && defined _M_X64) || (defined __GNUC__ && defined __x86_64__ \
    && defined __SSE2__ && !defined __APPLE__) || CV_SSE2) && !defined(__CUDACC__)
    __m128d t = _mm_set_sd( value );
    return _mm_cvtsd_si32(t);
#elif defined _MSC_VER && defined _M_IX86
    int t;
    __asm
    {
        fld value;
        fistp t;
    }
    return t;
#elif defined CV_ICC || defined __GNUC__
# if defined ARM_ROUND_DBL
    ARM_ROUND_DBL(value);
# else
    return (int)lrint(value);
# endif
#else
    /* it's ok if round does not comply with IEEE754 standard;
       the tests should allow +/-1 difference when the tested functions use round */
    return (int)(value + (value >= 0 ? 0.5 : -0.5));
#endif
}

int cvmin(int a, int b){
  return a > b ? b : a;
}

int cvmax(int a, int b){
  return a < b ? b : a;
}

//template<typename unsigned char, typename int, typename short>;
int clip(int x, int a, int b)
{
	return x >= a ? (x < b ? x : b - 1) : a;
}

void hresize_cubic(const unsigned char** src, int** dst, int count,
                    const int* xofs, const short* alpha,
                    int swidth, int dwidth, int cn, int xmin, int xmax)
{
	for( int k = 0; k < count; k++ )
	{
		const unsigned char *S = src[k];
		int *D = dst[k];
		int dx = 0, limit = xmin;
		for(;;)
		{
			for( ; dx < limit; dx++, alpha += 4 )
			{
				int j, sx = xofs[dx] - cn;
				int v = 0;
				for( j = 0; j < 4; j++ )
				{
					int sxj = sx + j*cn;
					if( (unsigned)sxj >= (unsigned)swidth )
					{
						while( sxj < 0 )
							sxj += cn;
						while( sxj >= swidth )
							sxj -= cn;
					}
					v += S[sxj]*alpha[j];
				}
				D[dx] = v;
			}
			if( limit == dwidth )
				break;
			for( ; dx < xmax; dx++, alpha += 4 )
			{
				int sx = xofs[dx];
				D[dx] = S[sx-cn]*alpha[0] + S[sx]*alpha[1] +
					S[sx+cn]*alpha[2] + S[sx+cn*2]*alpha[3];
			}
			limit = dwidth;
		}
		alpha -= dwidth*4;
	}
}

unsigned char FixedPtCast(int val){
	int bits = INTER_RESIZE_COEF_BITS * 2;
	int SHIFT = bits, DELTA = 1 << (bits-1);
	return saturate_cast_uchar((val + DELTA)>>SHIFT);
}

void vresize_cubic(const int** src, unsigned char* dst, const short* beta, int width )
{
	int b0 = beta[0], b1 = beta[1], b2 = beta[2], b3 = beta[3];
	const int *S0 = src[0], *S1 = src[1], *S2 = src[2], *S3 = src[3];
	// CastOp castOp;
	// VecOp vecOp;

	int x = 0;//vecOp(src, dst, beta, width);
	for( ; x < width; x++ ){
		dst[x] = FixedPtCast((S0[x]*b0 + S1[x]*b1 + S2[x]*b2 + S3[x]*b3));
	}
}

static void resizeGeneric_Cubic(resize_image src, resize_image dst,
	const int* xofs, const void* _alpha, const int* yofs, const void* _beta, int xmin, int xmax, int ksize)
{
	Size ssize = {src.width, src.height};
	Size dsize = {dst.width, dst.height};
	int dy, cn = 3;
	ssize.width *= cn;
	dsize.width *= cn;
	xmin *= cn;
	xmax *= cn;
	// image resize is a separable operation. In case of not too strong

	Range range = {0, dsize.height};

	int bufstep = (int)alignSize(dsize.width, 4);
	int _buffer[bufstep*ksize];
	const unsigned char* srows[MAX_ESIZE];
	memset(srows, 0, sizeof(unsigned char)*MAX_ESIZE);
	int* rows[MAX_ESIZE];
	memset(rows, 0, sizeof(int)*MAX_ESIZE);
	int prev_sy[MAX_ESIZE];

	for (int k = 0; k < ksize; k++) {
		prev_sy[k] = -1;
		rows[k] = (int*)_buffer + bufstep*k;
	}

	const short* beta = (const short*)_beta + ksize * range.start;

	for (dy = range.start; dy < range.end; dy++, beta += ksize) {
		int sy0 = yofs[dy], k0 = ksize, k1 = 0, ksize2 = ksize / 2;

		for (int k = 0; k < ksize; k++) {
			int sy = clip(sy0 - ksize2 + 1 + k, 0, ssize.height);
			for (k1 = cvmax(k1, k); k1 < ksize; k1++) {
				if (sy == prev_sy[k1]) { // if the sy-th row has been computed already, reuse it.
					if (k1 > k) {
						memcpy(rows[k], rows[k1], bufstep*sizeof(rows[0][0]));
					}
					break;
				}
			}
			if (k1 == ksize) {
				k0 = cvmin(k0, k); // remember the first row that needs to be computed
			}
			srows[k] = (const unsigned char*)(src.data + src.step * sy);
			prev_sy[k] = sy;
		}

		if (k0 < ksize) {
			hresize_cubic((const unsigned char**)(srows + k0), (int**)(rows + k0), ksize - k0, xofs, (const short*)(_alpha),
				ssize.width, dsize.width, cn, xmin, xmax);
		}
		vresize_cubic((const int**)rows, (unsigned char*)(dst.data + dst.step*dy), beta, dsize.width);
	}
}

static void interpolateCubic( float x, float* coeffs )
{
    const float A = -0.75f;

    coeffs[0] = ((A*(x + 1) - 5*A)*(x + 1) + 8*A)*(x + 1) - 4*A;
    coeffs[1] = ((A + 2)*x - (A + 3))*x*x + 1;
    coeffs[2] = ((A + 2)*(1 - x) - (A + 3))*(1 - x)*(1 - x) + 1;
    coeffs[3] = 1.f - coeffs[0] - coeffs[1] - coeffs[2];
}

int resize_cubic(resize_image src, resize_image dst)
{
	Size ssize = {src.width, src.height};
	Size dsize = {dst.width, dst.height};

	double inv_scale_x = (double)dsize.width / ssize.width;
	double inv_scale_y = (double)dsize.height / ssize.height;
	double scale_x = 1. / inv_scale_x, scale_y = 1. / inv_scale_y;

	int cn = 3;
	int k, sx, sy, dx, dy;
	int xmin = 0, xmax = dsize.width, width = dsize.width*cn;
	int fixpt = 1;
	float fx, fy;
	int ksize = 4, ksize2;
	ksize2 = ksize / 2;

	unsigned char _buffer[(width + dsize.height)*(sizeof(int) + sizeof(float)*ksize)];
	memset(_buffer, 0, sizeof(unsigned char)*(width + dsize.height)*(sizeof(int) + sizeof(float)*ksize));
	int* xofs = (int*)(unsigned char*)_buffer;
	int* yofs = xofs + width;
	float* alpha = (float*)(yofs + dsize.height);
	short* ialpha = (short*)alpha;
	float* beta = alpha + width*ksize;
	short* ibeta = ialpha + width*ksize;
	float cbuf[MAX_ESIZE];

	for (dx = 0; dx < dsize.width; dx++) {
		fx = (float)((dx + 0.5)*scale_x - 0.5);
		sx = cvFloor(fx);
		fx -= sx;

		if (sx < ksize2 - 1) {
			xmin = dx + 1;
		}

		if (sx + ksize2 >= ssize.width) {
			xmax = cvmin(xmax, dx);
		}

		for (k = 0, sx *= cn; k < cn; k++) {
			xofs[dx*cn + k] = sx + k;
		}

		interpolateCubic(fx, cbuf);

		if (fixpt) {
			for (k = 0; k < ksize; k++) {
				ialpha[dx*cn*ksize + k] = (short)cvround((cbuf[k] * INTER_RESIZE_COEF_SCALE));
			}
			for (; k < cn*ksize; k++) {
				ialpha[dx*cn*ksize + k] = ialpha[dx*cn*ksize + k - ksize];
			}
		} else {
			for (k = 0; k < ksize; k++) {
				alpha[dx*cn*ksize + k] = cbuf[k];
			}
			for (; k < cn*ksize; k++) {
				alpha[dx*cn*ksize + k] = alpha[dx*cn*ksize + k - ksize];
			}
		}
	}

	for (dy = 0; dy < dsize.height; dy++) {
		fy = (float)((dy + 0.5)*scale_y - 0.5);
		sy = cvFloor(fy);
		fy -= sy;

		yofs[dy] = sy;
		interpolateCubic(fy, cbuf);

		if (fixpt) {
			for (k = 0; k < ksize; k++) {
				ibeta[dy*ksize + k] = (short)cvround((cbuf[k] * INTER_RESIZE_COEF_SCALE));
			}
		} else {
			for (k = 0; k < ksize; k++) {
				beta[dy*ksize + k] = cbuf[k];
			}
		}
	}

	resizeGeneric_Cubic(src, dst,
		xofs, fixpt ? (void*)ialpha : (void*)alpha, yofs, fixpt ? (void*)ibeta : (void*)beta, xmin, xmax, ksize);

	return 0;
}
