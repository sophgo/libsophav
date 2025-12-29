#include <memory.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include "bmcv_a2_dpu_cmodel.h"
#ifdef __linux__
#include <sys/ioctl.h>
#endif

// ----------------------------------FGS Parameters----------------------------------
// static value
unsigned int w_scale;
unsigned int c_scale;
unsigned int f_scale;
int T;
int max_count;
int width = 1920;   // max 1920
int height = 1080;  // max 1080

// Parameters Array
static unsigned short weights_fx_a_c[96] = {16384, 9937, 6027, 3656, 2217, 1345, 816, 495, 300, 182, 110, 67, 41, 25, 15, 9, 5, 3, 2, 1,
                                            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4096, 2484, 1507, 914, 554, 336, 204, 124, 75, 46, 28, 17,
                                            10, 6, 4, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1024, 621, 377, 228, 139, 84, 51,
                                            31, 19, 11, 7, 4, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static unsigned short weights_fx_b[3072] = {32896, 26449, 22539, 20168, 18729, 17857, 17328, 17007, 16812, 16694, 16622, 16579, 16553, 16537, 16527,
                                            16521, 16517, 16515, 16514, 16513, 16513, 16512, 16512, 16512, 16512, 16512, 16512, 16512, 16512, 16512,
                                            16512, 16512, 26449, 20003, 16093, 13721, 12283, 11410, 10881, 10560, 10365, 10247, 10176, 10132, 10106,
                                            10090, 10080, 10074, 10071, 10069, 10067, 10067, 10066, 10066, 10066, 10066, 10065, 10065, 10065, 10065,
                                            10065, 10065, 10065, 10065, 22539, 16093, 12183, 9811, 8373, 7500, 6971, 6650, 6455, 6337, 6266, 6222, 6196,
                                            6180, 6170, 6164, 6161, 6159, 6157, 6157, 6156, 6156, 6156, 6156, 6155, 6155, 6155, 6155, 6155, 6155, 6155,
                                            6155, 20168, 13721, 9811, 7440, 6001, 5129, 4599, 4279, 4084, 3966, 3894, 3851, 3824, 3808, 3799, 3793, 3789,
                                            3787, 3786, 3785, 3785, 3784, 3784, 3784, 3784, 3784, 3784, 3784, 3784, 3784, 3784, 3784, 18729, 12283, 8373,
                                            6001, 4563, 3690, 3161, 2840, 2645, 2527, 2456, 2412, 2386, 2370, 2360, 2354, 2351, 2349, 2347, 2347, 2346, 2346,
                                            2346, 2345, 2345, 2345, 2345, 2345, 2345, 2345, 2345, 2345, 17857, 11410, 7500, 5129, 3690, 2818, 2289, 1968, 1773,
                                            1655, 1583, 1540, 1513, 1498, 1488, 1482, 1478, 1476, 1475, 1474, 1474, 1473, 1473, 1473, 1473, 1473, 1473, 1473, 1473,
                                            1473, 1473, 1473, 17328, 10881, 6971, 4599, 3161, 2289, 1759, 1438, 1244, 1126, 1054, 1011, 984, 968, 959, 953, 949, 947,
                                            946, 945, 944, 944, 944, 944, 944, 944, 944, 944, 944, 944, 944, 944, 17007, 10560, 6650, 4279, 2840, 1968, 1438, 1118,
                                            923, 805, 733, 690, 663, 647, 638, 632, 628, 626, 625, 624, 623, 623, 623, 623, 623, 623, 623, 623, 623, 623, 623, 623,
                                            16812, 10365, 6455, 4084, 2645, 1773, 1244, 923, 728, 610, 538, 495, 469, 453, 443, 437, 434, 431, 430, 429, 429, 429,
                                            428, 428, 428, 428, 428, 428, 428, 428, 428, 428, 16694, 10247, 6337, 3966, 2527, 1655, 1126, 805, 610, 492, 420, 377,
                                            351, 335, 325, 319, 316, 313, 312, 311, 311, 310, 310, 310, 310, 310, 310, 310, 310, 310, 310, 310, 16622, 10176, 6266,
                                            3894, 2456, 1583, 1054, 733, 538, 420, 349, 305, 279, 263, 253, 247, 244, 242, 240, 240, 239, 239, 239, 239, 238, 238,
                                            238, 238, 238, 238, 238, 238, 16579, 10132, 6222, 3851, 2412, 1540, 1011, 690, 495, 377, 305, 262, 236, 220, 210, 204,
                                            200, 198, 197, 196, 196, 195, 195, 195, 195, 195, 195, 195, 195, 195, 195, 195, 16553, 10106, 6196, 3824, 2386, 1513,
                                            984, 663, 469, 351, 279, 236, 209, 193, 184, 178, 174, 172, 171, 170, 169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
                                            169, 169, 16537, 10090, 6180, 3808, 2370, 1498, 968, 647, 453, 335, 263, 220, 193, 177, 168, 162, 158, 156, 155, 154,
                                            153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 16527, 10080, 6170, 3799, 2360, 1488, 959, 638, 443, 325,
                                            253, 210, 184, 168, 158, 152, 148, 146, 145, 144, 144, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 16521,
                                            10074, 6164, 3793, 2354, 1482, 953, 632, 437, 319, 247, 204, 178, 162, 152, 146, 143, 140, 139, 138, 138, 138, 137,
                                            137, 137, 137, 137, 137, 137, 137, 137, 137, 16517, 10071, 6161, 3789, 2351, 1478, 949, 628, 434, 316, 244, 200, 174,
                                            158, 148, 143, 139, 137, 136, 135, 134, 134, 134, 134, 134, 134, 134, 134, 134, 134, 134, 133, 16515, 10069, 6159, 3787,
                                            2349, 1476, 947, 626, 431, 313, 242, 198, 172, 156, 146, 140, 137, 135, 133, 133, 132, 132, 132, 131, 131, 131, 131, 131,
                                            131, 131, 131, 131, 16514, 10067, 6157, 3786, 2347, 1475, 946, 625, 430, 312, 240, 197, 171, 155, 145, 139, 136, 133, 132,
                                            131, 131, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 16513, 10067, 6157, 3785, 2347, 1474, 945, 624, 429, 311,
                                            240, 196, 170, 154, 144, 138, 135, 133, 131, 130, 130, 130, 130, 129, 129, 129, 129, 129, 129, 129, 129, 129, 16513, 10066,
                                            6156, 3785, 2346, 1474, 944, 623, 429, 311, 239, 196, 169, 153, 144, 138, 134, 132, 131, 130, 129, 129, 129, 129, 129, 129,
                                            129, 129, 129, 129, 129, 129, 16512, 10066, 6156, 3784, 2346, 1473, 944, 623, 429, 310, 239, 195, 169, 153, 143, 138, 134,
                                            132, 130, 130, 129, 129, 129, 129, 129, 129, 128, 128, 128, 128, 128, 128, 16512, 10066, 6156, 3784, 2346, 1473, 944, 623,
                                            428, 310, 239, 195, 169, 153, 143, 137, 134, 132, 130, 130, 129, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128,
                                            16512, 10066, 6156, 3784, 2345, 1473, 944, 623, 428, 310, 239, 195, 169, 153, 143, 137, 134, 131, 130, 129, 129, 129, 128,
                                            128, 128, 128, 128, 128, 128, 128, 128, 128, 16512, 10065, 6155, 3784, 2345, 1473, 944, 623, 428, 310, 238, 195, 169, 153,
                                            143, 137, 134, 131, 130, 129, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 16512, 10065, 6155, 3784, 2345,
                                            1473, 944, 623, 428, 310, 238, 195, 169, 153, 143, 137, 134, 131, 130, 129, 129, 129, 128, 128, 128, 128, 128, 128, 128,
                                            128, 128, 128, 16512, 10065, 6155, 3784, 2345, 1473, 944, 623, 428, 310, 238, 195, 169, 153, 143, 137, 134, 131, 130, 129,
                                            129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 16512, 10065, 6155, 3784, 2345, 1473, 944, 623, 428, 310, 238,
                                            195, 169, 153, 143, 137, 134, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 16512, 10065, 6155,
                                            3784, 2345, 1473, 944, 623, 428, 310, 238, 195, 169, 153, 143, 137, 134, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128,
                                            128, 128, 128, 128, 128, 16512, 10065, 6155, 3784, 2345, 1473, 944, 623, 428, 310, 238, 195, 169, 153, 143, 137, 134, 131,
                                            130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 16512, 10065, 6155, 3784, 2345, 1473, 944, 623, 428,
                                            310, 238, 195, 169, 153, 143, 137, 134, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
                                            16512, 10065, 6155, 3784, 2345, 1473, 944, 623, 428, 310, 238, 195, 169, 153, 143, 137, 133, 131, 130, 129, 129, 128, 128,
                                            128, 128, 128, 128, 128, 128, 128, 128, 128, 8320, 6708, 5731, 5138, 4778, 4560, 4428, 4348, 4299, 4270, 4252, 4241, 4234,
                                            4230, 4228, 4226, 4225, 4225, 4225, 4224, 4224, 4224, 4224, 4224, 4224, 4224, 4224, 4224, 4224, 4224, 4224, 4224, 6708,
                                            5097, 4119, 3526, 3167, 2949, 2816, 2736, 2687, 2658, 2640, 2629, 2623, 2619, 2616, 2615, 2614, 2613, 2613, 2613, 2613,
                                            2612, 2612, 2612, 2612, 2612, 2612, 2612, 2612, 2612, 2612, 2612, 5731, 4119, 3142, 2549, 2189, 1971, 1839, 1759, 1710,
                                            1680, 1662, 1652, 1645, 1641, 1639, 1637, 1636, 1636, 1635, 1635, 1635, 1635, 1635, 1635, 1635, 1635, 1635, 1635, 1635,
                                            1635, 1635, 1635, 5138, 3526, 2549, 1956, 1596, 1378, 1246, 1166, 1117, 1087, 1070, 1059, 1052, 1048, 1046, 1044, 1043,
                                            1043, 1042, 1042, 1042, 1042, 1042, 1042, 1042, 1042, 1042, 1042, 1042, 1042, 1042, 1042, 4778, 3167, 2189, 1596, 1237,
                                            1019, 886, 806, 757, 728, 710, 699, 692, 688, 686, 685, 684, 683, 683, 683, 683, 682, 682, 682, 682, 682, 682, 682, 682,
                                            682, 682, 682, 4560, 2949, 1971, 1378, 1019, 800, 668, 588, 539, 510, 492, 481, 474, 470, 468, 466, 466, 465, 465, 465,
                                            464, 464, 464, 464, 464, 464, 464, 464, 464, 464, 464, 464, 4428, 2816, 1839, 1246, 886, 668, 536, 456, 407, 377, 360,
                                            349, 342, 338, 336, 334, 333, 333, 332, 332, 332, 332, 332, 332, 332, 332, 332, 332, 332, 332, 332, 332, 4348, 2736,
                                            1759, 1166, 806, 588, 456, 375, 327, 297, 279, 268, 262, 258, 255, 254, 253, 253, 252, 252, 252, 252, 252, 252, 252, 252,
                                            252, 252, 252, 252, 252, 252, 4299, 2687, 1710, 1117, 757, 539, 407, 327, 278, 249, 231, 220, 213, 209, 207, 205, 204, 204,
                                            204, 203, 203, 203, 203, 203, 203, 203, 203, 203, 203, 203, 203, 203, 4270, 2658, 1680, 1087, 728, 510, 377, 297, 249, 219,
                                            201, 190, 184, 180, 177, 176, 175, 174, 174, 174, 174, 174, 174, 174, 174, 174, 174, 174, 174, 174, 174, 174, 4252, 2640,
                                            1662, 1070, 710, 492, 360, 279, 231, 201, 183, 172, 166, 162, 159, 158, 157, 156, 156, 156, 156, 156, 156, 156, 156, 156,
                                            156, 156, 156, 156, 156, 156, 4241, 2629, 1652, 1059, 699, 481, 349, 268, 220, 190, 172, 161, 155, 151, 148, 147, 146, 146,
                                            145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 4234, 2623, 1645, 1052, 692, 474, 342, 262, 213, 184,
                                            166, 155, 148, 144, 142, 140, 140, 139, 139, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 4230, 2619,
                                            1641, 1048, 688, 470, 338, 258, 209, 180, 162, 151, 144, 140, 138, 136, 136, 135, 135, 134, 134, 134, 134, 134, 134, 134,
                                            134, 134, 134, 134, 134, 134, 4228, 2616, 1639, 1046, 686, 468, 336, 255, 207, 177, 159, 148, 142, 138, 135, 134, 133, 133,
                                            132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 4226, 2615, 1637, 1044, 685, 466, 334, 254, 205, 176,
                                            158, 147, 140, 136, 134, 133, 132, 131, 131, 131, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 4225, 2614,
                                            1636, 1043, 684, 466, 333, 253, 204, 175, 157, 146, 140, 136, 133, 132, 131, 130, 130, 130, 130, 129, 129, 129, 129, 129,
                                            129, 129, 129, 129, 129, 129, 4225, 2613, 1636, 1043, 683, 465, 333, 253, 204, 174, 156, 146, 139, 135, 133, 131, 130, 130,
                                            129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 4225, 2613, 1635, 1042, 683, 465, 332, 252, 204, 174,
                                            156, 145, 139, 135, 132, 131, 130, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 4224, 2613,
                                            1635, 1042, 683, 465, 332, 252, 203, 174, 156, 145, 138, 134, 132, 131, 130, 129, 129, 129, 128, 128, 128, 128, 128, 128,
                                            128, 128, 128, 128, 128, 128, 4224, 2613, 1635, 1042, 683, 464, 332, 252, 203, 174, 156, 145, 138, 134, 132, 130, 130, 129,
                                            129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 4224, 2612, 1635, 1042, 682, 464, 332, 252, 203, 174,
                                            156, 145, 138, 134, 132, 130, 129, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 4224, 2612,
                                            1635, 1042, 682, 464, 332, 252, 203, 174, 156, 145, 138, 134, 132, 130, 129, 129, 129, 128, 128, 128, 128, 128, 128, 128,
                                            128, 128, 128, 128, 128, 128, 4224, 2612, 1635, 1042, 682, 464, 332, 252, 203, 174, 156, 145, 138, 134, 132, 130, 129, 129,
                                            129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 4224, 2612, 1635, 1042, 682, 464, 332, 252, 203, 174,
                                            156, 145, 138, 134, 132, 130, 129, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 4224, 2612,
                                            1635, 1042, 682, 464, 332, 252, 203, 174, 156, 145, 138, 134, 132, 130, 129, 129, 129, 128, 128, 128, 128, 128, 128, 128,
                                            128, 128, 128, 128, 128, 128, 4224, 2612, 1635, 1042, 682, 464, 332, 252, 203, 174, 156, 145, 138, 134, 132, 130, 129, 129,
                                            129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 4224, 2612, 1635, 1042, 682, 464, 332, 252, 203, 174,
                                            156, 145, 138, 134, 132, 130, 129, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 4224, 2612, 1635, 1042, 682, 464, 332, 252, 203, 174, 156, 145, 138, 134, 132, 130, 129, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 4224, 2612, 1635, 1042, 682, 464, 332, 252, 203, 174, 156, 145, 138, 134, 132, 130, 129, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 4224, 2612, 1635, 1042, 682, 464, 332, 252, 203, 174, 156, 145, 138, 134, 132, 130, 129, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 4224, 2612, 1635, 1042, 682, 464, 332, 252, 203, 174, 156, 145, 138, 134, 132, 130, 129, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 2176, 1773, 1529, 1380, 1291, 1236, 1203, 1183, 1171, 1163, 1159, 1156, 1155, 1154, 1153, 1153, 1152, 1152, 1152, 1152, 1152, 1152, 1152, 1152, 1152, 1152, 1152, 1152, 1152, 1152, 1152, 1152, 1773, 1370, 1126, 978, 888, 833, 800, 780, 768, 760, 756, 753, 752, 751, 750, 750, 749, 749, 749, 749, 749, 749, 749, 749, 749, 749, 749, 749, 749, 749, 749, 749, 1529, 1126, 881, 733, 643, 589, 556, 536, 523, 516, 512, 509, 507, 506, 506, 505, 505, 505, 505, 505, 505, 505, 505, 505, 505, 505, 505, 505, 505, 505, 505, 505, 1380, 978, 733, 585, 495, 441, 407, 387, 375, 368, 363, 361, 359, 358, 357, 357, 357, 357, 357, 357, 357, 357, 357, 356, 356, 356, 356, 356, 356, 356, 356, 356, 1291, 888, 643, 495, 405, 351, 318, 298, 285, 278, 273, 271, 269, 268, 268, 267, 267, 267, 267, 267, 267, 267, 267, 267, 267, 267, 267, 267, 267, 267, 267, 267, 1236, 833, 589, 441, 351, 296, 263, 243, 231, 223, 219, 216, 215, 214, 213, 213, 212, 212, 212, 212, 212, 212, 212, 212, 212, 212, 212, 212, 212, 212, 212, 212, 1203, 800, 556, 407, 318, 263, 230, 210, 198, 190, 186, 183, 182, 181, 180, 180, 179, 179, 179, 179, 179, 179, 179, 179, 179, 179, 179, 179, 179, 179, 179, 179, 1183, 780, 536, 387, 298, 243, 210, 190, 178, 170, 166, 163, 161, 160, 160, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 1171, 768, 523, 375, 285, 231, 198, 178, 166, 158, 154, 151, 149, 148, 148, 147, 147, 147, 147, 147, 147, 147, 147, 147, 147, 147, 147, 147, 147, 147, 147, 147, 1163, 760, 516, 368, 278, 223, 190, 170, 158, 151, 146, 144, 142, 141, 140, 140, 140, 140, 140, 139, 139, 139, 139, 139, 139, 139, 139, 139, 139, 139, 139, 139, 1159, 756, 512, 363, 273, 219, 186, 166, 154, 146, 142, 139, 137, 136, 136, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 1156, 753, 509, 361, 271, 216, 183, 163, 151, 144, 139, 136, 135, 134, 133, 133, 133, 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 1155, 752, 507, 359, 269, 215, 182, 161, 149, 142, 137, 135, 133, 132, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 1154, 751, 506, 358, 268, 214, 181, 160, 148, 141, 136, 134, 132, 131, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 1153, 750, 506, 357, 268, 213, 180, 160, 148, 140, 136, 133, 131, 130, 130, 130, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 1153, 750, 505, 357, 267, 213, 180, 159, 147, 140, 135, 133, 131, 130, 130, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 1152, 749, 505, 357, 267, 212, 179, 159, 147, 140, 135, 133, 131, 130, 129, 129, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 1152, 749, 505, 357, 267, 212, 179, 159, 147, 140, 135, 132, 131, 130, 129, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 1152, 749, 505, 357, 267, 212, 179, 159, 147, 140, 135, 132, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 1152, 749, 505, 357, 267, 212, 179, 159, 147, 139, 135, 132, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 1152, 749, 505, 357, 267, 212, 179, 159, 147, 139, 135, 132, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 1152, 749, 505, 357, 267, 212, 179, 159, 147, 139, 135, 132, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 1152, 749, 505, 357, 267, 212, 179, 159, 147, 139, 135, 132, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 1152, 749, 505, 356, 267, 212, 179, 159, 147, 139, 135, 132, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 1152, 749, 505, 356, 267, 212, 179, 159, 147, 139, 135, 132, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 1152, 749, 505, 356, 267, 212, 179, 159, 147, 139, 135, 132, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 1152, 749, 505, 356, 267, 212, 179, 159, 147, 139, 135, 132, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 1152, 749, 505, 356, 267, 212, 179, 159, 147, 139, 135, 132, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 1152, 749, 505, 356, 267, 212, 179, 159, 147, 139, 135, 132, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 1152, 749, 505, 356, 267, 212, 179, 159, 147, 139, 135, 132, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 1152, 749, 505, 356, 267, 212, 179, 159, 147, 139, 135, 132, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 1152, 749, 505, 356, 267, 212, 179, 159, 147, 139, 135, 132, 131, 130, 129, 129, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, };

// boundary
int y_1, y_2;
int file = 0;

// temp var
int reverse = 1;
int left_diff = 31, right_diff = 31;
unsigned short param_a, param_b, param_c;
unsigned short _c_a_cs;
unsigned short b_c_a_cs;
unsigned int _f_a_fs_fs;
unsigned int f_ws_fs;
int count_h = 0;

// buffer
unsigned char* guide_img_curr_row_buffer;
unsigned char* guide_img_next_row_buffer;
unsigned char* diff_bottom_buffer;
unsigned char* smooth_img_curr_row_buffer;
unsigned char* smooth_img_prev_row_buffer;
unsigned char* is_continuous;
unsigned char* count_v;
unsigned short* middle_c_h_buffer;
unsigned short* middle_f_h_buffer;
unsigned short* middle_c_v_buffer;
unsigned short* middle_f_v_buffer;
unsigned short* middle_u_v_buffer;

bm_frame* middle_c_v_frame;
bm_frame* middle_f_v_frame;
bm_frame* middle_u_v_frame;

// COMMON Functions
bm_status_t bm_frame_create(bm_frame *frame,
                            frame_type type,
                            bm_image_format_ext format,
                            int width,
                            int height,
                            int channel,
                            int depth)
{
    bm_status_t ret = BM_SUCCESS;
    if (width <= 0 || height <= 0 || channel <= 0 || depth <= 0)
    {
        printf("bm_frame_create parameters error \n");
        ret = BM_ERR_PARAM;
    }

    frame->type = type;
    frame->format = format;
    frame->width = width;
    frame->height = height;
    frame->channel = channel;
    frame->depth = depth;

    frame->data = (int *)malloc(width * height * channel * sizeof(int));
    if (frame->data == NULL)
    {
        printf("bm_frame_create memory allocation failed.\n");
        ret = BM_ERR_DATA;
    }
    memset(frame->data, 0, width * height * channel * sizeof(int));

    return ret;
}

bm_status_t bm_frame_copy_data(bm_frame *frame,
                               frame_type type,
                               bm_image_format_ext format,
                               int width,
                               int height,
                               int channel,
                               int depth,
                               unsigned char * input_img)
{
    bm_status_t ret = bm_frame_create(frame, type, format, width, height, channel, depth);
    if (ret != BM_SUCCESS)
    {
        printf("bm_frame_create in bm_frame_copy_data failed \n");
        return ret;
    }

    for (int i = 0; i < width * height * channel; i++) {
        (frame->data)[i] = (int)input_img[i];
    }
    return ret;
}

int bm_frame_destroy(bm_frame *frame)
{
    if (frame->data != NULL) {
        free(frame->data);
        frame->data = NULL;
    }
    return 0;
}

int get_bfw_size(bmcv_dpu_bfw_mode bfw)
{
    int bfw_size = 0;
    switch (bfw){
        case DPU_BFW_MODE_DEFAULT:
        case DPU_BFW_MODE_7x7:
            bfw_size = 7;
            break;
        case DPU_BFW_MODE_1x1:
            bfw_size = 1;
            break;
        case DPU_BFW_MODE_3x3:
            bfw_size = 3;
            break;
        case DPU_BFW_MODE_5x5:
            bfw_size = 5;
            break;
        default:
            printf("BFW_MODE NOT SUPPORT! \n");
            break;
            return -1;
    }
    return bfw_size;
}

int get_disp_range(bmcv_dpu_disp_range disp_range)
{
    int dispRange = 0;
    switch (disp_range){
        case DPU_DISP_RANGE_DEFAULT:
        case DPU_DISP_RANGE_16:
            dispRange = 0;
            break;
        case DPU_DISP_RANGE_32:
            dispRange = 1;
            break;
        case DPU_DISP_RANGE_48:
            dispRange = 2;
            break;
        case DPU_DISP_RANGE_64:
            dispRange = 3;
            break;
        case DPU_DISP_RANGE_80:
            dispRange = 4;
            break;
        case DPU_DISP_RANGE_96:
            dispRange = 5;
            break;
        case DPU_DISP_RANGE_112:
            dispRange = 6;
            break;
        case DPU_DISP_RANGE_128:
            dispRange = 7;
            break;
        default:
            printf("DISP_RANGE NOT SUPPORT \n");
            break;
            return -1;
    }
    return dispRange;
}

int get_depth_unit(bmcv_dpu_depth_unit dpu_depth_unit)
{
    int depthUnit = 0;
    switch(dpu_depth_unit){
        case DPU_DEPTH_UNIT_DEFAULT:
        case DPU_DEPTH_UNIT_MM:
            dpu_depth_unit = 0;
            break;
        case DPU_DEPTH_UNIT_CM:
            dpu_depth_unit = 1;
            break;
        case DPU_DEPTH_UNIT_DM:
            dpu_depth_unit = 2;
            break;
        case DPU_DEPTH_UNIT_M:
            dpu_depth_unit = 3;
            break;
        default:
            printf("DEPTH_UNIT NOT SUPPORT \n");
            break;
            return -1;
    }
    return depthUnit;
}

int get_dcc_dir(bmcv_dpu_dcc_dir dcc_dir)
{
    int dccDir = 0;
    switch(dcc_dir){
        case DPU_DCC_DIR_DEFAULT:
        case DPU_DCC_DIR_A12:
            dccDir = 0;
            break;
        case DPU_DCC_DIR_A13:
            dccDir = 1;
            break;
        case DPU_DCC_DIR_A14:
            dccDir = 2;
            break;
        default:
            printf("DCC_DIR NOT SUPPORT \n");
            break;
            return -1;
    }
    return dccDir;
}

// local functions
bm_status_t cmodel_dpu_add(bm_frame *src1,
                           bm_frame *src2,
                           bm_frame *dst,
                           bmcv_dpu_sgbm_attrs *grp)
{
    bm_status_t ret = BM_SUCCESS;
    // registers
    int cmodel_dpu_add_rshift1 = grp->dpu_rshift1;
    int cmodel_dpu_add_rshift2 = grp->dpu_rshift2;

    int width = src1->width;
    int height = src1->height;
    int channel = src1->channel;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int d = 0; d < channel; d++) {
                PIXEL(dst, x, y, d) = (PIXEL(src1, x, y, d) >> cmodel_dpu_add_rshift1) +
                                      (PIXEL(src2, x, y, d) >> cmodel_dpu_add_rshift2);
            }
        }
    }
    return ret;
}

bm_status_t cmodel_dpu_boxfilter(bm_frame *src,
                                 bm_frame *dst,
                                 bmcv_dpu_sgbm_attrs *grp,
                                 int width, int height)
{
    bm_status_t ret = BM_SUCCESS;
    // registers
    int imgWidth = width;
    int imgHeight = height;
    int disp_range = get_disp_range(grp->disp_range_en);
    int D = ((disp_range + 1) << 4);
    int minD = grp->disp_start_pos;
    int maxD = minD + D;

    int bfw = get_bfw_size(grp->bfw_mode_en);   // box filter window size

    if(bfw < 0 || disp_range < 0) {
        return BM_ERR_FAILURE;
    }
    // local parameters
    int SW2 = bfw / 2;
    int SH2 = bfw / 2;
    int channel = maxD - minD;
    int lImg_minX = MAX(maxD, 0);
    int lImg_maxX = imgWidth + MIN(minD, 0);
    int costWidth = lImg_maxX - lImg_minX;
    int costHeight = imgHeight;

    for (int d = 0; d < channel; ++d) {
        for (int out_y = 0; out_y < costHeight; ++out_y) {
            for (int out_x = 0; out_x < costWidth; ++out_x) {
                int sum = 0;

                for (int in_y = out_y - SH2; in_y <= out_y + SH2; ++in_y) {
                    int in_y_ = MAX(MIN(in_y, costHeight - 1), 0);
                    for (int in_x = out_x - SW2; in_x <= out_x + SW2; ++in_x) {
                        int in_x_ = MAX(MIN(in_x, costWidth - 1), 0);
                        sum += PIXEL(src, in_x_, in_y_, d);
                    }
                }
                PIXEL(dst, out_x, out_y, d) = MIN(MAX(-32768, sum), 32767);
            }
        }
    }
    return ret;
}

bm_status_t cmodel_dpu_btDistanceCost(bm_frame *pImg1,
                                      bm_frame *pImg2,
                                      bm_frame *dst,
                                      bmcv_dpu_sgbm_attrs *grp,
                                      int width, int height)
{
    bm_status_t ret = BM_SUCCESS;

    int imgWidth = width;
    int imgHeight = height;
    int disp_range = get_disp_range(grp->disp_range_en);
    int D = ((disp_range + 1) << 4);
    int minD = grp->disp_start_pos;
    int maxD = minD + D;

    int lImg_minX = MAX(maxD, 0);
    int lImg_maxX = imgWidth + MIN(minD, 0);
    int rImg_minX = MAX(lImg_minX - maxD, 0);
    int rImg_maxX = MIN(lImg_maxX - minD, imgWidth);

    if(disp_range < 0) {
        return BM_ERR_FAILURE;
    }

    unsigned char *rMin = (unsigned char *)malloc(imgWidth * sizeof(unsigned char));
    unsigned char *rMax = (unsigned char *)malloc(imgWidth * sizeof(unsigned char));

    for (int y = 0; y < imgHeight; y++) {
        for (int x = rImg_minX; x < rImg_maxX; x++) {
            int vl = (PIXEL(pImg2, x, y, 0) + PIXEL(pImg2, MAX(x - 1, 0), y, 0)) / 2;
            int vr = (PIXEL(pImg2, x, y, 0) + PIXEL(pImg2, MIN(x + 1, imgWidth - 1), y, 0)) / 2;

            int v0 = MIN(vl, vr);
            v0 = MIN(v0, PIXEL(pImg2, x, y, 0));
            int v1 = MAX(vl, vr);
            v1 = MAX(v1, PIXEL(pImg2, x, y, 0));
            rMin[x] = (unsigned char)v0;
            rMax[x] = (unsigned char)v1;
        }
        for (int x = lImg_minX; x < lImg_maxX; x++) {
            int ul = (PIXEL(pImg1, x, y, 0) + PIXEL(pImg1, MAX(x - 1, 0), y, 0)) / 2;
            int ur = (PIXEL(pImg1, x, y, 0) + PIXEL(pImg1, MIN(x + 1, imgWidth - 1), y, 0)) / 2;

            int u0 = MIN(ul, ur);
            u0 = MIN(u0, PIXEL(pImg1, x, y, 0));
            int u1 = MAX(ul, ur);
            u1 = MAX(u1, PIXEL(pImg1, x, y, 0));

            int costX = x - lImg_minX;
            for (int d = minD; d < maxD; d++) {
                int v = PIXEL(pImg2, x - d, y, 0);
                int v0 = rMin[x - d];
                int v1 = rMax[x - d];

                int c0 = MAX(0, PIXEL(pImg1, x, y, 0) - v1);
                c0 = MAX(c0, v0 - PIXEL(pImg1, x, y, 0));
                int c1 = MAX(0, v - u1);
                c1 = MAX(c1, u0 - v);

                int costD = d - minD;
                PIXEL(dst, costX, y, costD) = MIN(c0, c1);
            }
        }
    }
    free(rMin);
    free(rMax);
    return ret;
}

bm_status_t cmodel_dpu_DCCAggrX(bm_frame *src,
                                bm_frame *dst,
                                bool reverseX,
                                bmcv_dpu_sgbm_attrs *grp,
                                int width, int height)          // A1
{
    // registers
    int imgWidth = width;
    int imgHeight = height;
    int disp_range = get_disp_range(grp->disp_range_en);
    int D = ((disp_range + 1) << 4);
    int minD = grp->disp_start_pos;
    int p1 = grp->dpu_ca_p1;
    int p2 = grp->dpu_ca_p2;

    // local parameters
    int maxD = minD + D;
    int lImg_minX = MAX(maxD, 0);
    int lImg_maxX = imgWidth + MIN(minD, 0);
    int costWidth = lImg_maxX - lImg_minX;
    int costHeight = imgHeight;

    int x1 = reverseX ? costWidth - 1 : 0;
    int x2 = reverseX ? -1 : costWidth;
    int dx = reverseX ? -1 : 1;

    int *Lr_pad_prev = (int *)malloc((D + 2) * sizeof(int));
    int *Lr_pad_cur  = (int *)malloc((D + 2) * sizeof(int));
    int minLr_cur = 0;

    for (int y = 0; y != costHeight; ++y) {
        set_array_val(Lr_pad_prev, D + 2, 0);
        minLr_cur = 0;
        for (int x = x1; x != x2; x += dx) {
            int minLr_prev = minLr_cur;
            int *Lr_prev = Lr_pad_prev + 1;
            int *Lr_cur = Lr_pad_cur + 1;
            Lr_prev[-1] = Lr_prev[D] = 32767;
            minLr_cur = 32767;
            for (int d = 0; d < D; ++d) {
                int L = PIXEL(src, x, y, d) +
                        MIN4(Lr_prev[d],
                             Lr_prev[d - 1] + p1,
                             Lr_prev[d + 1] + p1,
                             minLr_prev + p2) - minLr_prev;
                L = MINMAX(L, -32768, 32767);
                PIXEL(dst, x, y, d) = MINMAX(PIXEL(dst, x, y, d) + L, -32768, 32767);
                Lr_cur[d] = L;
                minLr_cur = MIN(minLr_cur, L);
            }
            swap(Lr_pad_cur, Lr_pad_prev);
        }
    }
    free(Lr_pad_cur);
    free(Lr_pad_prev);
    return BM_SUCCESS;
}

bm_status_t cmodel_dpu_DCCAggrA234(bm_frame *src,
                                   bm_frame *dst,
                                   int dir,
                                   bmcv_dpu_sgbm_attrs *grp,
                                   int width,
                                   int height)       // A2, A3, A4
{
    // registers
    int imgWidth = width;
    int imgHeight = height;
    int disp_range = get_disp_range(grp->disp_range_en);
    int D = ((disp_range + 1) << 4);
    int minD = grp->disp_start_pos;
    int p1 = grp->dpu_ca_p1;
    int p2 = grp->dpu_ca_p2;

    // local parameters
    int maxD = minD + D;
    int lImg_minX = MAX(maxD, 0);
    int lImg_maxX = imgWidth + MIN(minD, 0);
    int costWidth = lImg_maxX - lImg_minX;
    int costHeight = imgHeight;

    // A2:x - 1, A3:x, A4:x + 1
    // Adir:x + dx
    int dx = dir - 3;
    int *Lr_pad_prev_line    = (int *)malloc((costWidth + 2) * (D + 2) * sizeof(int));
    int *Lr_pad_cur_line     = (int *)malloc((costWidth + 2) * (D + 2) * sizeof(int));
    int *minLr_pad_prev_line = (int *)malloc((costWidth + 2) * sizeof(int));
    int *minLr_pad_cur_line  = (int *)malloc((costWidth + 2) * sizeof(int));

    set_array_val(Lr_pad_prev_line, (costWidth + 2) * (D + 2), 0);
    set_array_val(Lr_pad_cur_line, (costWidth + 2) * (D + 2), 0);
    set_array_val(minLr_pad_prev_line, costWidth + 2, 0);
    set_array_val(minLr_pad_cur_line, costWidth + 2, 0);

    for (int y = 0; y != costHeight; ++y) {
        for (int x = 0; x != costWidth; ++x) {
            int minLr_prev = minLr_pad_prev_line[x + dx +1];
            int *Lr_prev = GET_LR_PTR_FROM_PAD(Lr_pad_prev_line, x + dx, D + 2);
            int *Lr_cur = GET_LR_PTR_FROM_PAD(Lr_pad_cur_line, x, D + 2);
            int minLr = 32767;
            Lr_prev[-1] = Lr_prev[D] = 32767;

            for (int d = 0; d < D; ++d) {
                int L = PIXEL(src, x, y, d) +
                        MIN4(Lr_prev[d],
                             Lr_prev[d - 1] + p1,
                             Lr_prev[d + 1] + p1,
                             minLr_prev + p2) - minLr_prev;
                L = MINMAX(L, -32768, 32767);
                PIXEL(dst, x, y, d) = MINMAX(PIXEL(dst, x, y, d) + L, -32768, 32767);
                Lr_cur[d] = L;
                minLr = MIN(minLr, L);
            }
            minLr_pad_cur_line[x + 1] = minLr;
        }
        swap(Lr_pad_cur_line, Lr_pad_prev_line);
        swap(minLr_pad_cur_line, minLr_pad_prev_line);
    }
    free(Lr_pad_cur_line);
    free(Lr_pad_prev_line);
    free(minLr_pad_prev_line);
    free(minLr_pad_cur_line);
    return BM_SUCCESS;
}

bm_status_t cmodel_dpu_dccPair(bm_frame *src,
                               bm_frame *dst,
                               bmcv_dpu_sgbm_attrs *grp,
                               int width,
                               int height)
{
    bm_status_t ret = BM_SUCCESS;
    int dcc_pair_num = get_dcc_dir(grp->dcc_dir_en);
    if(dcc_pair_num < 0) {
        return BM_ERR_FAILURE;
    }

    switch (dcc_pair_num)
    {
        case 0:
        {
            cmodel_dpu_DCCAggrX(src, dst, false, grp, width, height);  // A1
            cmodel_dpu_DCCAggrA234(src, dst, 2, grp, width, height);   // A2
        }break;
        case 1:
        {
            cmodel_dpu_DCCAggrX(src, dst, false, grp, width, height);  // A1
            cmodel_dpu_DCCAggrA234(src, dst, 3, grp, width, height);   // A3
        }break;
        case 2:
        {
            cmodel_dpu_DCCAggrX(src, dst, false, grp, width, height);  // A1
            cmodel_dpu_DCCAggrA234(src, dst, 4, grp, width, height);   // A4
        }break;
        default:
        {
            cmodel_dpu_DCCAggrX(src, dst, false, grp, width, height);  // A1
            cmodel_dpu_DCCAggrA234(src, dst, 2, grp, width, height);   // A2
        }
            break;
    }
    return ret;
}

bm_status_t cmodel_dpu_wta(bm_frame *src_cost,
                           bm_frame *dst_disp,
                           bmcv_dpu_sgbm_attrs *grp,
                           int width,
                           int height)
{
    bm_status_t ret = BM_SUCCESS;
    // registers
    int imgWidth = width;
    int imgHeight = height;
    int disp_range = get_disp_range(grp->disp_range_en);
    int D = ((disp_range + 1) << 4);
    int minD = grp->disp_start_pos;

    // local parameters
    int maxD = minD + D;
    int INVALID_DISP = 0;
    int lImg_minX = MAX(maxD, 0);
    int minX1 = lImg_minX;

    int cost_d = 0, minCost = 0;
    int min_d = INVALID_DISP;

    if(disp_range < 0) {
        return BM_ERR_FAILURE;
    }

    for (int y = 0; y < imgHeight; y++)
    {
        for (int x1 = 0; x1 < imgWidth; x1++)
        {
            int x = x1 - minX1;
            if (x >= 0)
            {
                for (int d = 0; d < D; d++)
                {
                    if (d == 0)
                    {
                        cost_d = PIXEL(src_cost, x, y, d);
                        minCost = cost_d;
                        min_d = 0 + minD;
                    } else {
                        cost_d = PIXEL(src_cost, x, y, d);
                        if (cost_d <= minCost) {
                            minCost = cost_d;
                            min_d = d + minD;
                        }
                    }
                }
            } else {
                min_d = INVALID_DISP;
            }
            PIXEL(dst_disp, x + minX1, y, 0) = min_d;
        }
    }
    return ret;
}

bm_status_t cmodel_dpu_UniquenessCheck(bm_frame *src_cost,
                                       bm_frame *src_disp,
                                       bm_frame *dst_disp,
                                       bmcv_dpu_sgbm_attrs *grp,
                                       int width,
                                       int height)
{
    // registers
    int imgWidth = width;
    int imgHeight = height;
    int disp_range = get_disp_range(grp->disp_range_en);
    int D = ((disp_range + 1) << 4);
    int minD = grp->disp_start_pos;
    int uniq_ratio = grp->dpu_uniq_ratio;

    // local parameters
    int maxD = minD + D;
    int INVALID_DISP = 0;
    int lImg_minX = MAX(maxD, 0);
    int lImg_maxX = imgWidth + MIN(minD, 0);

    int bestDisp, refined_disp;
    int costX, costY;
    int pCost_d, pCost_best_d;

    if(disp_range < 0) {
        return BM_ERR_FAILURE;
    }

    for (int y = 0; y < imgHeight; y++)
    {
        for (int x = 0; x < imgWidth; x++)
        {
            refined_disp = PIXEL(src_disp, x, y, 0);
            if (x >= lImg_minX && x < lImg_maxX)
            {
                bestDisp = PIXEL(src_disp, x, y, 0) - minD;
                costX = x - lImg_minX;
                costY = y;

                pCost_best_d = PIXEL(src_cost, costX, costY, bestDisp);
                for (int d = 0; d < D; d++)
                {
                    pCost_d = PIXEL(src_cost, costX, costY, d);
                    if ((pCost_d * (100 - uniq_ratio) < pCost_best_d * 100) && (ABS(bestDisp - d) > 1))
                    {
                        refined_disp = INVALID_DISP;
                        break;
                    }
                }
            }
            PIXEL(dst_disp, x, y, 0) = refined_disp;
        }
    }
    return BM_SUCCESS;
}

bm_status_t cmodel_dpu_lrCheck(bm_frame *src_cost,
                               bm_frame *src_disp,
                               bm_frame *dst_disp,
                               bmcv_dpu_sgbm_attrs *grp)
{
    // registers
    int imgWidth = src_cost->width;
    int imgHeight = src_cost->height;
    int disp_range = get_disp_range(grp->disp_range_en);
    int D = ((disp_range + 1) << 4);
    int minD = grp->disp_start_pos;
    int disp_shift = grp->dpu_disp_shift;
    int disp12MaxDiff = 1;

    // local parameters
    int maxD = minD + D;
    int INVALID_DISP = grp->disp_start_pos - 1;
    int lImg_minX = MAX(maxD, 0);

    int minX1 = lImg_minX;
    int dispscale = 1 << disp_shift;
    int INVALID_DISP_SCALED = INVALID_DISP * dispscale;

    int *pDispCost2_h = (int *)malloc(imgWidth * sizeof(int));
    int *pDisp2_h     = (int *)malloc(imgWidth * sizeof(int));

    for (int y = 0; y < imgHeight; y++)
    {
        set_array_val(pDisp2_h, imgWidth, INVALID_DISP_SCALED);
        set_array_val(pDispCost2_h, imgWidth, DPU_MAX_COST);
        for (int x1 = 0; x1 < imgWidth; x1++)
        {
            int x = x1 - minX1;
            if (x >= 0)
            {
                int pSrcDisp = PIXEL(src_disp, x1, y, 0);
                if (pSrcDisp != INVALID_DISP_SCALED)
                {
                    int _d = pSrcDisp >> disp_shift;
                    int _d_offset = _d - minD;
                    int match_x2 = x1 - _d;
                    int pSrcCost = PIXEL(src_cost, x, y, _d_offset);
                    if (pDispCost2_h[match_x2] > pSrcCost)
                    {
                        pDispCost2_h[match_x2] = pSrcCost;
                        pDisp2_h[match_x2] = _d;
                    }
                }
            }
        }
        for (int x1 = 0; x1 < imgWidth; x1++)
        {
            int x = x1 - minX1;
            int pSrcDisp = PIXEL(src_disp, x1, y, 0);
            int dst_disp0 = pSrcDisp;

            if (x >= 0)
            {
                if (pSrcDisp != INVALID_DISP_SCALED)
                {
                    int _d = pSrcDisp >> disp_shift;
                    int d_ = (pSrcDisp + dispscale - 1) >> disp_shift;
                    int _x = x1 - _d;
                    int x_ = x1 - d_;

                    if (0 <= _x && _x < imgWidth &&
                        pDisp2_h[_x] != INVALID_DISP_SCALED &&
                        ABS(pDisp2_h[_x] - _d) > disp12MaxDiff &&
                        0 <= x_ && x_ < imgWidth &&
                        pDisp2_h[x_] != INVALID_DISP_SCALED &&
                        ABS(pDisp2_h[x_] - d_) > disp12MaxDiff)
                    {
                        dst_disp0 = INVALID_DISP_SCALED;
                    }
                }
            }
            PIXEL(dst_disp, x1, y, 0) = dst_disp0;
        }
    }
    free(pDispCost2_h);
    free(pDisp2_h);
    return BM_SUCCESS;
}

bm_status_t cmodel_dpu_median3x3(bm_frame *src_disp,
                                 bm_frame *dst_disp,
                                 int width,
                                 int height)
{
    bm_status_t ret = BM_SUCCESS;
    // registers
    int imgWidth = width;
    int imgHeight = height;

    int data[3][3], min_d[3], med_d[3], max_d[3];
    int px, py;

    for (int y = 0; y < imgHeight; y++)
    {
        for (int x = 0; x < imgWidth; x++)
        {
            for (int j = 0; j < 3; j++)
            {
                for (int i = 0; i < 3; i++)
                {
                    px = MIN(MAX(x + i - 1, 0), imgWidth - 1);
                    py = MIN(MAX(y + j - 1, 0), imgHeight - 1);
                    data[j][i] = PIXEL(src_disp, px, py, 0);
                }
            }
            for (int i = 0; i < 3; i++)
            {
                min_d[i] = MIN(MIN(data[i][0], data[i][1]), data[i][2]);
                med_d[i] = MEDIAN(data[i][0], data[i][1], data[i][2]);
                max_d[i] = MAX(MAX(data[i][0], data[i][1]), data[i][2]);
            }
            int min0 = MAX(MAX(min_d[0], min_d[1]), min_d[2]);
            int med0 = MEDIAN(med_d[0], med_d[1], med_d[2]);
            int max0 = MIN(MIN(max_d[0], max_d[1]), max_d[2]);
            int med1 = MEDIAN(min0, med0, max0);
            PIXEL(dst_disp, x, y, 0) = med1;
        }
    }
    return ret;
}

bm_status_t cmodel_dpu_dispinterp(bm_frame *src_cost,
                                  bm_frame *src_disp,
                                  bm_frame *dst_disp,
                                  bmcv_dpu_sgbm_attrs *grp,
                                  int width, int height)
{
    bm_status_t ret = BM_SUCCESS;
    // registers
    int imgWidth = width;
    int imgHeight = height;
    int disp_shift = grp->dpu_disp_shift;
    int disp_range = get_disp_range(grp->disp_range_en);
    int D = ((disp_range + 1) << 4);
    int minD = grp->disp_start_pos;

    // control parameters
    int maxD = minD + D;
    int INVALID_DISP = 0;
    int lImg_minX = MAX(maxD, 0);
    int lImg_maxX = imgWidth + MIN(minD, 0);
    int minX1 = lImg_minX;
    int dispScale = 1 << disp_shift;
    int INVALID_DISP_SCALED = INVALID_DISP * dispScale;

    int dst_disp0, dst_disp1;
    int costX, costY;

    if(disp_range < 0) {
        return BM_ERR_FAILURE;
    }

    for (int y = 0; y < imgHeight; y++)
    {
        for (int x = 0; x < lImg_maxX; x++)
        {
            if (x >= minX1) dst_disp0 = PIXEL(src_disp, x, y, 0);
            else dst_disp0 = INVALID_DISP;

            if (dst_disp0 == INVALID_DISP) dst_disp1 = INVALID_DISP_SCALED;
            else
            {
                int d = dst_disp0 - minD;
                if (dst_disp0 == minD || dst_disp0 == maxD - 1) d = d * dispScale;
                else
                {
                    costX = x - minX1;
                    costY = y;
                    int pCost_dm1 = PIXEL(src_cost, costX, costY, d - 1);
                    int pCost_d0  = PIXEL(src_cost, costX, costY, d);
                    int pCost_dp1 = PIXEL(src_cost, costX, costY, d + 1);
                    int denom2 = MAX((pCost_dm1 + pCost_dp1 - 2 * pCost_d0), 1);
                    d = d * dispScale + ((pCost_dm1 - pCost_dp1) * dispScale + denom2) / (denom2 * 2);
                }
                dst_disp1 = (d + minD * dispScale);
            }
            PIXEL(dst_disp, x, y, 0) = dst_disp1;
        }
    }
    return ret;
}

bm_status_t cmodel_dpu_census(bm_frame *src_disp,
                              bm_frame *dst_disp,
                              bmcv_dpu_sgbm_attrs *grp,
                              int width,
                              int height)
{
    bm_status_t ret = BM_SUCCESS;
    // registers
    int imgWidth = width;
    int imgHeight = height;
    int kw = 3;
    int kh = 3;
    int CENSUS_FACTOR = grp->dpu_census_shift;
    // local parameters
    int SW2 = kw / 2;
    int SH2 = kh / 2;

    // int data[5][5];
    // __uint8_t gray_center, gray;
    // int px, py;
    unsigned int census_val;
    for (int y = 0; y < imgHeight; y++)
    {
        for (int x = 0; x < imgWidth; x++)
        {
            PIXEL(dst_disp, x, y, 0) = 0;
        }
    }
    for (int y = SH2; y < imgHeight - SH2; y++)
    {
        for (int x = SW2; x < imgWidth - SW2; x++)
        {
            const __uint8_t gray_center = PIXEL(src_disp, x, y, 0);
            census_val = 0u;
            for (int r = -SH2; r <= SH2; r++)
            {
                for (int c = -SW2; c <= SW2; c++)
                {
                    census_val <<= 1;
                    const __uint8_t gray = PIXEL(src_disp, x + c, y + r, 0);
                    if (gray < gray_center) census_val += 1;
                }
            }
            PIXEL(dst_disp, x, y, 0) = MIN((census_val >> CENSUS_FACTOR), 255);
        }
    }
    return ret;
}

bm_status_t cmodel_dpu_dispU16toU8(bm_frame *src,
                                   bm_frame *dst,
                                   bmcv_dpu_sgbm_attrs *grp,
                                   int width,
                                   int height)
{
    bm_status_t ret = BM_SUCCESS;
    // registers
    int imgWidth = width;
    int imgHeight = height;
    // int disp_shift_bit = grp->dpu_disp_shift;
    int dispScale = 1 << grp->dpu_disp_shift;
    int disp_range = get_disp_range(grp->disp_range_en);
    // int D = ((disp_range + 1) << 4);
    int INVALID_DISP = 0;      // minD - 1

    // control parameters
    int drxpow2ds = (disp_range + 1) * dispScale;
    int INVALID_DISP_SCALED = INVALID_DISP * dispScale;

    if(disp_range < 0) {
        return BM_ERR_FAILURE;
    }

    for (int y = 0; y < imgHeight; y++)
    {
        for (int x = 0; x < imgWidth; x++)
        {
            if (PIXEL(src, x, y, 0) == INVALID_DISP_SCALED) PIXEL(dst, x, y, 0) = INVALID_DISP;
            else {
                int dispU8 = MIN(((PIXEL(src, x, y, 0) << 4) / drxpow2ds), 255);
                PIXEL(dst, x, y, 0) = dispU8;
            }
        }
    }
    return ret;
}

bm_status_t cmodel_dpu_dispU8toDepthU16(bm_frame *dispData,
                                        bm_frame *depthData,
                                        bmcv_dpu_disp_range dispRange,
                                        bmcv_dpu_fgs_attrs *fgs_grp,
                                        int width,
                                        int height)
{
    bm_status_t ret = BM_SUCCESS;
    // registers
    int imgWidth  = width;
    int imgHeight = height;
    int fxb       = fgs_grp->fxbase_line;

    int output_unit_choose = get_depth_unit(fgs_grp->depth_unit_en);
    output_unit_choose = (int)pow(10, output_unit_choose);
    int INVALID_DISP = 0;
    int nd = get_disp_range(dispRange) + 1;

    if(output_unit_choose < 0 || nd < 0) {
        return BM_ERR_FAILURE;
    }

    for (int y = 0; y < imgHeight; y++)
    {
        for (int x = 0; x < imgWidth; x++)
        {
            PIXEL(depthData, x, y, 0) = 0;
            if (!PIXEL(dispData, x, y, 0) || PIXEL(dispData, x, y, 0) == INVALID_DISP) {
                PIXEL(depthData, x, y, 0) = INVALID_DISP;
            } else {
                PIXEL(depthData, x, y, 0) = MIN(((nd * fxb) / (PIXEL(dispData, x, y, 0) << 4)), 256 * 128 - 1) / output_unit_choose;
            }
        }
    }
    return ret;
}

void read_row_uint8(bm_frame* src, unsigned char* dest, int y)
{
    for (int x = 0; x < width; x++)
    {
        dest[x] = (unsigned char)PIXEL(src, x, y, 0);
    }
}

void write_row_uint8(unsigned char* src, bm_frame* dest, int y)
{
    for (int x = 0; x < width; x++)
    {
        PIXEL(dest, x, y, 0) = src[x];
    }
}

void read_row_uint16(bm_frame* src, unsigned short* dest, int y)
{
    for (int x = 0; x < width; x++)
    {
        dest[x] = (unsigned short)PIXEL(src, x, y, 0);
    }
}

void write_row_uint16(unsigned short* src, bm_frame* dest, int y)
{
    for (int x = 0; x < width; x++)
    {
        PIXEL(dest, x, y, 0) = src[x];
    }
}

void calc_left_to_right(int t, int y) {
    for (int x = 0; x < width; x++)
    {
        //  horizontal left -> right
        if (x == 0)
        {
            left_diff = 31;
        } else {
            left_diff = right_diff;
        }
        if (x == width - 1)
        {
            right_diff = 31;
        } else {
            right_diff = abs((int)guide_img_curr_row_buffer[x + 1] - (int)guide_img_curr_row_buffer[x]);
        }
        if (right_diff > 31) right_diff = 31;
        if (x != 0 && guide_img_curr_row_buffer[x - 1] == guide_img_curr_row_buffer[x] &&
            smooth_img_curr_row_buffer[x - 1] == smooth_img_curr_row_buffer[x])
        {
            count_h++;
            if (count_h >= max_count)
            {
                left_diff = 31;
                count_h = 0;
            }
        } else {
            count_h = 0;
        }
        param_a = weights_fx_a_c[t * 32 + left_diff];
        param_b = weights_fx_b[t * 32 * 32 + left_diff * 32 + right_diff];
        param_c = weights_fx_a_c[t * 32 + right_diff];
        if (x == 0)
        {
            _c_a_cs = 0;
            _f_a_fs_fs = 0;
        } else {
            _c_a_cs = (unsigned short)quan_div(middle_c_h_buffer[x - 1] * param_a, c_scale);
            _f_a_fs_fs = middle_f_h_buffer[x - 1] * param_a;
        }
        b_c_a_cs = param_b - _c_a_cs;
        f_ws_fs = smooth_img_curr_row_buffer[x] * w_scale * f_scale;
        middle_c_h_buffer[x] = quan_div(param_c * c_scale, b_c_a_cs);
        middle_f_h_buffer[x] = quan_div(f_ws_fs + _f_a_fs_fs, b_c_a_cs);
    }
}

void calc_right_to_left (int t, int y) {
    for (int x = width - 1; x >= 0; x--)
    {
        if (x < width - 1)
        {
            middle_f_h_buffer[x] = quan_div((middle_f_h_buffer[x] * c_scale + middle_c_h_buffer[x] * middle_f_h_buffer[x + 1]), c_scale);
        }
        smooth_img_curr_row_buffer[x] = (unsigned char)quan_div(middle_f_h_buffer[x], f_scale);     // div
        if (y == y_1)
        {
            left_diff = 31;
        } else {
            left_diff = diff_bottom_buffer[x];
        }
        if (is_continuous[x] == 1 && smooth_img_curr_row_buffer[x] == smooth_img_prev_row_buffer[x])
        {
            count_v[x]++;
            if (count_v[x] >= max_count)
            {
                left_diff = 31;
                count_v[x] = 0;
            }
        } else {
            count_v[x] = 0;
        }

        // calculate curr row & next row
        if (y == y_2 - reverse)
        {
            right_diff = 31;
        } else {
            right_diff = abs((int)guide_img_curr_row_buffer[x] - (int)guide_img_next_row_buffer[x]);
        }
        if (right_diff > 31)
        {
            right_diff = 31;
        }
        diff_bottom_buffer[x] = right_diff;
        param_a = weights_fx_a_c[t * 32 + left_diff];
        param_b = weights_fx_b[t * 32 * 32 + left_diff * 32 + right_diff];
        param_c = weights_fx_a_c[t * 32 + right_diff];

        _c_a_cs = quan_div(middle_c_v_buffer[x] * param_a, c_scale);
        b_c_a_cs = param_b - _c_a_cs;
        middle_c_v_buffer[x] = quan_div(param_c * c_scale, b_c_a_cs);

        _f_a_fs_fs = middle_f_v_buffer[x] * param_a;
        f_ws_fs = smooth_img_curr_row_buffer[x] * w_scale * f_scale;
        middle_f_v_buffer[x] = quan_div(f_ws_fs + _f_a_fs_fs, b_c_a_cs);
    }
}

void calc_bottom_to_top(int t, int y) {
    for (int x = 0; x < width; x++)
    {
        middle_u_v_buffer[x] = quan_div(middle_f_v_buffer[x] * c_scale + middle_c_v_buffer[x] * middle_u_v_buffer[x], c_scale);
        smooth_img_curr_row_buffer[x] = quan_div(middle_u_v_buffer[x], f_scale);
    }
}

unsigned int quan_div(unsigned int divisor, unsigned int dividend) {
    if (dividend == 0)
    {
        printf("Divide 0 Error\n");
        exit(-1);
    }
    unsigned int quotient = divisor / dividend;
    unsigned int remainder = divisor % dividend;
    return quotient + remainder / ((dividend + 1) / 2);
}

bm_status_t cmodel_dpu_fgs(bm_frame *smooth_img,
                           bm_frame *guide_img,
                           bmcv_dpu_fgs_attrs *fgs_grp)
{
    bm_status_t ret = BM_SUCCESS;
    // get reg value
    width = smooth_img->width;
    height = smooth_img->height;
    T = fgs_grp->fgs_max_t;
    max_count = fgs_grp->fgs_max_count;
    // f_scale = dpu_reg.hw.reg_dpu_fgs_f_scale;   // ini read is 0
    f_scale = 6;
    f_scale = pow(2, f_scale);
    // c_scale = dpu_reg.hw.reg_dpu_fgs_c_scale;   // ini read is 0
    c_scale = 11;
    c_scale = pow(2, c_scale);
    // w_scale = dpu_reg.hw.reg_dpu_fgs_w_scale;   // ini read is 0
    w_scale = 7;
    w_scale = pow(2, w_scale);

    guide_img_curr_row_buffer = (unsigned char*)malloc(width * sizeof(unsigned char));  // 8-bit
    guide_img_next_row_buffer = (unsigned char*)malloc(width * sizeof(unsigned char));  // 8-bit
    diff_bottom_buffer = (unsigned char*)malloc(width * sizeof(unsigned char));         // 5-bit
    smooth_img_curr_row_buffer = (unsigned char*)malloc(width * sizeof(unsigned char)); // 8-bit
    smooth_img_prev_row_buffer = (unsigned char*)malloc(width * sizeof(unsigned char)); // 8-bit
    is_continuous = (unsigned char*)malloc(width * sizeof(unsigned char));              // 5-bit
    count_v = (unsigned char*)malloc(width * sizeof(unsigned char));                    // 5-bit
    middle_c_h_buffer = (unsigned short*)malloc(width * sizeof(unsigned short));        // 11-bit
    middle_f_h_buffer = (unsigned short*)malloc(width * sizeof(unsigned short));        // 14-bit
    middle_c_v_buffer = (unsigned short*)malloc(width * sizeof(unsigned short));        // 11-bit
    middle_f_v_buffer = (unsigned short*)malloc(width * sizeof(unsigned short));        // 14-bit
    middle_u_v_buffer = (unsigned short*)malloc(width * sizeof(unsigned short));        // 14-bit

    bm_frame *middle_c_v_frame = malloc(sizeof(bm_frame));
    bm_frame *middle_f_v_frame = malloc(sizeof(bm_frame));
    bm_frame *middle_u_v_frame = malloc(sizeof(bm_frame));
    bm_frame *revu_frame = malloc(sizeof(bm_frame));

    bm_status_t ret1 = bm_frame_create(middle_c_v_frame, smooth_img->type, FORMAT_GRAY, width, height, 1, (16));
    bm_status_t ret2 = bm_frame_create(middle_f_v_frame, smooth_img->type, FORMAT_GRAY, width, height, 1, (16));
    bm_status_t ret3 = bm_frame_create(middle_u_v_frame, smooth_img->type, FORMAT_GRAY, width, height, 1, (16));
    bm_status_t ret4 = bm_frame_create(revu_frame, smooth_img->type, FORMAT_GRAY, width, height, 1, (8));
    if (ret1 != BM_SUCCESS || ret2 != BM_SUCCESS || ret3 != BM_SUCCESS || ret4 != BM_SUCCESS)
    {
        printf("Create FGS middle frame failed\n");
        ret = BM_ERR_DATA;
        goto fail;
    }

    // init
    reverse = 1;
    for (int x = 0; x < width; x++)
    {
        middle_c_v_buffer[x] = 0;
        middle_f_v_buffer[x] = 0;
        count_v[x] = 0;
        guide_img_curr_row_buffer[x] = 0;
        guide_img_next_row_buffer[x] = 0;
        diff_bottom_buffer[x] = 31;
        smooth_img_curr_row_buffer[x] = 0;
        smooth_img_prev_row_buffer[x] = 0;
        is_continuous[x] = 0;
        count_v[x] = 0;
        middle_c_h_buffer[x] = 0;
        middle_f_h_buffer[x] = 0;
        middle_c_v_buffer[x] = 0;
        middle_f_v_buffer[x] = 0;
        middle_u_v_buffer[x] = 0;
    }
    if (reverse == 1)
    {
        y_1 = 0;
        y_2 = height;
    } else if (reverse == -1)
    {
        y_1 = height - 1;
        y_2 = -1;
    }
    for (int t = 0; t < T; t++)
    {
        file = t;
        int t_true;
        if (t == 0) t_true = 0;
        else if (t == T - 1) t_true = 2;
        else t_true = 1;
        // printf("FGS Load weights[%d]\n", t_true);

        // read first guide image row g(x)_0
        read_row_uint8(guide_img, guide_img_curr_row_buffer, y_1);
        for (int y = y_1; y != y_2; y += reverse)
        {
            // 1. read smooth image row
            read_row_uint8(smooth_img, smooth_img_curr_row_buffer, y);
            // 2. calculate from left to right(c_h, f_h)
            calc_left_to_right(t_true, y);
            // 3. read next guide image row
            if (y != y_2 - reverse)
            {
                for (int x = 0; x < width; x++)
                {
                    if (y != y_1 && guide_img_curr_row_buffer[x] == guide_img_next_row_buffer[x])
                    {
                        is_continuous[x] = 1;
                    } else {
                        is_continuous[x] = 0;
                    }
                }
                read_row_uint8(guide_img, guide_img_next_row_buffer, y + reverse);
            }
            // 4. calculate from right to left(c_v, u_h, f_v)
            calc_right_to_left(t_true, y);
            // 5. save c_v, f_v
            write_row_uint16(middle_c_v_buffer, middle_c_v_frame, y);
            write_row_uint16(middle_f_v_buffer, middle_f_v_frame, y);
            write_row_uint8(smooth_img_curr_row_buffer, smooth_img, y);

            // Switch guide image pointer
            unsigned char* img_temp_pointer;
            img_temp_pointer = guide_img_curr_row_buffer;
            guide_img_curr_row_buffer = guide_img_next_row_buffer;
            guide_img_next_row_buffer = img_temp_pointer;

            img_temp_pointer = smooth_img_curr_row_buffer;
            smooth_img_curr_row_buffer = smooth_img_prev_row_buffer;
            smooth_img_prev_row_buffer = img_temp_pointer;
        }
        // upside down
        reverse *= -1;
        if (reverse == 1)
        {
            y_1 = 0;
            y_2 = height;
        } else {
            y_1 = height - 1;
            y_2 = -1;
        }
        for (int y = y_1; y != y_2; y += reverse)
        {
            read_row_uint16(middle_c_v_frame, middle_c_v_buffer, y);
            read_row_uint16(middle_f_v_frame, middle_f_v_buffer, y);
            calc_bottom_to_top(t_true, y);
            write_row_uint8(smooth_img_curr_row_buffer, smooth_img, y);
            write_row_uint8(smooth_img_curr_row_buffer, revu_frame, height - y - 1);
        }
    }

fail:
    free(guide_img_curr_row_buffer);
    free(guide_img_next_row_buffer);
    free(diff_bottom_buffer);
    free(smooth_img_curr_row_buffer);
    free(smooth_img_prev_row_buffer);
    free(is_continuous);
    free(count_v);
    free(middle_c_h_buffer);
    free(middle_f_h_buffer);
    free(middle_c_v_buffer);
    free(middle_f_v_buffer);
    free(middle_u_v_buffer);

    bm_frame_destroy(revu_frame);
    bm_frame_destroy(middle_c_v_frame);
    bm_frame_destroy(middle_f_v_frame);
    bm_frame_destroy(middle_u_v_frame);
    free(revu_frame);
    free(middle_c_v_frame);
    free(middle_f_v_frame);
    free(middle_u_v_frame);

    return ret;
}

// test_cmodel Func
bm_status_t test_cmodel_sgbm(unsigned char *left_input,
                             unsigned char *right_input,
                             unsigned char *ref_output,
                             int width,
                             int height,
                             bmcv_dpu_sgbm_mode sgbm_mode,
                             bmcv_dpu_sgbm_attrs *grp)
{
    bm_status_t ret = BM_SUCCESS;

    bm_frame *left_img = malloc(sizeof(bm_frame));
    bm_frame *right_img = malloc(sizeof(bm_frame));

    bm_status_t ret1 = bm_frame_copy_data(left_img, PROGRESSIVE, FORMAT_GRAY, width, height, 1, 8, left_input);
    bm_status_t ret2 = bm_frame_copy_data(right_img, PROGRESSIVE, FORMAT_GRAY, width, height, 1, 8, right_input);

    if (ret1 != BM_SUCCESS || ret2 != BM_SUCCESS)
    {
        printf("Bm_frame_copy_data failed\n");
        bm_frame_destroy(left_img);
        bm_frame_destroy(right_img);
        free(left_img);
        free(right_img);
        return ret;
    }

    int imgWidth = left_img->width;
    int imgHeight = left_img->height;
    int disp_range = get_disp_range(grp->disp_range_en);
    int D = ((disp_range + 1) << 4);
    int minD = grp->disp_start_pos;
    int maxD = minD + D;
    int lImg_minX = MAX(maxD, 0);
    int lImg_maxX = imgWidth + MIN(minD, 0);
    int cost_width = lImg_maxX - lImg_minX;
    int cost_height = imgHeight;

    bm_frame *pBtCost0 = malloc(sizeof(bm_frame));
    bm_frame *pSobelXL = malloc(sizeof(bm_frame));
    bm_frame *pSobelXR = malloc(sizeof(bm_frame));
    bm_frame *pSobelBtCost = malloc(sizeof(bm_frame));
    bm_frame *pBtCost_sum = malloc(sizeof(bm_frame));
    bm_frame *pSadCost = malloc(sizeof(bm_frame));
    bm_frame *pSpdCost = malloc(sizeof(bm_frame));
    bm_frame *pWTADisp0 = malloc(sizeof(bm_frame));
    bm_frame *pWTADisp1 = malloc(sizeof(bm_frame));
    bm_frame *pDispInterp = malloc(sizeof(bm_frame));
    bm_frame *pMedDisp = malloc(sizeof(bm_frame));

    ret = bm_frame_create(pBtCost0, left_img->type, FORMAT_GRAY, cost_width, cost_height, D, DPU_COST_BITS);
    if(ret != BM_SUCCESS){
        printf("pBtCost0 bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_btDistanceCost(left_img, right_img, pBtCost0, grp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_btDistanceCost failed!\n");
        goto fail;
    }

    ret1 = bm_frame_create(pSobelXL, left_img->type, FORMAT_GRAY, imgWidth, imgHeight, 1, DPU_SOBEL_BITS);
    ret2 = bm_frame_create(pSobelXR, left_img->type, FORMAT_GRAY, imgWidth, imgHeight, 1, DPU_SOBEL_BITS);
    if (ret1 != BM_SUCCESS) {
        printf("pSobelXL bm_image create failed.\n");
        ret = ret1;
        goto fail;
    }
    if (ret2 != BM_SUCCESS) {
        printf("pSobelXR bm_image create failed.\n");
        ret = ret2;
        goto fail;
    }
    ret = cmodel_dpu_census(left_img, pSobelXL, grp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_left_census failed!\n");
        goto fail;
    }
    ret = cmodel_dpu_census(right_img, pSobelXR, grp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_right_census failed!\n");
        goto fail;
    }

    ret = bm_frame_create(pSobelBtCost, left_img->type, FORMAT_GRAY, cost_width, cost_height, D, DPU_COST_BITS);
    if(ret != BM_SUCCESS){
        printf("pSobelBtCost bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_btDistanceCost(pSobelXL, pSobelXR, pSobelBtCost, grp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_btDistanceCost failed!\n");
        goto fail;
    }

    ret = bm_frame_create(pBtCost_sum, left_img->type, FORMAT_GRAY, cost_width, cost_height, D, DPU_COST_BITS);
    if(ret != BM_SUCCESS){
        printf("pBtCost_sum bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_add(pSobelBtCost, pBtCost0, pBtCost_sum, grp);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_add failed!\n");
        goto fail;
    }

    ret = bm_frame_create(pSadCost, left_img->type, FORMAT_GRAY, cost_width, cost_height, D, DPU_COST_BITS);
    if(ret != BM_SUCCESS){
        printf("pSadCost bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_boxfilter(pBtCost_sum, pSadCost, grp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_boxfilter failed!\n");
        goto fail;
    }

    ret = bm_frame_create(pSpdCost, left_img->type, FORMAT_GRAY, cost_width, cost_height, D, DPU_COST_BITS);
    if(ret != BM_SUCCESS){
        printf("pSpdCost bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_dccPair(pSadCost, pSpdCost, grp, width, height);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_dccPair failed!\n");
        goto fail;
    }

    ret = bm_frame_create(pWTADisp0, left_img->type, FORMAT_GRAY, imgWidth, imgHeight, 1, DPU_SOBEL_BITS);
    if(ret != BM_SUCCESS){
        printf("pWTADisp0 bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_wta(pSpdCost, pWTADisp0, grp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_wta failed!\n");
        goto fail;
    }

    ret = bm_frame_create(pWTADisp1, left_img->type, FORMAT_GRAY, imgWidth, imgHeight, 1, DPU_SOBEL_BITS);
    if(ret != BM_SUCCESS){
        printf("pWTADisp1 bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_UniquenessCheck(pSpdCost, pWTADisp0, pWTADisp1, grp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_UniquenessCheck failed!\n");
        goto fail;
    }

    ret = bm_frame_create(pDispInterp, left_img->type, FORMAT_GRAY, imgWidth, imgHeight, 1, DPU_DEPTH_BITS);
    if(ret != BM_SUCCESS){
        printf("pDispInterp bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_dispinterp(pSpdCost, pWTADisp1, pDispInterp, grp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_dispinterp failed!\n");
        goto fail;
    }

#if 0
    printf("============= LRCheck. =============\n");
    bm_frame *pLRCheck = bm_frame_create(left_img->type, FORMAT_GRAY, imgWidth, imgHeight, 1, DPU_DEPTH_BITS);
    if(pLRCheck == NULL){
        printf("pLRCheck bm_image create failed.\n");
        goto fail;
    }
    NOTE:No LRCheck in Cvitek Code.
    cmodel_dpu_lrCheck(pSpdCost, pDispInterp, pLRCheck);
    if(pLRCheck == NULL){
        printf("LRCheck failed.\n");
        goto fail;
    }
#endif

    ret = bm_frame_create(pMedDisp, left_img->type, FORMAT_GRAY, imgWidth, imgHeight, 1, DPU_DEPTH_BITS);
    if(ret != BM_SUCCESS){
        printf("pLRCheck bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_median3x3(pDispInterp, pMedDisp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_median3x3 failed!\n");
        goto fail;
    }

    if (sgbm_mode == DPU_SGBM_MUX0)
    {
        for(int y = 0; y < pWTADisp1->height; y++){
            for(int x = 0; x < pWTADisp1->width; x++){
                int iG = PIXEL(pWTADisp1, x, y, 0);
                unsigned char c;
                c = (iG & 0x000000FF) >> 0;
                ref_output[y * pWTADisp1->width + x] = c;
            }
        }
        // printf("Save pWTADisp1->data to ref_output success.\n");
    } else if (sgbm_mode == DPU_SGBM_MUX2)
    {
        // Post-Process + U16toU8
        bm_frame *pU16toU8 = malloc(sizeof(bm_frame));
        ret = bm_frame_create(pU16toU8, left_img->type, FORMAT_GRAY, imgWidth, imgHeight, 1, DPU_SOBEL_BITS);
        if (ret != BM_SUCCESS) {
            printf("pU16toU8 bm_image create failed.\n");
            bm_frame_destroy(pU16toU8);
            free(pU16toU8);
            goto fail;
        }
        ret = cmodel_dpu_dispU16toU8(pMedDisp, pU16toU8, grp, imgWidth, imgHeight);
        if (ret != BM_SUCCESS)
        {
            printf("cmodel_dpu_dispU16toU8 failed!\n");
            bm_frame_destroy(pU16toU8);
            free(pU16toU8);
            goto fail;
        }

        for(int y = 0; y < pU16toU8->height; y++){
            for(int x = 0; x < pU16toU8->width; x++){
                int iG = PIXEL(pU16toU8, x, y, 0);
                unsigned char c;
                c = (iG & 0x000000FF) >> 0;
                ref_output[y * pU16toU8->width + x] = c;
            }
        }
        bm_frame_destroy(pU16toU8);
        free(pU16toU8);
        // printf("Save pU16toU8->data to ref_output success.\n");
    }

fail:
    bm_frame_destroy(left_img);
    bm_frame_destroy(right_img);
    bm_frame_destroy(pBtCost0);
    bm_frame_destroy(pSobelXL);
    bm_frame_destroy(pSobelXR);
    bm_frame_destroy(pSobelBtCost);
    bm_frame_destroy(pBtCost_sum);
    bm_frame_destroy(pSadCost);
    bm_frame_destroy(pSpdCost);
    bm_frame_destroy(pWTADisp0);
    bm_frame_destroy(pWTADisp1);
    bm_frame_destroy(pDispInterp);
    bm_frame_destroy(pMedDisp);
    free(left_img);
    free(right_img);
    free(pBtCost0);
    free(pSobelXL);
    free(pSobelXR);
    free(pSobelBtCost);
    free(pBtCost_sum);
    free(pSadCost);
    free(pSpdCost);
    free(pWTADisp0);
    free(pWTADisp1);
    free(pDispInterp);
    free(pMedDisp);

    return ret;
}

bm_status_t test_cmodel_sgbm_u16(unsigned char *left_input,
                                 unsigned char *right_input,
                                 unsigned short *ref_output,
                                 int width,
                                 int height,
                                 bmcv_dpu_sgbm_mode sgbm_mode,
                                 bmcv_dpu_sgbm_attrs *grp)
{
    bm_status_t ret = BM_SUCCESS;

    bm_frame *left_img = malloc(sizeof(bm_frame));
    bm_frame *right_img = malloc(sizeof(bm_frame));

    bm_status_t ret1 = bm_frame_copy_data(left_img, PROGRESSIVE, FORMAT_GRAY, width, height, 1, 8, left_input);
    bm_status_t ret2 = bm_frame_copy_data(right_img, PROGRESSIVE, FORMAT_GRAY, width, height, 1, 8, right_input);
    if (ret1 != BM_SUCCESS || ret2 != BM_SUCCESS)
    {
        printf("Bm_frame_copy_data failed\n");
        bm_frame_destroy(left_img);
        bm_frame_destroy(right_img);
        return ret;
    }

    int imgWidth = left_img->width;
    int imgHeight = left_img->height;
    int disp_range = get_disp_range(grp->disp_range_en);
    int D = ((disp_range + 1) << 4);
    int minD = grp->disp_start_pos;
    int maxD = minD + D;
    int lImg_minX = MAX(maxD, 0);
    int lImg_maxX = imgWidth + MIN(minD, 0);
    int cost_width = lImg_maxX - lImg_minX;
    int cost_height = imgHeight;

    bm_frame *pBtCost0 = malloc(sizeof(bm_frame));
    bm_frame *pSobelXL = malloc(sizeof(bm_frame));
    bm_frame *pSobelXR = malloc(sizeof(bm_frame));
    bm_frame *pSobelBtCost = malloc(sizeof(bm_frame));
    bm_frame *pBtCost_sum = malloc(sizeof(bm_frame));
    bm_frame *pSadCost = malloc(sizeof(bm_frame));
    bm_frame *pSpdCost = malloc(sizeof(bm_frame));
    bm_frame *pWTADisp0 = malloc(sizeof(bm_frame));
    bm_frame *pWTADisp1 = malloc(sizeof(bm_frame));
    bm_frame *pDispInterp = malloc(sizeof(bm_frame));
    bm_frame *pMedDisp = malloc(sizeof(bm_frame));

    ret = bm_frame_create(pBtCost0, left_img->type, FORMAT_GRAY, cost_width, cost_height, D, DPU_COST_BITS);
    if(ret != BM_SUCCESS){
        printf("pBtCost0 bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_btDistanceCost(left_img, right_img, pBtCost0, grp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_btDistanceCost failed!\n");
        goto fail;
    }

    ret1 = bm_frame_create(pSobelXL, left_img->type, FORMAT_GRAY, imgWidth, imgHeight, 1, DPU_SOBEL_BITS);
    ret2 = bm_frame_create(pSobelXR, left_img->type, FORMAT_GRAY, imgWidth, imgHeight, 1, DPU_SOBEL_BITS);
    if (ret1 != BM_SUCCESS) {
        printf("pSobelXL bm_image create failed.\n");
        ret = ret1;
        goto fail;
    }
    if (ret2 != BM_SUCCESS) {
        printf("pSobelXR bm_image create failed.\n");
        ret = ret2;
        goto fail;
    }
    ret = cmodel_dpu_census(left_img, pSobelXL, grp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_census failed!\n");
        goto fail;
    }
    ret = cmodel_dpu_census(right_img, pSobelXR, grp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_census failed!\n");
        goto fail;
    }

    ret = bm_frame_create(pSobelBtCost, left_img->type, FORMAT_GRAY, cost_width, cost_height, D, DPU_COST_BITS);
    if(ret != BM_SUCCESS){
        printf("pSobelBtCost bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_btDistanceCost(pSobelXL, pSobelXR, pSobelBtCost, grp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_btDistanceCost failed!\n");
        goto fail;
    }

    ret = bm_frame_create(pBtCost_sum, left_img->type, FORMAT_GRAY, cost_width, cost_height, D, DPU_COST_BITS);
    if(ret != BM_SUCCESS){
        printf("pBtCost_sum bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_add(pSobelBtCost, pBtCost0, pBtCost_sum, grp);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_add failed!\n");
        goto fail;
    }

    ret = bm_frame_create(pSadCost, left_img->type, FORMAT_GRAY, cost_width, cost_height, D, DPU_COST_BITS);
    if(ret != BM_SUCCESS){
        printf("pSadCost bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_boxfilter(pBtCost_sum, pSadCost, grp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_boxfilter failed!\n");
        goto fail;
    }

    ret = bm_frame_create(pSpdCost, left_img->type, FORMAT_GRAY, cost_width, cost_height, D, DPU_COST_BITS);
    if(ret != BM_SUCCESS){
        printf("pSpdCost bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_dccPair(pSadCost, pSpdCost, grp, width, height);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_dccPair failed!\n");
        goto fail;
    }

    ret = bm_frame_create(pWTADisp0, left_img->type, FORMAT_GRAY, imgWidth, imgHeight, 1, DPU_SOBEL_BITS);
    if(ret != BM_SUCCESS){
        printf("pWTADisp0 bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_wta(pSpdCost, pWTADisp0, grp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_wta failed!\n");
        goto fail;
    }

    ret = bm_frame_create(pWTADisp1, left_img->type, FORMAT_GRAY, imgWidth, imgHeight, 1, DPU_SOBEL_BITS);
    if(ret != BM_SUCCESS){
        printf("pWTADisp1 bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_UniquenessCheck(pSpdCost, pWTADisp0, pWTADisp1, grp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_UniquenessCheck failed!\n");
        goto fail;
    }

    ret = bm_frame_create(pDispInterp, left_img->type, FORMAT_GRAY, imgWidth, imgHeight, 1, DPU_DEPTH_BITS);
    if(ret != BM_SUCCESS){
        printf("pDispInterp bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_dispinterp(pSpdCost, pWTADisp1, pDispInterp, grp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_dispinterp failed!\n");
        goto fail;
    }

#if 0
    printf("============= LRCheck. =============\n");
    bm_frame *pLRCheck = bm_frame_create(left_img->type, FORMAT_GRAY, imgWidth, imgHeight, 1, DPU_DEPTH_BITS);
    if(pLRCheck == NULL){
        printf("pLRCheck bm_image create failed.\n");
        goto fail;
    }
    NOTE:No LRCheck in Cvitek Code.
    cmodel_dpu_lrCheck(pSpdCost, pDispInterp, pLRCheck);
    if(pLRCheck == NULL){
        printf("LRCheck failed.\n");
        goto fail;
    }
#endif

    ret = bm_frame_create(pMedDisp, left_img->type, FORMAT_GRAY, imgWidth, imgHeight, 1, DPU_DEPTH_BITS);
    if(ret != BM_SUCCESS){
        printf("pLRCheck bm_image create failed.\n");
        goto fail;
    }
    ret = cmodel_dpu_median3x3(pDispInterp, pMedDisp, imgWidth, imgHeight);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_median3x3 failed!\n");
        goto fail;
    }

    if (sgbm_mode == DPU_SGBM_MUX1)
    {
        for(int y = 0; y < pMedDisp->height; y++){
            for(int x = 0; x < pMedDisp->width; x++){
                int iG = PIXEL(pMedDisp, x, y, 0);
                unsigned char high8 = (iG & 0x0000FF00) >> 8;
                unsigned char low8  = (iG & 0x000000FF) >> 0;
                unsigned short c16 = (high8 << 8) | low8;
                ref_output[y * pMedDisp->width + x] = c16;
            }
        }
        // printf("Save pMedDisp->data to ref_output success.\n");
    }

fail:
    bm_frame_destroy(left_img);
    bm_frame_destroy(right_img);
    bm_frame_destroy(pBtCost0);
    bm_frame_destroy(pSobelXL);
    bm_frame_destroy(pSobelXR);
    bm_frame_destroy(pSobelBtCost);
    bm_frame_destroy(pBtCost_sum);
    bm_frame_destroy(pSadCost);
    bm_frame_destroy(pSpdCost);
    bm_frame_destroy(pWTADisp0);
    bm_frame_destroy(pWTADisp1);
    bm_frame_destroy(pDispInterp);
    bm_frame_destroy(pMedDisp);
    free(left_img);
    free(right_img);
    free(pBtCost0);
    free(pSobelXL);
    free(pSobelXR);
    free(pSobelBtCost);
    free(pBtCost_sum);
    free(pSadCost);
    free(pSpdCost);
    free(pWTADisp0);
    free(pWTADisp1);
    free(pDispInterp);
    free(pMedDisp);

    return ret;
}

bm_status_t set_sgbmDefaultParam(bmcv_dpu_sgbm_attrs *grp_){
    grp_->bfw_mode_en = 4;
    grp_->disp_range_en = 1;
    grp_->disp_start_pos = 0;
    grp_->dcc_dir_en = 1;
    grp_->dpu_census_shift = 1;
    grp_->dpu_rshift1 = 3;
    grp_->dpu_rshift2 = 2;
    grp_->dcc_dir_en = 1;
    grp_->dpu_ca_p1 = 1800;
    grp_->dpu_ca_p2 = 14400;
    grp_->dpu_uniq_ratio = 25;
    grp_->dpu_disp_shift = 4;
    return BM_SUCCESS;
}

bm_status_t test_cmodel_fgs(unsigned char *smooth_input,
                            unsigned char *guide_input,
                            unsigned char *ref_output,
                            int width,
                            int height,
                            bmcv_dpu_fgs_mode fgs_mode,
                            bmcv_dpu_fgs_attrs *grp)
{
    bm_status_t ret = BM_SUCCESS;

    bm_frame *smooth_img = malloc(sizeof(bm_frame));
    bm_frame *guide_img = malloc(sizeof(bm_frame));

    bm_status_t ret1 = bm_frame_copy_data(smooth_img, PROGRESSIVE, FORMAT_GRAY, width, height, 1, 8, smooth_input);
    bm_status_t ret2 = bm_frame_copy_data(guide_img,  PROGRESSIVE, FORMAT_GRAY, width, height, 1, 8, guide_input);
    if (ret1 != BM_SUCCESS || ret2 != BM_SUCCESS)
    {
        printf("Bm_frame_copy_data failed\n");
        ret = BM_ERR_DATA;
        goto fail;
    }

    bmcv_dpu_sgbm_attrs cmodel_param;
    set_sgbmDefaultParam(&cmodel_param);

    // FGS only support 8bit input
    // printf("=== Fast Global Smooth. ===\n");
    ret = cmodel_dpu_fgs(smooth_img, guide_img, grp);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_fgs failed!\n");
        goto fail;
    }

    if (fgs_mode == DPU_FGS_MUX0)
    {
        // only fgs, u8 disp out,16 align
        for(int y = 0; y < smooth_img->height; y++){
            for(int x = 0; x < smooth_img->width; x++){
                int iG = PIXEL(smooth_img, x, y, 0);
                unsigned char c;
                c = (iG & 0x000000FF) >> 0;
                ref_output[y * smooth_img->width + x] = c;
            }
        }
        // printf("=== Save smooth_img->data to ref_output success. ===\n");
    }

fail:
    bm_frame_destroy(smooth_img);
    bm_frame_destroy(guide_img);
    free(smooth_img);
    free(guide_img);
    return ret;
}

bm_status_t test_cmodel_fgs_u16(unsigned char *smooth_input,
                                unsigned char *guide_input,
                                unsigned short *ref_output,
                                int width,
                                int height,
                                bmcv_dpu_fgs_mode fgs_mode,
                                bmcv_dpu_fgs_attrs *grp,
                                bmcv_dpu_disp_range disp_range)
{
    bm_status_t ret = BM_SUCCESS;

    bm_frame *smooth_img = malloc(sizeof(bm_frame));
    bm_frame *guide_img = malloc(sizeof(bm_frame));

    bm_status_t ret1 = bm_frame_copy_data(smooth_img, PROGRESSIVE, FORMAT_GRAY, width, height, 1, 8, smooth_input);
    bm_status_t ret2 = bm_frame_copy_data(guide_img,  PROGRESSIVE, FORMAT_GRAY, width, height, 1, 8, guide_input);
    if (ret1 != BM_SUCCESS || ret2 != BM_SUCCESS)
    {
        printf("Bm_frame_copy_data failed\n");
        ret = BM_ERR_DATA;
        goto fail;
    }

    int imgWidth = smooth_img->width;
    int imgHeight = smooth_img->height;

    // FGS only support 8bit input
    // printf("=== Fast Global Smooth. ===\n");
    ret = cmodel_dpu_fgs(smooth_img, guide_img, grp);
    if (ret != BM_SUCCESS)
    {
        printf("cmodel_dpu_fgs failed!\n");
        goto fail;
    }

    if (fgs_mode == DPU_FGS_MUX1)
    {
        //only fgs, u16 depth out,32 align
        bm_frame *pU8toU16 = malloc(sizeof(bm_frame));
        ret = bm_frame_create(pU8toU16, guide_img->type, FORMAT_GRAY, imgWidth, imgHeight, 1, DPU_DEPTH_BITS);
        if (ret != BM_SUCCESS) {
            printf("pU8toU16 bm_image create failed.\n");
            bm_frame_destroy(pU8toU16);
            free(pU8toU16);
            goto fail;
        }

        ret = cmodel_dpu_dispU8toDepthU16(smooth_img, pU8toU16, disp_range, grp, imgWidth, imgHeight);
        if (ret != BM_SUCCESS)
        {
            printf("cmodel_dpu_dispU8toDepthU16 failed!\n");
            bm_frame_destroy(pU8toU16);
            free(pU8toU16);
            goto fail;
        }

        for(int y = 0; y < pU8toU16->height; y++){
            for(int x = 0; x < pU8toU16->width; x++){
                int iG = PIXEL(pU8toU16, x, y, 0);
                unsigned char high8 = (iG & 0x0000FF00) >> 8;
                unsigned char low8  = (iG & 0x000000FF) >> 0;
                unsigned short c16 = (high8 << 8) | low8;
                ref_output[y * pU8toU16->width + x] = c16;
            }
        }
        bm_frame_destroy(pU8toU16);
        free(pU8toU16);
    }

fail:
    bm_frame_destroy(smooth_img);
    bm_frame_destroy(guide_img);
    free(smooth_img);
    free(guide_img);
    return ret;
}

bm_status_t test_cmodel_online(unsigned char *left_input,
                               unsigned char *right_input,
                               unsigned char *ref_output,
                               int width,
                               int height,
                               bmcv_dpu_online_mode online_mode,
                               bmcv_dpu_sgbm_attrs *grp,
                               bmcv_dpu_fgs_attrs *fgs_grp)
{
    bm_status_t ret = BM_SUCCESS;
    // 1. SGBM
    // printf("=== online start stage1——sgbm. ===\n");
    unsigned char *u8_output = malloc(width * height * sizeof(unsigned char));
    ret = test_cmodel_sgbm(left_input, right_input, u8_output, width, height, DPU_SGBM_MUX2, grp);
    if (ret != BM_SUCCESS)
    {
        printf("test_cmodel_sgbm failed!\n");
        goto fail;
    }
    // 2. FGS
    // CMODEL_FGS_MUX0 = 7,              //only fgs, u8 disp out,16 align
    // printf("=== online start stage2——fgs. ===\n");
    ret = test_cmodel_fgs(u8_output, left_input, ref_output, width, height, DPU_FGS_MUX0, fgs_grp);
    if (ret != BM_SUCCESS)
    {
        printf("test_cmodel_fgs failed!\n");
        goto fail;
    }

fail:
    free(u8_output);
    return ret;
}

bm_status_t test_cmodel_online_u16(unsigned char *left_input,
                                   unsigned char *right_input,
                                   unsigned short *ref_output,
                                   int width,
                                   int height,
                                   bmcv_dpu_online_mode online_mode,
                                   bmcv_dpu_sgbm_attrs *grp,
                                   bmcv_dpu_fgs_attrs *fgs_grp)
{
    bm_status_t ret = BM_SUCCESS;
    // 1.SGBM
    // printf("=== online start stage1——sgbm. ===\n");
    unsigned char *u8_output = malloc(width * height * sizeof(unsigned char));
    ret = test_cmodel_sgbm(left_input, right_input, u8_output, width, height, DPU_SGBM_MUX2, grp);
    if (ret != BM_SUCCESS)
    {
        printf("test_cmodel_sgbm failed!\n");
        return ret;
    }

    if (online_mode == DPU_ONLINE_MUX2) {
        // CMODEL_ONLINE_MUX2

        // 1. Create u8_output bm_frame
        bm_frame *u8_frame = malloc(sizeof(bm_frame));
        bm_status_t ret1 = bm_frame_copy_data(u8_frame, PROGRESSIVE, FORMAT_GRAY, width, height, 1, 8, u8_output);
        if (ret1 != BM_SUCCESS)
        {
            printf("Bm_frame_copy_data failed\n");
            bm_frame_destroy(u8_frame);
            free(u8_frame);
            goto fail;
        }
        // 2. U8ToU16
        bm_frame *pU8toU16 = malloc(sizeof(bm_frame));
        ret = bm_frame_create(pU8toU16, u8_frame->type, FORMAT_GRAY, width, height, 1, DPU_DEPTH_BITS);
        if (ret != BM_SUCCESS) {
            printf("pU8toU16 bm_image create failed.\n");
            bm_frame_destroy(pU8toU16);
            bm_frame_destroy(u8_frame);
            free(u8_frame);
            free(pU8toU16);
            goto fail;
        }
        ret = cmodel_dpu_dispU8toDepthU16(u8_frame, pU8toU16, grp->disp_range_en, fgs_grp, width, height);
        if (ret != BM_SUCCESS)
        {
            printf("cmodel_dpu_dispU8toDepthU16 failed!\n");
            bm_frame_destroy(pU8toU16);
            bm_frame_destroy(u8_frame);
            free(u8_frame);
            free(pU8toU16);
            goto fail;
        }
        // 3. save u16 data
        for(int y = 0; y < pU8toU16->height; y++){
            for(int x = 0; x < pU8toU16->width; x++){
                int iG = PIXEL(pU8toU16, x, y, 0);
                unsigned char high8 = (iG & 0x0000FF00) >> 8;
                unsigned char low8  = (iG & 0x000000FF) >> 0;
                unsigned short c16  = (high8 << 8) | low8;
                ref_output[y * pU8toU16->width + x] = c16;
            }
        }
        bm_frame_destroy(pU8toU16);
        bm_frame_destroy(u8_frame);
        free(u8_frame);
        free(pU8toU16);
        // printf("=== Save online_CMODEL_ONLINE_MUX dispU8toDepthU16 data to ref_output success. ===\n");
    } else if (online_mode == DPU_ONLINE_MUX1) {
        // 1. FGS
        // CMODEL_FGS_MUX1 = 8,              //only fgs, u16 depth out,32 align
        // printf("=== online_CMODEL_ONLINE_MUX0 start stage2——fgs. ===\n");
        ret = test_cmodel_fgs_u16(u8_output, left_input, ref_output, width, height, DPU_FGS_MUX1, fgs_grp, grp->disp_range_en);
        if (ret != BM_SUCCESS)
        {
            printf("test_cmodel_fgs_u16 failed!\n");
            goto fail;
        }
    }

fail:
    free(u8_output);
    return ret;
}