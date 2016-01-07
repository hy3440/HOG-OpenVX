#include <VX/vx.h>
#include <VX/vxu.h>

#include <VX/vx_lib_debug.h>
#include <VX/vx_lib_extras.h>
#include <VX/vx_lib_xyz.h>

#include <VX/framework/vx_inlines.c>
#include <VX/framework/vx_internal.h>


#if defined(EXPERIMENTAL_USE_NODE_MEMORY)
#include <VX/vx_khr_node_memory.h>
#endif

#if defined(EXPERIMENTAL_USE_DOT)
#include <VX/vx_khr_dot.h>
#endif

#if defined(EXPERIMENTAL_USE_XML)
#include <VX/vx_khr_xml.h>
#endif

#if defined(EXPERIMENTAL_USE_TARGET)
#include <VX/vx_ext_target.h>
#endif

#if defined(EXPERIMENTAL_USE_VARIANTS)
#include <VX/vx_khr_variants.h>
#endif

#include <VX/vx_helper.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <assert.h>

#define CHECK_ALL_ITEMS(array, iter, status, label) { \
status = VX_SUCCESS; \
for ((iter) = 0; (iter) < dimof(array); (iter)++) { \
if ((array)[(iter)] == 0) { \
printf("Item %u in "#array" is null!\n", (iter)); \
assert((array)[(iter)] != 0); \
status = VX_ERROR_NOT_SUFFICIENT; \
} \
} \
if (status != VX_SUCCESS) { \
goto label; \
} \
}

//using namespace std;
/***************************************************/
/*          Definitions & Globals                  */
/***************************************************/
#define PI 3.14159265
//General
vx_uint32  width = 64, height = 128, bin_number = 9, bin_size = 20, range = 144, cell_size = 8, VECTOR_NUM = 3780, img_size;
vx_int16* magnitude;
vx_float32* theta;
vx_float32* alpha;
vx_int32 c[64][128];
vx_float32** m;
vx_int32** visual;
vx_float64** block_norm_v;
/***************************************************/
/*             Function Declararion                */
/***************************************************/
//General
vx_uint32 XYToIdx_3(vx_uint32 x, vx_uint32 y);
vx_uint32 XYToIdx(vx_uint32 x, vx_uint32 y);
/***************************************************/
/*                   Main Procedure                */
/***************************************************/
int main(int argc, char *argv[])
{
    vx_uint32 img_size = width * height * 3;
    vx_uint32 z;
    FILE *save_vector;
    save_vector = fopen("vector.txt", "wb");
    if (NULL == save_vector)
    {
        printf("Open file failure");
        return 1;
    }
    magnitude = (vx_int16 *)malloc(sizeof(vx_int16)*img_size / 3);
    theta = (vx_float32 *)malloc(sizeof(vx_float32)*img_size / 3);
    alpha = (vx_float32 *)malloc(sizeof(vx_float32)*img_size / 3);
    m = (vx_float32 **)malloc(sizeof(vx_float32*)*((img_size / 3 / cell_size) / cell_size));
    visual = (vx_int32 **)malloc(sizeof(vx_int32*)*((img_size / 3 / cell_size) / cell_size));
    for (z = 0; z <= ((img_size / 3 / cell_size) / cell_size); z++)
        *(visual + z) = (vx_int32 *)malloc(sizeof(vx_int32) * 9);
    
    for (z = 0; z <= ((img_size / 3 / cell_size) / cell_size); z++)
        *(m + z) = (vx_float32 *)malloc(sizeof(vx_float32) * 9);
    block_norm_v = (vx_float64 **)malloc(sizeof(vx_float64*)*((img_size / 3 / cell_size) / cell_size));
    for (z = 0; z <= ((img_size / 3 / cell_size) / cell_size); z++)
        *(block_norm_v + z) = (vx_float64 *)malloc(sizeof(vx_float64) * 36);
    
    
    vx_status status = VX_FAILURE;
    //Create Context
    vx_context context = vxCreateContext();
    if (context)
    {
        vx_uint32 x, y, i = 0;
        vx_int32 j, k;
        
        //Create Image
        vx_image images[] = {
            vxCreateImage(context, width, height, VX_DF_IMAGE_RGB),    /* 0: rgb */
            vxCreateImage(context, width, height, VX_DF_IMAGE_IYUV),   /* 1: yuv */
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8),     /* 2: luma */
            vxCreateImage(context, width, height, VX_DF_IMAGE_S16),    /* 3: grad_x */
            vxCreateImage(context, width, height, VX_DF_IMAGE_S16),    /* 4: grad_y */
            vxCreateImage(context, width, height, VX_DF_IMAGE_S16),    /* 5: mag */
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8),     /* 6: phase */
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8),     /* 7: visualization */
        };
        CHECK_ALL_ITEMS(images, i, status, exit);
        status = vxLoadKernels(context, "openvx-debug");
        if (status == VX_SUCCESS)
        {
            //Create Graph
            vx_graph graph = vxCreateGraph(context);
            if (graph)
            {
                vx_node nodes[] = {
                    vxFReadImageNode(graph, "per_64x128.bmp", images[0]),
                    vxFWriteImageNode(graph, images[0], "test.rgb"),
                    vxColorConvertNode(graph, images[0], images[1]),
                    vxFWriteImageNode(graph, images[1], "yuv_64x128_P420.yuv"),
                    vxChannelExtractNode(graph, images[1], VX_CHANNEL_Y, images[2]),
                    vxFWriteImageNode(graph, images[2], "ychannel_64x128_P400.bw"),
                    vxSobel3x3Node(graph, images[2], images[3], images[4]),
                    vxFWriteImageNode(graph, images[3], "gradx_64x128_P400.bw"),
                    vxFWriteImageNode(graph, images[4], "grady_64x128_P400.bw"),
                    vxMagnitudeNode(graph, images[3], images[4], images[5]),
                    vxFWriteImageNode(graph, images[5], "magnitude_64x128_P400.bw"),
                    vxPhaseNode(graph, images[3], images[4], images[6]),
                    vxFWriteImageNode(graph, images[6], "phase_64x128_P400.bw"),
                    vxFWriteImageNode(graph, images[7], "visualization_64x128_P400.bw"),
                };
                CHECK_ALL_ITEMS(nodes, i, status, exit);
                if (status == VX_SUCCESS)
                {
                    //Verify Graph
                    status = vxVerifyGraph(graph);
                    if (status == VX_SUCCESS)
                    {
                        //Process Graph
                        status = vxProcessGraph(graph);
                    }
                }
                vx_rectangle_t rect = { 0, 0, width, height };
                vx_imagepatch_addressing_t image5_addr;
                vx_imagepatch_addressing_t image6_addr;
                void *base5 = NULL;
                void *base6 = NULL;
                status = vxAccessImagePatch(images[5], &rect, 0, &image5_addr, &base5, VX_READ_AND_WRITE);
                for (y = 0; y < height; y++)
                {
                    for (x = 0; x < width; x++)
                    {
                        vx_int16 *ptr = vxFormatImagePatchAddress2d(base5, x, y, &image5_addr);
                        //printf("mag[%u][%u]=%d\n", x, y, *ptr);
                        magnitude[XYToIdx_3(x, y)] = *ptr;
                        //printf("mag[%u][%u]=%d\n", x, y, magnitude[XYToIdx_3(x, y)]);
                    }
                }
                
                status = vxAccessImagePatch(images[6], &rect, 0, &image6_addr, &base6, VX_READ_AND_WRITE);
                for (y = 0; y < height; y++)
                {
                    for (x = 0; x < width; x++)
                    {
                        vx_uint8 *ptr = vxFormatImagePatchAddress2d(base6, x, y, &image6_addr);
                        //printf("phase[%u][%u]=%d\n", x, y, *ptr);
                        vx_uint8 temp = (*ptr<128) ? *ptr : (*ptr - 128);
                        theta[XYToIdx_3(x, y)] = temp * 2 * PI / 255;
                        //printf("phase[%u][%u]=%f\n", x, y, theta[XYToIdx_3(x, y)]);
                    }
                }
                
                for (x = 0; x < ((img_size / 3 / cell_size) / cell_size); x++)
                    for (y = 0; y < 9; y++)
                        m[x][y] = 0;
                for (x = 0; x < ((img_size / 3 / cell_size) / cell_size); x++)
                    for (y = 0; y < 9; y++)
                        visual[x][y] = 0;
                
                for (x = 0; x < (width / 8 - 1)*(height / 8 - 1); x++)
                    for (y = 0; y < 9; y++)
                        block_norm_v[x][y] = 0;
                
                for (y = 0; y < height; y++)
                {
                    if (((x / cell_size) + (y / cell_size)*(width / 8)) >= (img_size / 3 / cell_size) / cell_size)
                        break;
                    for (x = 0; x < width; x++)
                    {
                        if (x == 0 || x == width - 1 || y == 0 || y == height - 1)
                        {
                            alpha[XYToIdx_3(x, y)] = 0;
                            c[x][y]=0;
                        }
                        else
                        {
                            vx_uint32 n = ((theta[XYToIdx_3(x, y)] / PI * 180) / bin_size);
                            c[x][y]=n;
                            alpha[XYToIdx_3(x, y)] = ((9 * theta[XYToIdx_3(x, y)]) / PI) - n;
                            m[(x / cell_size) + (y / cell_size)*(width / cell_size)][n] += alpha[XYToIdx_3(x, y)] * magnitude[XYToIdx_3(x, y)];
                            
                            
                            if (n == 8)
                                m[(x / cell_size) + (y / cell_size)*(width / cell_size)][n - 1] += (1 - alpha[XYToIdx_3(x, y)]) * magnitude[XYToIdx_3(x, y)];
                            else if (n == 0)
                                m[(x / cell_size) + (y / cell_size)*(width / cell_size)][n + 1] += (1 - alpha[XYToIdx_3(x, y)]) * magnitude[XYToIdx_3(x, y)];
                            else
                            {
                                m[(x / cell_size) + (y / cell_size)*(width / cell_size)][n - 1] += (1 - alpha[XYToIdx_3(x, y)]) * magnitude[XYToIdx_3(x, y)];
                                m[(x / cell_size) + (y / cell_size)*(width / cell_size)][n + 1] += (1 - alpha[XYToIdx_3(x, y)]) * magnitude[XYToIdx_3(x, y)];
                            }
                        
                            //********************Block Normalization********************//
                            if (x%cell_size == cell_size - 1 && y%cell_size == cell_size - 1)
                            {
                                if (x / cell_size > 0 && y / cell_size > 0)
                                {
                                    vx_float64 epsilon = 0.0001;
                                    vx_float64 v22 = 0;
                                    for (j = -1; j <= 0; j++)
                                    {
                                        for (k = -1; k <= 0; k++)
                                        {
                                            for (n = 0; n < 9; n++)
                                            {
                                                v22 += pow(m[(x / cell_size + j) + (y / cell_size + k)*(width / cell_size)][n], 2);
                                            }
                                        }
                                    }
                                    for (j = -1; j <= 0; j++)
                                    {
                                        for (k = -1; k <= 0; k++)
                                        {
                                            vx_int32 cell_num = (k == -1) ? ((j == -1) ? 0 : 9) : ((j == -1) ? 18 : 27);
                                            for (n = 0; n < 9; n++)
                                            {
                                                block_norm_v[(x / cell_size - 1) + (y / cell_size - 1)*(width / cell_size - 1)][n + cell_num] = (double)((m[(x / cell_size + j) + (y / cell_size + k)*(width / cell_size)][n]) / sqrt(v22 + pow(epsilon, 2)));
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
				//********************Visualization********************//
                for(y=0;y<height;y++)
                    for(x=0;x<width;x++)
                    {
                        vx_int32 n=c[x][y];
                        visual[(x / cell_size) + (y / cell_size)*(width / cell_size)][n]++;
                    } 

                vx_rectangle_t rect7 = { 0, 0, width, height };
                vx_imagepatch_addressing_t image7_addr;
                void *base7 = NULL;
                status = vxAccessImagePatch(images[7], &rect7, 0, &image7_addr, &base7, VX_READ_AND_WRITE);
                vx_uint32 startx, starty;
                vx_uint32 max,submax;
                for (starty=0; starty <height-cell_size+1; starty=starty+cell_size)
                    for (startx=0; startx <width-cell_size+1; startx=startx+cell_size)
                    {
                        max=0;
                        for(j = 1; j < bin_number; j++)
                        {
                            if(visual[(startx / cell_size) + (starty / cell_size)*(width / cell_size)][j]>visual[(startx / cell_size) + (starty / cell_size)*(width / cell_size)][max])
                            {
                                max=j;
                            }
                        }
                        
                        visual[(startx / cell_size) + (starty / cell_size)*(width / cell_size)][max] = 0;
                        submax = 0;
                        for (j = 0; j < bin_number; j++)
                        {
                            if (visual[(startx / cell_size) + (starty / cell_size)*(width / cell_size)][j]>visual[(startx / cell_size) + (starty / cell_size)*(width / cell_size)][submax])
                            {
                                submax = j;
                            }
                        }
                        for (y = starty+1; y < starty+7; y++)
                        {
                            for (x = startx+1; x < startx+7; x++)
                            {
                                vx_float32 xx=x-startx-3.5,yy=starty+3.5-y;
                                vx_uint8 *ptr = vxFormatImagePatchAddress2d(base7, x, y, &image7_addr);
                                
                                
                                switch (max) {
                                    case 0:
                                        if ((yy == 0.5&&xx > 0) || (yy == -0.5&&xx < 0)){ *ptr = 255; }
                                        break;
                                    case 1:
                                        if ((xx == 0.5&&yy == 0.5) || (xx == 1.5&&yy == 0.5) || (xx == 2.5&&yy == 1.5) || (xx == 3.5&&yy == 1.5)){ *ptr = 255; }
                                        if ((xx == -0.5&&yy == -0.5) || (xx == -1.5&&yy == -0.5) || (xx == -2.5&&yy == -1.5) || (xx == -3.5&&yy == -1.5)){ *ptr = 255; }
                                        break;
                                    case 2:
                                        if (yy == xx) { *ptr = 255; }
                                        break;
                                    case 3:
                                        if ((yy == 0.5&&xx == 0.5) || (yy == 1.5&&xx == 0.5) || (yy == 2.5&&xx == 1.5) || (yy == 3.5&&xx == 1.5)){ *ptr = 255; }
                                        if ((yy == -0.5&&xx == -0.5) || (yy == -1.5&&xx == -0.5) || (yy == -2.5&&xx == -1.5) || (yy == -3.5&&xx == -1.5)){ *ptr = 255; }
                                        break;
                                    case 4:
                                        if ((xx == 0.5&&yy>0) || (xx == -0.5&&yy < 0)){ *ptr = 255; }
                                        break;
                                    case 5:
                                        if ((yy == 0.5&&xx == -0.5) || (yy == 1.5&&xx == -0.5) || (yy == 2.5&&xx == -1.5) || (yy == 3.5&&xx == -1.5)){ *ptr = 255; }
                                        if ((yy == -0.5&&xx == 0.5) || (yy == -1.5&&xx == 0.5) || (yy == -2.5&&xx == 1.5) || (yy == -3.5&&xx == 1.5)){ *ptr = 255; }
                                        break;
                                    case 6:
                                        if (yy == -xx) { *ptr = 255; }
                                        break;
                                    case 7:
                                        if ((xx == -0.5&&yy == 0.5) || (xx == -1.5&&yy == 0.5) || (xx == -2.5&&yy == 1.5) || (xx == -3.5&&yy == 1.5)){ *ptr = 255; }
                                        if ((xx == 0.5&&yy == -0.5) || (xx == 1.5&&yy == -0.5) || (xx == 2.5&&yy == -1.5) || (xx == 3.5&&yy == -1.5)){ *ptr = 255; }
                                        break;
                                    case 8:
                                        if ((yy == 0.5&&xx < 0) || (yy == -0.5&&xx>0)){ *ptr = 255; }
                                        break;
                                    default:
                                        break;
                                }
                                
                                switch (submax) {
                                    case 0:
                                        if((yy==0.5&&xx>0)||(yy==-0.5&&xx<0)){*ptr=255;}
                                        break;
                                    case 1:
                                        if((xx==0.5&&yy==0.5)||(xx==1.5&&yy==0.5)||(xx==2.5&&yy==1.5)||(xx==3.5&&yy==1.5)){*ptr=255;}
                                        if((xx==-0.5&&yy==-0.5)||(xx==-1.5&&yy==-0.5)||(xx==-2.5&&yy==-1.5)||(xx==-3.5&&yy==-1.5)){*ptr=255;}
                                        break;
                                    case 2:
                                        if (yy==xx) {*ptr=255;}
                                        break;
                                    case 3:
                                        if((yy==0.5&&xx==0.5)||(yy==1.5&&xx==0.5)||(yy==2.5&&xx==1.5)||(yy==3.5&&xx==1.5)){*ptr=255;}
                                        if((yy==-0.5&&xx==-0.5)||(yy==-1.5&&xx==-0.5)||(yy==-2.5&&xx==-1.5)||(yy==-3.5&&xx==-1.5)){*ptr=255;}
                                        break;
                                    case 4:
                                        if((xx==0.5&&yy>0)||(xx==-0.5&&yy<0)){*ptr=255;}
                                        break;
                                    case 5:
                                        if((yy==0.5&&xx==-0.5)||(yy==1.5&&xx==-0.5)||(yy==2.5&&xx==-1.5)||(yy==3.5&&xx==-1.5)){*ptr=255;}
                                        if((yy==-0.5&&xx==0.5)||(yy==-1.5&&xx==0.5)||(yy==-2.5&&xx==1.5)||(yy==-3.5&&xx==1.5)){*ptr=255;}
                                        break;
                                    case 6:
                                        if (yy==-xx) {*ptr=255;}
                                        break;
                                    case 7:
                                        if((xx==-0.5&&yy==0.5)||(xx==-1.5&&yy==0.5)||(xx==-2.5&&yy==1.5)||(xx==-3.5&&yy==1.5)){*ptr=255;}
                                        if((xx==0.5&&yy==-0.5)||(xx==1.5&&yy==-0.5)||(xx==2.5&&yy==-1.5)||(xx==3.5&&yy==-1.5)){*ptr=255;}
                                        break;
                                    case 8:
                                        if((yy==0.5&&xx<0)||(yy==-0.5&&xx>0)){*ptr=255;}
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                    }
                status = vxCommitImagePatch(images[7], &rect7, 0, &image7_addr, base7);
                CHECK_ALL_ITEMS(nodes, i, status, exit);
                if (status == VX_SUCCESS)
                {
                    status = vxVerifyGraph(graph);
                    if (status == VX_SUCCESS)
                    {
                        status = vxProcessGraph(graph);
                    }
                }

                
                vx_uint32 count = 0;
                //fprintf (save_vector, "+1 ");
                for (i = 0; i < (width / 8 - 1)*(height / 8 - 1); i++)
                {
                    for (j = 0; j < 36; j++)
                    {
                        count++;
                        if (count == VECTOR_NUM)
                        {
                            fprintf(save_vector, "%d:%.6f", count, block_norm_v[i][j]);
                        }
                        else
                        {
                            fprintf(save_vector, "%d:%.6f ", count, block_norm_v[i][j]);
                        }
                    }
                }
                fprintf(save_vector, "\n");
                
                free(magnitude);
                free(theta);
                free(alpha);
                
                fclose(save_vector);
                
                for (i = 0; i < dimof(nodes); i++)
                {
                    //Release Node
                    vxReleaseNode(&nodes[i]);
                }
                //Release Graph
                vxReleaseGraph(&graph);
            }
        }
        for (i = 0; i < dimof(images); i++)
        {
            //Release Image
            vxReleaseImage(&images[i]);
        }
    exit:
        vxReleaseContext(&context);
    }
    return status;
}


/***************************************************/
/*              Function Implemantation            */
/***************************************************/
vx_uint32 XYToIdx(vx_uint32 x, vx_uint32 y)
{
    return y * 3 * width + x * 3;
}

vx_uint32 XYToIdx_3(vx_uint32 x, vx_uint32 y)
{
    return y*width + x;
}
