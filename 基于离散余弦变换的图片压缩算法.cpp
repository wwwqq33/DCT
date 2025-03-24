#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include<easyx.h>
#include <conio.h>
#include <windows.h>
#include <graphics.h> 

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define BLOCK_SIZE 8
#define PI 3.14159265358979323846

int r[3][4] = { {230,20,430,100},{470,20,520,100},{560,20,610,100} };
int button_judge(int x, int y)
{
	if (x > r[0][0] && x<r[0][2] && y>r[0][1] && y < r[0][3])return 1;
	if (x > r[1][0] && x<r[1][2] && y>r[1][1] && y < r[1][3])return 2;
	if (x > r[2][0] && x<r[2][2] && y>r[2][1] && y < r[2][3])return 3;
	return 0;
}

// ��������
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

// ��8x8�Ŀ������ɢ���ұ任
void performDCT(float block[BLOCK_SIZE][BLOCK_SIZE]) {
	float coefficient;
	float sum;
	float alpha_u, alpha_v;
	float dctBlock[BLOCK_SIZE][BLOCK_SIZE];

	// ������ɢ���ұ任ϵ��
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

			// ִ����ɢ���ұ任
			for (int x = 0; x < BLOCK_SIZE; x++) {
				for (int y = 0; y < BLOCK_SIZE; y++) {
					dctBlock[u][v] += block[x][y] * cos(((2 * x + 1) * u * PI) / (2.0 * BLOCK_SIZE)) *
						cos(((2 * y + 1) * v * PI) / (2.0 * BLOCK_SIZE));
				}
			}

			dctBlock[u][v] *= coefficient;
		}
	}

	// ����ɢ���ұ任������ƻ�ԭʼ��
	for (int i = 0; i < BLOCK_SIZE; i++) {
		for (int j = 0; j < BLOCK_SIZE; j++) {
			block[i][j] = dctBlock[i][j];
		}
	}
}

// ��һ��8x8��DCTϵ���������DCT�任���õ�ԭʼ�����ؿ�
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

	// ����DCT�任������ƻ�ԭʼ��
	for (int i = 0; i < BLOCK_SIZE; i++) {
		for (int j = 0; j < BLOCK_SIZE; j++) {
			// ������ȣ����Գ���һ����������
			result[i][j] *= 15; // ���Ը�����Ҫ���е���

			block[i][j] = result[i][j];
		}
	}
}

// �Կ������������DCTϵ���������������ж�Ӧλ�õ�ֵ�����������뵽���������
void performQuantization(float block[BLOCK_SIZE][BLOCK_SIZE]) {
	for (int i = 0; i < BLOCK_SIZE; i++) {
		for (int j = 0; j < BLOCK_SIZE; j++) {
			block[i][j] = round(block[i][j] / quantizationMatrix[i][j]);
		}
	}
}

// �Կ���з��������������������������ϵ���������������ж�Ӧλ�õ�ֵ
void performDequantization(float block[BLOCK_SIZE][BLOCK_SIZE]) {
	for (int i = 0; i < BLOCK_SIZE; i++) {
		for (int j = 0; j < BLOCK_SIZE; j++) {
			block[i][j] = block[i][j] * quantizationMatrix[i][j];
		}
	}
}

// ��������ֵ��Ƶ�ʼ���ÿ������ֵ�ĸ��ʣ���ʹ���ع�ʽ������ֵ��
double calculateEntropy(unsigned char* image, int width, int height, int channels) {
	int hist[256] = { 0 }; // Histogram

	// ��������ֵ��Ƶ��
	for (int i = 0; i < width * height * channels; i += channels) {
		int pixelValue = image[i];

		hist[pixelValue]++;
	}

	double entropy = 0.0;
	double totalPixels = width * height;

	// ������ֵ
	for (int i = 0; i < 256; i++) {
		double probability = hist[i] / totalPixels;

		if (probability > 0)
			entropy -= probability * log2(probability);
	}

	return entropy;
}

int main() {
	initgraph(1200, 1800);
	setbkcolor(BLACK);
	cleardevice();
	RECT R1 = { r[0][0],r[0][1],r[0][2],r[0][3] };
	RECT R2 = { r[1][0],r[1][1],r[1][2],r[1][3] };
	RECT R3 = { r[2][0],r[2][1],r[2][2],r[2][3] };
	drawtext("���ѹ�����ͼƬ", &R1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);//�ھ�������R1���������֣�ˮƽ���У���ֱ���У�������ʾ
	drawtext("����ֵ", &R2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);//�ھ�������R2���������֣�ˮƽ���У���ֱ���У�������ʾ
	drawtext("�˳�", &R3, DT_CENTER | DT_VCENTER | DT_SINGLELINE);//�ھ�������R3���������֣�ˮƽ���У���ֱ���У�������ʾ
	setlinecolor(BLACK);
	rectangle(r[1][0], r[1][1], r[1][2], r[1][3]);
	rectangle(r[2][0], r[2][1], r[2][2], r[2][3]);
	rectangle(r[3][0], r[3][1], r[3][2], r[3][3]);
	MOUSEMSG m;//���ָ��
	setrop2(R2_NOTXORPEN);//��Ԫ��դ����NOT(��Ļ��ɫ XOR ��ǰ��ɫ)
	while (true)
	{
		m = GetMouseMsg();//��ȡһ�������Ϣ
		if (m.uMsg == WM_LBUTTONDOWN)
		{
			for (int i = 0; i <= 10; i++)
			{
				setlinecolor(RGB(25 * i, 25 * i, 25 * i));//����Բ��ɫ
				circle(m.x, m.y, 2 * i);
				Sleep(25);//ͣ��2ms
				circle(m.x, m.y, 2 * i);//Ĩȥ�ոջ���Բ
			}
			flushmessage();//��������Ϣ������
		}
		settextcolor(WHITE);
		settextstyle(30, 0, _T("����"));
		outtextxy(400, 120, _T("ԭʼͼƬ"));
		IMAGE input;
		IMAGE output;
		double db;
		char str[100];
		loadimage(&input, _T("inout.jpg"));
		putimage(300, 160, &input);
		// ��ȡͼ������
		int width, height, channels;
		unsigned char* image = stbi_load("inout.jpg", &width, &height, &channels, 0);


		// ����ͼ�����к�����
		int rows = height / BLOCK_SIZE;
		int cols = width / BLOCK_SIZE;

		// �����µ�ͼ����������
		unsigned char* newImage = (unsigned char*)malloc(width * height * channels * sizeof(unsigned char));

		// ��ÿ��8x8����д���
		for (int row = 0; row < rows; row++) {
			for (int col = 0; col < cols; col++) {
				// ��ȡ��ǰ�����ʼ����λ��
				int startRow = row * BLOCK_SIZE;
				int startCol = col * BLOCK_SIZE;

				// ������ǰ������ؿ�
				float blockR[BLOCK_SIZE][BLOCK_SIZE];
				float blockG[BLOCK_SIZE][BLOCK_SIZE];
				float blockB[BLOCK_SIZE][BLOCK_SIZE];

				// ��ÿһ�����ÿһ������ֵת��Ϊ��������������洢�����ؿ���
				for (int i = 0; i < BLOCK_SIZE; i++) {
					for (int j = 0; j < BLOCK_SIZE; j++) {
						int pixelIndex = ((startRow + i) * width + (startCol + j)) * channels;

						// ��ͨ��ͼ�񣨻Ҷ�ͼ�񣩣���ÿ�����ص���ֱֵ��ת��Ϊ���������洢�����ؿ���
						if (channels == 1) {
							blockR[i][j] = (float)image[pixelIndex];
							blockG[i][j] = (float)image[pixelIndex];
							blockB[i][j] = (float)image[pixelIndex];
						}
						// ͼ������ͨ��ͼ��RGBͼ�񣩣���RGB����ֵת��Ϊ��Ӧͨ���ĸ�����
						//�������ͬ�ĻҶ�ֵ��������ͨ�����൱�ڽ���ɫ��Ϣѹ�����˵�һ�ĻҶ�ֵ��
						else if (channels == 3) {
							blockR[i][j] = (float)image[pixelIndex];
							blockG[i][j] = (float)image[pixelIndex + 1];
							blockB[i][j] = (float)image[pixelIndex + 2];
						}
					}
				}

				// ����DCT�任
				performDCT(blockR);
				performDCT(blockG);
				performDCT(blockB);

				// �Կ��������
				performQuantization(blockR);
				performQuantization(blockG);
				performQuantization(blockB);

				// �Կ���з�����
				performDequantization(blockR);
				performDequantization(blockG);
				performDequantization(blockB);

				// ������DCT�任
				performIDCT(blockR);
				performIDCT(blockG);
				performIDCT(blockB);

				// �����������ؿ�洢����ͼ������������
				for (int i = 0; i < BLOCK_SIZE; i++) {
					for (int j = 0; j < BLOCK_SIZE; j++) {
						int pixelIndex = ((startRow + i) * width + (startCol + j)) * channels;

						if (channels == 1) {
							// ���ڻҶ�ͼ�񣬽�����ֵ�洢������ͨ��
							newImage[pixelIndex] = (unsigned char)blockR[i][j];
							newImage[pixelIndex + 1] = (unsigned char)blockR[i][j];
							newImage[pixelIndex + 2] = (unsigned char)blockR[i][j];
						}
						else if (channels == 3) {
							// ���ڲ�ɫͼ�񣬽�������RGBֵ�洢����Ӧͨ��
							newImage[pixelIndex] = (unsigned char)blockR[i][j]; // �����ĺ�ɫͨ��ֵ
							newImage[pixelIndex + 1] = (unsigned char)blockG[i][j]; // ��������ɫͨ��ֵ
							newImage[pixelIndex + 2] = (unsigned char)blockB[i][j]; // ��������ɫͨ��ֵ
						}
					}
				}
			}
		}
		switch (button_judge(m.x, m.y))
		{
			//��ԭ��ťԭ��
		case 1:
			if (!image) {
				outtextxy(350, 500, _T("ѹ��ʧ�ܣ�δ�ҵ��ļ���"));
				return 1;
			}
			outtextxy(350, 500, _T("ѹ����ʼ����Լ��Ҫ�����"));
			 //���洦����ͼ��100����ʾJPEGͼ���ѹ����������Χ��0��100������100��ʾ�������������ѹ����
			stbi_write_jpg("output.jpg", width, height, channels, newImage, 5);
			outtextxy(350, 530, _T("ѹ�����"));
			outtextxy(350, 560, _T("ѹ����ͼƬΪ��"));
			loadimage(&output, _T("output.jpg"));
			putimage(300, 600, &output);

			
			flushmessage();//�����¼�����������Ϣ
			break;
		case 2:
			// ����ͼ�����ֵ
			db = calculateEntropy(newImage, width, height, channels);
			settextcolor(GREEN);
			settextstyle(30, 0, _T("����"));
			outtextxy(350, 900, _T("ͼ�����ֵΪ: "));
			sprintf_s(str, "%lf", db);
			outtextxy(550, 900, str);

			flushmessage();//�����¼�����������Ϣ
			break;
		case 3:
			// �ͷ��ڴ�
			stbi_image_free(image);
			free(newImage);
			closegraph();//�رջ�ͼ����
			exit(0);//�����˳�
			break;
		default:
			flushmessage();//�����¼�����������Ϣ
			//printf("\r\n(%d,%d)",m.x,m.y);//��ӡ������꣬�������ʱȷ������
			break;
		}

	}
	
	

	
	

	
	system("pause");
	return 0;
}