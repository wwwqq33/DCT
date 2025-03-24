#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define BLOCK_SIZE 8
#define PI 3.14159265358979323846

// 量化矩阵
int quantizationMatrix[BLOCK_SIZE][BLOCK_SIZE] = {
	{16, 11, 10, 16, 24, 40, 51, 61},
	{12, 12, 14, 19, 26, 58, 60, 55},
	{14, 13, 16, 24, 40, 57, 69, 56},
	{14, 17, 22, 29, 51, 87, 80, 62},
	{18, 22, 37, 56, 68, 109, 103, 77},
	{24, 35, 55, 64, 81, 104, 113, 92},
	{49, 64, 78, 87, 103, 121, 120, 101},
	{72, 92, 95, 98, 112, 100, 103, 99}
};

// 对8x8的块进行离散余弦变换
void performDCT(float block[BLOCK_SIZE][BLOCK_SIZE]) {
	float coefficient;
	float sum;
	float alpha_u, alpha_v;
	float dctBlock[BLOCK_SIZE][BLOCK_SIZE];

	// 计算离散余弦变换系数
	for (int u = 0; u < BLOCK_SIZE; u++) {
		for (int v = 0; v < BLOCK_SIZE; v++) {
			if (u == 0)
				alpha_u = sqrt(1.0 / BLOCK_SIZE);
			else
				alpha_u = sqrt(2.0 / BLOCK_SIZE);

			if (v == 0)
				alpha_v = sqrt(1.0 / BLOCK_SIZE);
			else
				alpha_v = sqrt(2.0 / BLOCK_SIZE);

			coefficient = (alpha_u * alpha_v) / 4.0;

			dctBlock[u][v] = 0.0;

			// 执行离散余弦变换
			for (int x = 0; x < BLOCK_SIZE; x++) {
				for (int y = 0; y < BLOCK_SIZE; y++) {
					dctBlock[u][v] += block[x][y] * cos(((2 * x + 1) * u * PI) / (2.0 * BLOCK_SIZE)) *
					                  cos(((2 * y + 1) * v * PI) / (2.0 * BLOCK_SIZE));
				}
			}

			dctBlock[u][v] *= coefficient;
		}
	}

	// 将离散余弦变换结果复制回原始块
	for (int i = 0; i < BLOCK_SIZE; i++) {
		for (int j = 0; j < BLOCK_SIZE; j++) {
			block[i][j] = dctBlock[i][j];
		}
	}
}

// 将一个8x8的DCT系数块进行逆DCT变换，得到原始的像素块
void performIDCT(float block[BLOCK_SIZE][BLOCK_SIZE]) {
	float result[BLOCK_SIZE][BLOCK_SIZE];

	for (int u = 0; u < BLOCK_SIZE; u++) {
		for (int v = 0; v < BLOCK_SIZE; v++) {
			float sum = 0.0;
			float alpha_u, alpha_v;

			for (int x = 0; x < BLOCK_SIZE; x++) {
				for (int y = 0; y < BLOCK_SIZE; y++) {
					if (x == 0)
						alpha_u = sqrt(1.0 / BLOCK_SIZE);
					else
						alpha_u = sqrt(2.0 / BLOCK_SIZE);

					if (y == 0)
						alpha_v = sqrt(1.0 / BLOCK_SIZE);
					else
						alpha_v = sqrt(2.0 / BLOCK_SIZE);

					float coefficient = (alpha_u * alpha_v) / 4.0;

					sum += coefficient * block[x][y] * cos(((2 * u + 1) * x * PI) / (2.0 * BLOCK_SIZE)) *
					       cos(((2 * v + 1) * y * PI) / (2.0 * BLOCK_SIZE));
				}
			}

			result[u][v] = sum;
		}
	}

	// 将逆DCT变换结果复制回原始块
	for (int i = 0; i < BLOCK_SIZE; i++) {
		for (int j = 0; j < BLOCK_SIZE; j++) {
			// 提高亮度，可以乘以一个增益因子
			result[i][j] *= 15; // 可以根据需要进行调整

			block[i][j] = result[i][j];
		}
	}
}

// 对块进行量化，将DCT系数除以量化矩阵中对应位置的值，并四舍五入到最近的整数
void performQuantization(float block[BLOCK_SIZE][BLOCK_SIZE]) {
	for (int i = 0; i < BLOCK_SIZE; i++) {
		for (int j = 0; j < BLOCK_SIZE; j++) {
			block[i][j] = round(block[i][j] / quantizationMatrix[i][j]);
		}
	}
}

// 对块进行反量化，反量化操作将量化后的系数乘以量化矩阵中对应位置的值
void performDequantization(float block[BLOCK_SIZE][BLOCK_SIZE]) {
	for (int i = 0; i < BLOCK_SIZE; i++) {
		for (int j = 0; j < BLOCK_SIZE; j++) {
			block[i][j] = block[i][j] * quantizationMatrix[i][j];
		}
	}
}

// 根据像素值的频率计算每个像素值的概率，并使用熵公式计算熵值。
double calculateEntropy(unsigned char *image, int width, int height, int channels) {
	int hist[256] = {0}; // Histogram

	// 计算像素值的频率
	for (int i = 0; i < width * height * channels; i += channels) {
		int pixelValue = image[i];

		hist[pixelValue]++;
	}

	double entropy = 0.0;
	double totalPixels = width * height;

	// 计算熵值
	for (int i = 0; i < 256; i++) {
		double probability = hist[i] / totalPixels;

		if (probability > 0)
			entropy -= probability * log2(probability);
	}

	return entropy;
}

int main() {
	// 读取图像数据
	int width, height, channels;
	unsigned char *image = stbi_load("input1.jpg", &width, &height, &channels, 0);

	if (!image) {
		printf("图片加载出错，请检查路径或者命名\n");
		return 1;
	}
	printf("图片正在压缩中，请不要关闭界面...\n");

	// 计算图像块的行和列数
	int rows = height / BLOCK_SIZE;
	int cols = width / BLOCK_SIZE;

	// 创建新的图像数据数组
	unsigned char *newImage = (unsigned char *)malloc(width * height * channels * sizeof(unsigned char));

	// 对每个8x8块进行处理
	for (int row = 0; row < rows; row++) {
		for (int col = 0; col < cols; col++) {
			// 获取当前块的起始像素位置
			int startRow = row * BLOCK_SIZE;
			int startCol = col * BLOCK_SIZE;

			// 创建当前块的像素块
			float blockR[BLOCK_SIZE][BLOCK_SIZE];
			float blockG[BLOCK_SIZE][BLOCK_SIZE];
			float blockB[BLOCK_SIZE][BLOCK_SIZE];

			// 对每一个块的每一个像素值转换为浮点数，并将其存储在像素块中
			for (int i = 0; i < BLOCK_SIZE; i++) {
				for (int j = 0; j < BLOCK_SIZE; j++) {
					int pixelIndex = ((startRow + i) * width + (startCol + j)) * channels;

					// 单通道图像（灰度图像），则每个像素的数值直接转换为浮点数并存储在像素块中
					if (channels == 1) {
						blockR[i][j] = (float)image[pixelIndex];
						blockG[i][j] = (float)image[pixelIndex];
						blockB[i][j] = (float)image[pixelIndex];
					}
					// 图像是三通道图像（RGB图像），则将RGB像素值转换为对应通道的浮点数
					//如果将相同的灰度值赋给三个通道，相当于将彩色信息压缩成了单一的灰度值。
					else if (channels == 3) {
						blockR[i][j] = (float)image[pixelIndex];
						blockG[i][j] = (float)image[pixelIndex + 1];
						blockB[i][j] = (float)image[pixelIndex + 2];
					}
				}
			}

			// 进行DCT变换
			performDCT(blockR);
			performDCT(blockG);
			performDCT(blockB);

			// 对块进行量化
			performQuantization(blockR);
			performQuantization(blockG);
			performQuantization(blockB);

			// 对块进行反量化
			performDequantization(blockR);
			performDequantization(blockG);
			performDequantization(blockB);

			// 进行逆DCT变换
			performIDCT(blockR);
			performIDCT(blockG);
			performIDCT(blockB);

			// 将处理后的像素块存储在新图像数据数组中
			for (int i = 0; i < BLOCK_SIZE; i++) {
				for (int j = 0; j < BLOCK_SIZE; j++) {
					int pixelIndex = ((startRow + i) * width + (startCol + j)) * channels;

					if (channels == 1) {
						// 对于灰度图像，将亮度值存储到所有通道
						newImage[pixelIndex] = (unsigned char)blockR[i][j];
						newImage[pixelIndex + 1] = (unsigned char)blockR[i][j];
						newImage[pixelIndex + 2] = (unsigned char)blockR[i][j];
					} else if (channels == 3) {
						// 对于彩色图像，将处理后的RGB值存储到对应通道
						newImage[pixelIndex] = (unsigned char)blockR[i][j]; // 处理后的红色通道值
						newImage[pixelIndex + 1] = (unsigned char)blockG[i][j]; // 处理后的绿色通道值
						newImage[pixelIndex + 2] = (unsigned char)blockB[i][j]; // 处理后的蓝色通道值
					}
				}
			}
		}
	}

	// 保存处理后的图像，100：表示JPEG图像的压缩质量，范围从0到100，其中100表示最高质量的无损压缩。
	stbi_write_jpg("output14.jpg", width, height, channels, newImage, 10);
	printf("图片压缩成功！\n请回目录下查看\n");

	// 计算图像的熵值
	double entropy = calculateEntropy(newImage, width, height, channels);
	printf("图像的熵值为: %f\n", entropy);

	// 释放内存
	stbi_image_free(image);
	free(newImage);

	return 0;
}