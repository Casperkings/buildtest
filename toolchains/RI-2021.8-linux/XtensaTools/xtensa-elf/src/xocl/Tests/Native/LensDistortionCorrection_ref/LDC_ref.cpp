#include "LDC_parameters.h"
#include "xi_api_ref.h"

void ref_image_transform_420sp(short *input_x, short *input_y,
                               const xi_pTile srcY, const xi_pTile srcUV,
                               xi_pTile dstY, xi_pTile dstUV, int bbx,
                               int bby) {
  int input_width = XI_TILE_GET_WIDTH(srcY);
  int input_height = XI_TILE_GET_HEIGHT(srcY);
  int input_stride = XI_TILE_GET_PITCH(srcY);
  unsigned char *input_image_y = (unsigned char *)XI_TILE_GET_DATA_PTR(srcY);
  unsigned char *input_image_uv = (unsigned char *)XI_TILE_GET_DATA_PTR(srcUV);
  int output_stride = XI_TILE_GET_PITCH(dstY);
  unsigned char *__restrict__ output_image_y =
      (unsigned char *)XI_TILE_GET_DATA_PTR(dstY);
  unsigned char *output_image_uv = (unsigned char *)XI_TILE_GET_DATA_PTR(dstUV);

  short i, j;
  short x, y, x0, x1, y0, y1;
  short x_int, y_int, x_dec, y_dec;
  short pixel_values[4], res0, res1;
  // translate rectangle relative to bounding box to reduce the range of integer
  // part
  input_x[0] -= bbx << PIXEL_Q;
  input_x[1] -= bbx << PIXEL_Q;
  input_x[2] -= bbx << PIXEL_Q;
  input_x[3] -= bbx << PIXEL_Q;
  input_y[0] -= bby << PIXEL_Q;
  input_y[1] -= bby << PIXEL_Q;
  input_y[2] -= bby << PIXEL_Q;
  input_y[3] -= bby << PIXEL_Q;

  // Calculate Y pixel values
  for (i = 0; i < RECT_SIZE; i++) {
    for (j = 0; j < RECT_SIZE; j++) {
      // Linear interpolation of the coordinates
      x0 = ((RECT_SIZE - j) * input_x[0] + j * input_x[1] + RECT_SIZE / 2) /
           RECT_SIZE;
      x1 = ((RECT_SIZE - j) * input_x[2] + j * input_x[3] + RECT_SIZE / 2) /
           RECT_SIZE;
      x = ((RECT_SIZE - i) * x0 + i * x1 + RECT_SIZE / 2) / RECT_SIZE;
      y0 = ((RECT_SIZE - j) * input_y[0] + j * input_y[1] + RECT_SIZE / 2) /
           RECT_SIZE;
      y1 = ((RECT_SIZE - j) * input_y[2] + j * input_y[3] + RECT_SIZE / 2) /
           RECT_SIZE;
      y = ((RECT_SIZE - i) * y0 + i * y1 + RECT_SIZE / 2) / RECT_SIZE;
      // printf("%f, %f, ", (float)x/PIXEL_DEC, (float)y/PIXEL_DEC);

      // Divide the coordinates into integer parts and decimal parts.
      x_int = x >> PIXEL_Q;
      y_int = y >> PIXEL_Q;
      x_dec = x & (PIXEL_DEC - 1);
      y_dec = y & (PIXEL_DEC - 1);

      if (x_int < 0 || y_int < 0 || x_int >= input_width - 1 ||
          y_int >= input_height - 1) {
        // Outside of the input image
        output_image_y[j + i * output_stride] = 0;
      } else {
        // Obtain adjacent pixel values
        pixel_values[0] = input_image_y[(x_int) + (y_int)*input_stride];
        pixel_values[1] = input_image_y[(x_int + 1) + (y_int)*input_stride];
        pixel_values[2] = input_image_y[(x_int) + (y_int + 1) * input_stride];
        pixel_values[3] =
            input_image_y[(x_int + 1) + (y_int + 1) * input_stride];

        // Linear interpolation of the pixel value
        res0 = ((PIXEL_DEC - x_dec) * pixel_values[0] +
                x_dec * pixel_values[1] + PIXEL_DEC / 2) /
               PIXEL_DEC;
        res1 = ((PIXEL_DEC - x_dec) * pixel_values[2] +
                x_dec * pixel_values[3] + PIXEL_DEC / 2) /
               PIXEL_DEC;
        output_image_y[j + i * output_stride] =
            ((PIXEL_DEC - y_dec) * res0 + y_dec * res1 + PIXEL_DEC / 2) /
            PIXEL_DEC;
      }
    }
    // printf("\n\n");
  }

  // Calculate UV pixel values
  for (i = 1; i < RECT_SIZE * 2;
       i += 4) { // 0.5 pixel shift for the difference of sampling positions
    for (j = 0; j < RECT_SIZE; j += 2) {
      // Linear interpolation of the coordinates
      x0 = ((RECT_SIZE - j) * input_x[0] + j * input_x[1] + RECT_SIZE / 2) /
           RECT_SIZE;
      x1 = ((RECT_SIZE - j) * input_x[2] + j * input_x[3] + RECT_SIZE / 2) /
           RECT_SIZE;
      x = ((RECT_SIZE * 2 - i) * x0 + i * x1 + RECT_SIZE) / (RECT_SIZE * 2);
      y0 = ((RECT_SIZE - j) * input_y[0] + j * input_y[1] + RECT_SIZE / 2) /
           RECT_SIZE;
      y1 = ((RECT_SIZE - j) * input_y[2] + j * input_y[3] + RECT_SIZE / 2) /
           RECT_SIZE;
      y = ((RECT_SIZE * 2 - i) * y0 + i * y1 + RECT_SIZE) / (RECT_SIZE * 2);

      // 0.5 pixel shift for the difference of sampling positions
      // y -= PIXEL_DEC/2;
      // printf("%f, %f, ", (float)x/PIXEL_DEC, (float)y/PIXEL_DEC);
      // Separete the integer parts and decimal parts
      // The width and height of the UV image is 1/2 compared to the Y image
      x_int = x >> (PIXEL_Q + 1);
      y_int = y >> (PIXEL_Q + 1);
      x_dec = x & (PIXEL_DEC * 2 - 1);
      y_dec = y & (PIXEL_DEC * 2 - 1);

      if (x_int < 0) {
        // printf("Error: x index negative: %d\n", x_int);
      }
      if (y_int < 0) {
        // printf("Error: y index negative: %d\n", y_int);
      }

      if (x_int < 0 || y_int < 0 || x_int >= input_width / 2 - 1 ||
          y_int >= input_height / 2 - 1) {
        // Outside of the input image
        output_image_uv[j + i / 4 * output_stride] = 128;
        output_image_uv[j + 1 + i / 4 * output_stride] = 128;
      } else {
        // printf("%d, %d, %d\n", x_int, y_int, (x_int    )*2 + (y_int
        // )*input_stride); Obtain adjacent pixel values
        pixel_values[0] = input_image_uv[(x_int)*2 + (y_int)*input_stride];
        pixel_values[1] =
            input_image_uv[(x_int + 1) * 2 + (y_int)*input_stride];
        pixel_values[2] =
            input_image_uv[(x_int)*2 + (y_int + 1) * input_stride];
        pixel_values[3] =
            input_image_uv[(x_int + 1) * 2 + (y_int + 1) * input_stride];

        // printf("%d\n", pixel_values[1]);
        // printf("%d %d\n", x_dec, y_dec);
        // printf("%d\n", (x_int + 1)*2 + (y_int    )*input_stride);

        // Linear interpolation of the pixel value
        res0 = ((PIXEL_DEC * 2 - x_dec) * pixel_values[0] +
                x_dec * pixel_values[1] + PIXEL_DEC) /
               (PIXEL_DEC * 2);
        res1 = ((PIXEL_DEC * 2 - x_dec) * pixel_values[2] +
                x_dec * pixel_values[3] + PIXEL_DEC) /
               (PIXEL_DEC * 2);
        output_image_uv[j + i / 4 * output_stride] =
            ((PIXEL_DEC * 2 - y_dec) * res0 + y_dec * res1 + PIXEL_DEC) /
            (PIXEL_DEC * 2);

        // printf("%d %d\n", res0, res1);

        // Obtain adjacent pixel values
        pixel_values[0] = input_image_uv[(x_int)*2 + 1 + (y_int)*input_stride];
        pixel_values[1] =
            input_image_uv[(x_int + 1) * 2 + 1 + (y_int)*input_stride];
        pixel_values[2] =
            input_image_uv[(x_int)*2 + 1 + (y_int + 1) * input_stride];
        pixel_values[3] =
            input_image_uv[(x_int + 1) * 2 + 1 + (y_int + 1) * input_stride];

        // printf("%d\n", pixel_values[1]);

        // Linear interpolation of the pixel value
        res0 = ((PIXEL_DEC * 2 - x_dec) * pixel_values[0] +
                x_dec * pixel_values[1] + PIXEL_DEC) /
               (PIXEL_DEC * 2);
        res1 = ((PIXEL_DEC * 2 - x_dec) * pixel_values[2] +
                x_dec * pixel_values[3] + PIXEL_DEC) /
               (PIXEL_DEC * 2);
        output_image_uv[j + 1 + i / 4 * output_stride] =
            ((PIXEL_DEC * 2 - y_dec) * res0 + y_dec * res1 + PIXEL_DEC) /
            (PIXEL_DEC * 2);

        // printf("%d %d\n", res0, res1);
      }
    }
  }
};
