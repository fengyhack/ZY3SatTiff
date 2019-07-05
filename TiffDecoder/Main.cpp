#include "TiffDecoder.h"

int main(void)
{
	CHAR fileName[] = "F:\\1.tiff";

	TiffDecoder  td;
	td.Open(fileName);
	td.Prepare(); // Prepare before Load !!!
	
	 int width = td.GetWidth();
	int height = td.GetHeight();
	int imgSize = width*height;
	
	// -- As for ZY3 TIFF image, we know it is 16bit ('depth') --

	u16* img_src = new u16[imgSize];
	td.LoadData((byte*)img_src);

	// Do something with img_src

	delete[] img_src;

	td.Release(); // This will be called automatically

	return 0;
}