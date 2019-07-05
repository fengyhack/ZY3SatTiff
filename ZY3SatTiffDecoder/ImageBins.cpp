#include "ImageBins.h"
#include "stdio.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

void SaveAsBin(byte* imgData, int width, int height, const char* fileName)
{
	FILE* fp = fopen(fileName, "wb");
	if (fp == NULL)
	{
		printf("FAILED to open \'%s\' !\n", fileName);
		return;
	}

	fwrite(&width, sizeof(int), 1, fp);
	fwrite(&height, sizeof(int), 1, fp);
	fwrite(imgData, width*height, 1, fp);
	fflush(fp);
	fclose(fp);
}