#include<fstream>
#include<iostream>
#include "app1.h"

int main(){
	
	// Open JPG as binary file
	ifstream jpg("./img/img016.jpg", ios::binary | ios::ate);
	ofstream jpgExif("./img/img016Exif.jpg", ios::binary | ios::trunc);
	
	
	// Get filesize in bytes
	int filesize = jpg.tellg();
	jpg.seekg(0, ios::beg);		// Reset for reading
	
	unsigned char buf[2];
	
	// Read SOI (0xFFD8) from input JPG and write to output JPG
	jpg.read((char*)&buf, 2);
	jpgExif.write((char*)buf, 2);
	
	
	
	
	
	// Create APP1 segment with metadata
	APP1 app1;
	app1.addMetadata(apertureIFDTag, 14);
	app1.addMetadata(shutterSpeedIFDTag, 125);
	
	// Export APP1
	unsigned char appBytes[app1.getSize()];
	app1.get(appBytes);
	
	
	
	
	
	// Write generated APP1 to output JPG
	jpgExif.write((char*)&appBytes, app1.getSize());
	
	// Read input JPG in 2-byte chunks and writes it to output JPG, omitting any APPn segments
	for(int i = 1; i < filesize; i+=2){	// Incremented by 2 since input JPG is read in 2-byte increments
		jpg.read((char*)&buf, 2);
		
		// Look for an APPn marker; denoting the beginning of an APPn segment
		if(buf[0] == 0xFF && (buf[1] & 0xF0) == 0xE0){
			jpg.read((char*)&buf, 2);	// Read length of APPn segment
			
			// Read through the APPn segment and omit writing segment to output JPG
			// segLength is subtracted by 2 to remove the 2 bytes denoting length (was already read)
			unsigned short segLength = byteToUShort(buf);
			for(int x = 0; x < (segLength - 2); x++)
				jpg.read((char*)&buf, 1);
		}
		// No APPn marker; write to file
		else{
			jpgExif.write((char*)buf, 2);
		}
	}
	
	// Close file streams
	jpg.close();
	jpgExif.close();
	
	return 0;
}