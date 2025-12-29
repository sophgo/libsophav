#include <bmcv_cmodel.h>

void Resize_NN(resize_image src, resize_image dst)
{
    double inv_scale_x = (double)dst.width / src.width;
    double inv_scale_y = (double)dst.height / src.height;

    int _x_ofs[dst.width];
    int* x_ofs = _x_ofs;
    int pix_size = src.elemSize;
    int pix_size_ = (int)(pix_size / sizeof(int));
    double ifx = 1. / inv_scale_x, ify = 1. / inv_scale_y;

    int x1;
    for (x1 = 0; x1 < dst.width; x1++)
    {
        int sx = cvFloor(x1*ifx);
        x_ofs[x1] = cvmin(sx, src.width - 1)*pix_size;
    }
    int x;
    for (int y = 0; y < dst.height; y++)
    {
        unsigned char* D = dst.data + dst.step*y;
        int sy = cvmin(cvFloor(y*ify), src.height - 1);
        const unsigned char* S = src.data + src.step*sy;

        switch (pix_size)
        {
        case 1:
            for (x = 0; x <= dst.width - 2; x += 2)
            {
                unsigned char t0 = S[x_ofs[x]];
                unsigned char t1 = S[x_ofs[x + 1]];
                D[x] = t0;
                D[x + 1] = t1;
            }

            for (; x < dst.width; x++)
                D[x] = S[x_ofs[x]];
            break;
        case 2:
            for (x = 0; x < dst.width; x++)
                *(ushort*)(D + x * 2) = *(ushort*)(S + x_ofs[x]);
            break;
        case 3:
            for (x = 0; x < dst.width; x++, D += 3)
            {
                const unsigned char* _tS = S + x_ofs[x];
                D[0] = _tS[0]; D[1] = _tS[1]; D[2] = _tS[2];
            }
            break;
        case 4:
            for (x = 0; x < dst.width; x++)
                *(int*)(D + x * 4) = *(int*)(S + x_ofs[x]);
            break;
        case 6:
            for (x = 0; x < dst.width; x++, D += 6)
            {
                const ushort* _tS = (const ushort*)(S + x_ofs[x]);
                ushort* _tD = (ushort*)D;
                _tD[0] = _tS[0]; _tD[1] = _tS[1]; _tD[2] = _tS[2];
            }
            break;
        case 8:
            for (x = 0; x < dst.width; x++, D += 8)
            {
                const int* _tS = (const int*)(S + x_ofs[x]);
                int* _tD = (int*)D;
                _tD[0] = _tS[0]; _tD[1] = _tS[1];
            }
            break;
        case 12:
            for (x = 0; x < dst.width; x++, D += 12)
            {
                const int* _tS = (const int*)(S + x_ofs[x]);
                int* _tD = (int*)D;
                _tD[0] = _tS[0]; _tD[1] = _tS[1]; _tD[2] = _tS[2];
            }
            break;
        default:
            for (x = 0; x < dst.width; x++, D += pix_size)
            {
                const int* _tS = (const int*)(S + x_ofs[x]);
                int* _tD = (int*)D;
                for (int k = 0; k < pix_size_; k++)
                    _tD[k] = _tS[k];
            }
        }
    }
}
