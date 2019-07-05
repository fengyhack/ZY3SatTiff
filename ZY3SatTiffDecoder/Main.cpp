#include "TiffDecoder.h"
#include "ImageBins.h"
#include <time.h>
#include <exception>
using namespace std;

#define MAX_NAME_LEN 256

int main(int argc, char** argv)
{
	if (argc<2)
	{
		printf("Usage: <Command> <FileName1> <FileName2> ...");
		return -1;
	}

	printf("======== Tiff Image Decoder ======== \n\n");

	char fn[MAX_NAME_LEN] = { 0 };

	try
	{
		for (int c = 1; c < argc; ++c)
		{
			memset(fn, 0, MAX_NAME_LEN);
			strcpy(fn, argv[c]);

			TiffDecoder td;
			td.Open(fn);
			td.Prepare();

			int width = td.GetWidth();
			int height = td.GetHeight();
			int imgSize = width*height;

			printf("File: %s\nSize: %d*%d\n", fn, width, height);

			u16* srcData = new u16[imgSize];
			byte* u8 = new byte[imgSize];

			printf("Decoding...");

			time_t t0 = time(NULL);
			td.LoadData((byte*)srcData);
			time_t t1 = time(NULL);
			double et = difftime(t1, t0);

			printf("finished. (%.2f sec)\n", et);

			printf("Remapping...");

			u16 min = srcData[0];
			u16 max = min;
			for (int i = 1; i < imgSize; ++i)
			{
				if (srcData[i]>max) max = srcData[i];
				else if (srcData[i] < min) min = srcData[i];
				else continue;
			}

			if (max == min)
			{
				printf("INVALID IMAGE DATA!\n");
				throw new exception("max==min");
			}

			double factor = 255.0 / (max - min);

			for (int i = 0; i < imgSize; ++i)
			{
				u8[i] = (byte)((srcData[i] - min)*factor);
			}

			int len = strlen(fn);
			fn[len - 4] = 'b';
			fn[len - 2] = 'n';
			fn[len - 1] = 0;
			SaveAsBin(u8, width, height, fn);

			time_t t2 = time(NULL);
			et = difftime(t2, t1);

			printf("finished. (%.2f sec)\n", et);

			delete[] srcData;
			delete[] u8;

			td.Release();

			printf("Done.\n\n");
		}
	}
	catch (exception e)
	{
		puts(e.what());
	}

	printf("======== Tiff Image Decoder ======== \n\n");
}