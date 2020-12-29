#include<fstream>
#include<iostream>

using namespace std;

int main(){
	cout << hex;	// Print in hex
	
	// Open JPG as binary file
	ifstream jpg("./img/img016.jpg", ios::binary | ios::ate);
	ofstream jpgExif("./img/img016exif.jpg", ios::binary | ios::trunc);
	
	// Get filesize in bytes
	int filesize = jpg.tellg();
	jpg.seekg(0, ios::beg);	// Reset for reading
	
	// Read file in 2-byte chunks
	unsigned char buf[2];	// 2 byte buffer
	for(int i = 0; i < 10; i++){
		jpg.read((char*)&buf, 2);
		cout << (int)buf[0] << (int)buf[1] << endl;
		jpgExif.write((char*)buf, 2);
	}
	
	// Insert after APP0
	// APP1 IFD for f/2 and 1/125
	unsigned char exif[60] = {	0xFF, 0xE1, 0x00, 0x38, 0x45, 0x78, 0x69, 0x66, 0x00, 0x00, 0x49, 0x49, 0x2a, 0x00,
								0x08, 0x00, 0x00, 0x00, 0x00, 0x00,0x9D, 0x82, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x28, 0x00,
								0x00, 0x00,	0x9A, 0x82, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x30, 0x00,	0x00, 0x00,
								0xC8, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x20, 0x03,
								0x00, 0x00	};
	
	jpgExif.write((char*)exif, 60);
	
	
	
	
	for(int i = 10; i < filesize; i++){
		jpg.read((char*)&buf, 2);
		//cout << (int)buf[0] << (int)buf[1] << endl;
		jpgExif.write((char*)buf, 2);
	}
	
	
	// Psuedocode for Exif cleanup and insertion
	/*
	begin
		while(in APPn zone){
			check APPn segment type
			get segment length
			skip forward length
			if(APP1)
				delete
			else
				keep
		}
		insert custom APP1
		write rest of data
	end
	*/
	
	
	
	jpg.close();
	jpgExif.close();
	return 0;
}