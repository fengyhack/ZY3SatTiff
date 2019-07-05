# ZY3Sat - Tiff Image Decoder

   CREAT _by_  :  fengyhack (Ind.)
   
   Last modified:  2015-08-13 Thur

   Email:   fengyhack@{163,gmail}.com
   
   Website: [http://fengyh.cn/](http://fenyh.cn/)

   Supported compression type: UNCOMPRESSED, PACKBITS

## How-to-use (In caller function)

    --------------------------------------------------------------------
     
     // CHAR fileName[]="F:\\1.tiff"; // your TIFF (full) file name

     TiffDecoder  td;
     td.Open(fileName);
     td.Prepare(); // Prepare before Load !!!

     int width = td.GetWidth();
     int height = td.GetHeight();
     int imgSize = width*height;

     // -- As for ZY3 TIFF image, we know it is 16bit ('depth') --

     u16* img_src = new u16[imgSize];
     td.LoadData((byte*)img_src);

     // -- Or if we do NOT known the 'depth' filed --
     // int nbytes=td.GetDataByteCount();
     // byte* img_src=new byte[nbytes];
     // td.LoadData(img_src);
     // --

     // td.Release(); // This will bde called automatically (in td.dtor())
     
     // Do something with img_src
     
     delete[] img_src;
     
     // return

    --------------------------------------------------------------------

###### <i> (You can find the example code in Main.cpp) </i>
