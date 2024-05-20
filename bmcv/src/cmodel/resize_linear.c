#include <bmcv_cmodel.h>

void hresize(const unsigned char** src, int** dst, int count,
	const int* xofs, const short* alpha,
	int swidth, int dwidth, int cn, int xmin, int xmax)
{
	int ONE = INTER_RESIZE_COEF_SCALE;
	int dx, k;
	//VecOp vecOp;

	int dx0 = 0;//vecOp((const unsigned char**)src, (unsigned char**)dst, count, xofs, (const unsigned char*)alpha, swidth, dwidth, cn, xmin, xmax);

	for (k = 0; k <= count - 2; k+=2)
	{
		const unsigned char *S0 = src[k], *S1 = src[k + 1];
		int *D0 = dst[k], *D1 = dst[k + 1];
		for (dx = dx0; dx < xmax; dx++)
		{
			int sx = xofs[dx];
			int a0 = alpha[dx * 2], a1 = alpha[dx * 2 + 1];
			int t0 = S0[sx] * a0 + S0[sx + cn] * a1;
			int t1 = S1[sx] * a0 + S1[sx + cn] * a1;
			D0[dx] = t0; D1[dx] = t1;
		}

		for (; dx < dwidth; dx++)
		{
			int sx = xofs[dx];
			D0[dx] = (int)(S0[sx] * ONE); D1[dx] = (int)(S1[sx] * ONE);
		}
	}

	for (; k < count; k++)
	{
		const unsigned char *S = src[k];
		int *D = dst[k];
		for (dx = 0; dx < xmax; dx++)
		{
			int sx = xofs[dx];
			D[dx] = S[sx] * alpha[dx * 2] + S[sx + cn] * alpha[dx * 2 + 1];
		}

		for (; dx < dwidth; dx++)
			D[dx] = (int)(S[xofs[dx]] * ONE);
	}
}

void vresize(const int** src, unsigned char* dst, const short* beta, int width)
{
	short b0 = beta[0], b1 = beta[1];
	const int *S0 = src[0], *S1 = src[1];
	//VResizeLinearVec_32s8u vecOp;

	int x = 0;//vecOp((const unsigned char**)src, (unsigned char*)dst, (const unsigned char*)beta, width);
#if CV_ENABLE_UNROLLED
	for (; x <= width - 4; x += 4)
	{
		dst[x + 0] = (unsigned char)((((b0 * (S0[x + 0] >> 4)) >> 16) + ((b1 * (S1[x + 0] >> 4)) >> 16) + 2) >> 2);
		dst[x + 1] = (unsigned char)((((b0 * (S0[x + 1] >> 4)) >> 16) + ((b1 * (S1[x + 1] >> 4)) >> 16) + 2) >> 2);
		dst[x + 2] = (unsigned char)((((b0 * (S0[x + 2] >> 4)) >> 16) + ((b1 * (S1[x + 2] >> 4)) >> 16) + 2) >> 2);
		dst[x + 3] = (unsigned char)((((b0 * (S0[x + 3] >> 4)) >> 16) + ((b1 * (S1[x + 3] >> 4)) >> 16) + 2) >> 2);
	}
#endif
	for (; x < width; x++)
		dst[x] = (unsigned char)((((b0 * (S0[x] >> 4)) >> 16) + ((b1 * (S1[x] >> 4)) >> 16) + 2) >> 2);
}

void invoker(const int *xofs, const int *yofs,
	const short* alpha, const short* _beta,
	int ksize, int xmin, int xmax, resize_image src, resize_image dst)
{
    int dy = 0, cn = 3;

	int bufstep = ((dst.width * 3 + 15) >> 4) << 4;
	int _buffer[bufstep*ksize];
	const unsigned char* srows[MAX_ESIZE];
	int* rows[MAX_ESIZE];
	int prev_sy[MAX_ESIZE];

	for (int k = 0; k < ksize; k++)
	{
		prev_sy[k] = -1;
		rows[k] = (int*)_buffer + bufstep*k;
	}

	const short* beta = _beta + ksize * 0;

	for (dy = 0; dy < dst.height; dy++, beta += ksize)
	{
		int sy0 = yofs[dy], k0 = ksize, k1 = 0, ksize2 = ksize / 2;

		for (int k = 0; k < ksize; k++)
		{
			int sy = clip(sy0 - ksize2 + 1 + k, 0, src.height);
			for (k1 = cvmax(k1, k); k1 < ksize; k1++)
			{
				if (k1 < MAX_ESIZE && sy == prev_sy[k1]) // if the sy-th row has been computed already, reuse it.
				{
					if (k1 > k)
						memcpy(rows[k], rows[k1], bufstep*sizeof(rows[0][0]));
					break;
				}
			}
			if (k1 == ksize)
				k0 = cvmin(k0, k); // remember the first row that needs to be computed
			srows[k] = (unsigned char*)(src.data + src.step*sy);
			prev_sy[k] = sy;
		}

		if (k0 < ksize)
			hresize((const unsigned char**)(srows + k0), (int**)(rows + k0), ksize - k0, xofs, (const short*)(alpha),
			src.width * 3, dst.width * 3, cn, xmin, xmax);
		vresize((const int**)rows, (unsigned char*)(dst.data + dst.step*dy), beta, dst.width * 3);
	}
}

static void resizeGeneric_(
	const int* xofs, const void* _alpha,
	const int* yofs, const void* _beta,
	int xmin, int xmax, int ksize, resize_image src, resize_image dst)
{
	const short* beta = (const short*)_beta;
	int cn = 3;
	xmin *= cn;
	xmax *= cn;
	// image resize is a separable operation. In case of not too strong

	//resizeGeneric_Invoker<HResize, VResize> invoker(src, dst, xofs, yofs, (const short*)_alpha, beta,
	//ssize, dsize, ksize, xmin, xmax);
	//parallel_for_(range, invoker, dst.total() / (double)(1 << 16));
	invoker(xofs, yofs, (const short*)_alpha, beta,
		ksize, xmin, xmax, src, dst);
}

void Resize_linear(resize_image src, resize_image dst)
{
	double inv_scale_x;
    double inv_scale_y;

    inv_scale_x = (double)dst.width / src.width;
    inv_scale_y = (double)dst.height / src.height;

	int depth = 0, cn = 3;
	double scale_x = 1. / inv_scale_x, scale_y = 1. / inv_scale_y;
	// float scale_x = (float)(src.width) / (float)(dst.width), scale_y = (float)(src.height) / (float)(dst.height);
	int k, sx, sy, dx, dy;

	int xmin = 0, xmax = dst.width, width = dst.width*cn;
	int area_mode = 1;
	area_mode = 0;
	int fixpt = depth == 0;
	float fx, fy;
	int ksize = 0, ksize2;
	ksize = 2;
	//func = linear_tab[depth];
	ksize2 = ksize / 2;

	//CV_Assert(func != 0);

	unsigned char _buffer[(width + dst.height)*(sizeof(int)+sizeof(float)*ksize)];
	int* xofs = (int*)_buffer;
	int* yofs = xofs + width;
	float* alpha = (float*)(yofs + dst.height);
	short* ialpha = (short*)alpha;
	float* beta = alpha + width*ksize;
	short* ibeta = ialpha + width*ksize;
	float cbuf[MAX_ESIZE];
	float value;

	for (dx = 0; dx < dst.width; dx++)
	{
		if (!area_mode)
		{
			fx = (float)((dx + 0.5)*scale_x - 0.5);
			sx = cvFloor(fx);
			fx -= sx;
		}
		else
		{
			sx = cvFloor(dx*scale_x);
			fx = (float)((dx + 1) - (sx + 1)*inv_scale_x);
			fx = fx <= 0 ? 0.f : fx - cvFloor(fx);
		}
		if (sx < ksize2 - 1)
		{
			xmin = dx + 1;
			if (sx < 0)
				fx = 0, sx = 0;
		}
		if (sx + ksize2 >= src.width)
		{
			xmax = cvmin(xmax, dx);
			if (sx >= src.width - 1)
				fx = 0, sx = src.width - 1;
		}
		for (k = 0, sx *= cn; k < cn; k++)
			xofs[dx*cn + k] = sx + k;

		cbuf[0] = 1.f - fx;
		cbuf[1] = fx;

		if (fixpt)
		{
			for (k = 0; k < ksize; k++){
				value = cvround(cbuf[k] * INTER_RESIZE_COEF_SCALE);
				ialpha[dx*cn*ksize + k] = (short)value;
			}
			for (; k < cn*ksize; k++)
				ialpha[dx*cn*ksize + k] = ialpha[dx*cn*ksize + k - ksize];
		}
		else
		{
			for (k = 0; k < ksize; k++)
				alpha[dx*cn*ksize + k] = cbuf[k];
			for (; k < cn*ksize; k++)
				alpha[dx*cn*ksize + k] = alpha[dx*cn*ksize + k - ksize];
		}
	}

	for (dy = 0; dy < dst.height; dy++)
	{
		if (!area_mode)
		{
			fy = (float)((dy + 0.5)*scale_y - 0.5);
			sy = cvFloor(fy);
			fy -= sy;
		}
		else
		{
			sy = cvFloor(dy*scale_y);
			fy = (float)((dy + 1) - (sy + 1)*inv_scale_y);
			fy = fy <= 0 ? 0.f : fy - cvFloor(fy);
		}

		yofs[dy] = sy;
		cbuf[0] = 1.f - fy;
		cbuf[1] = fy;

		if (fixpt)
		{
			for (k = 0; k < ksize; k++){
				value = (double)cvround(cbuf[k] * INTER_RESIZE_COEF_SCALE);
				ibeta[dy*ksize + k] = (short)value;
			}
		}
		else
		{
			for (k = 0; k < ksize; k++)
				beta[dy*ksize + k] = cbuf[k];
		}
	}

	resizeGeneric_(xofs, fixpt ? (void*)ialpha : (void*)alpha, yofs,
		fixpt ? (void*)ibeta : (void*)beta, xmin, xmax, ksize, src, dst);
}