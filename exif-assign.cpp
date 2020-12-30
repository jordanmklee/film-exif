#include<fstream>
#include<iostream>
#include<vector>

// EXIF/TIFF Tags
unsigned char exifIFDTag[2] = {0x87, 0x69};
unsigned char apertureIFDTag[2] = {0x82, 0x9D};
unsigned char shutterSpeedIFDTag[2] = {0x82, 0x9A};

// IFDs
unsigned char ifdOffset[4] = {0x00, 0x00, 0x00, 0x00};

// APP1
unsigned char app1Tag[2] = {0xFF, 0xE1};
unsigned char exifID[6] = {0x45, 0x78, 0x69, 0x66, 0x00, 0x00};	// "Exif  "

// TIFF
unsigned char littleEndianID[2] = {0x49, 0x49};	// "II"
unsigned char bigEndianID[2] = {0x4D, 0x4D};	// "MM"
unsigned char tiffID[2] = {0x00, 0x2A};			// 42

using namespace std;

void printBytes(unsigned char arr[], int len){
	for(int i = 0; i < len; i++){
		printf("%02x", arr[i]);
		cout << " ";
	}
}

class IFDField{
	private:
		unsigned char tagID[2];
		unsigned char typeID[2];
		unsigned char count[4];
		unsigned char value[4];
	
	public:
		// Return field as a vector of bytes
		vector<unsigned char> getField(){
			vector<unsigned char> field;
			
			// Add field elements to vector
			for(int i = 0; i < 2; i++)
				field.push_back(tagID[i]);
			
			for(int i = 0; i < 2; i++)
				field.push_back(typeID[i]);
			
			for(int i = 0; i < 4; i++)
				field.push_back(count[i]);
			
			for(int i = 0; i < 4; i++)
				field.push_back(value[i]);
				
			return field;
		}
		
		void setTagID(unsigned char tag[]){
			memcpy(tagID, tag, 2);
		}
		
		void setTypeID(unsigned char type[]){
			memcpy(typeID, type, 2);
		}
		
		void setCount(unsigned char c[]){
			memcpy(count, c, 4);
		
		}
		
		// Set value or offset (if data greater than 4 bytes; offset from beginning of TIFF header)
		void setValue(unsigned char v[]){
			memcpy(value, v, 4);
		}
};

class ExifIFDField : public IFDField{
	public:
		// Creates a IFD field for the ExifIFD offset
		// Field value (the actual offset needs to be calculated and set via IFDField method)
		ExifIFDField(){
			unsigned char type[2] = {0x00, 0x04};
			unsigned char count[4] = {0x00, 0x00, 0x00, 0x01};
			
			setTagID(exifIFDTag);
			setTypeID(type);
			setCount(count);
		}	
};

class IFDFieldWithData : public IFDField{
	private:
		vector<unsigned char> data;
	
	public:		
		// Adds data to data vector
		void setData(unsigned char newData[], int dataLength){
			for(int i = 0; i < dataLength; i++){
				data.push_back(newData[i]);
			}
		}
		
		// Returns the data vector
		vector<unsigned char> getData(){
			cout << "Data" << endl;
			for(int i = 0; i < data.size(); i++){
				printf("%02X", data.at(i));
				cout << " ";
			}
			cout << endl << endl;
			return data;
		}
};

class ApertureIFDField : public IFDFieldWithData{
	private:
		unsigned char apertureNumerator[4];
		unsigned char apertureDenominator[4];
	
	public:
		ApertureIFDField(){
			unsigned char type[2] = {0x00, 0x05};				// Type 5 = rational
			unsigned char count[4] = {0x00, 0x00, 0x00, 0x01};	// Count = 1
			
			setTagID(apertureIFDTag);
			setTypeID(type);
			setCount(count);
		}
		
		// Aperture is stored in XML as integer = (f-stop * 10)
		// eg. f/1.4 stored as 14
		// Aperture is stored in IFD data area as numerator/denominator = (XML-value)/10
		void setApertureData(int aperture){
			// Denominator is always 10
			unsigned char denom[4] = {0x00, 0x00, 0x00, 0x0A};
			memcpy(apertureDenominator, denom, 4);
			
			// Numerator is f-stop * 10
			apertureNumerator[0] = 0x00;
			apertureNumerator[1] = 0x00;
			apertureNumerator[2] = 0x00;
			
			switch(aperture){
				case 14:
					apertureNumerator[3] = 0x0E;
					break;
				case 20:
					apertureNumerator[3] = 0x14;
					break;
				case 28:
					apertureNumerator[3] = 0x1C;
					break;
				case 40:
					apertureNumerator[3] = 0x28;
					break;
				case 56:
					apertureNumerator[3] = 0x38;
					break;
				case 80:
					apertureNumerator[3] = 0x50;
					break;
				case 110:
					apertureNumerator[3] = 0x6E;
					break;
				case 160:
					apertureNumerator[3] = 0xA0;
					break;
				case 220:
					apertureNumerator[3] = 0xDC;
					break;
			}
			
			// Add numerator/denominator to data field
			setData(apertureNumerator, 4);
			setData(apertureDenominator, 4);	
		}
};

class ShutterSpeedIFDField : public IFDFieldWithData{
	private:
		unsigned char shutterSpeedNumerator[4];
		unsigned char shutterSpeedDenominator[4];
	
	public:
		ShutterSpeedIFDField(){
			unsigned char type[2] = {0x00, 0x05};				// Type 5 = rational
			unsigned char count[4] = {0x00, 0x00, 0x00, 0x01};	// Count = 1
			
			setTagID(shutterSpeedIFDTag);
			setTypeID(type);
			setCount(count);
		}
		
		// Shutter speed is stored in XML as integer = (time in seconds) * 1000
		// eg. 1/8 is stored as (0.125 * 1000) = 125
		// Shutter speed is stored in IFD data area as numerator/denominator = (XML-value) / 1000
		void setShutterSpeedData(int shutterSpeed){
			// Denominator is always 1000
			unsigned char denom[4] = {0x00, 0x00, 0x03, 0x0E8};
			memcpy(shutterSpeedDenominator, denom, 4);
			
			// Numerator is (s) * 1000
			shutterSpeedNumerator[0] = 0x00;
			shutterSpeedNumerator[1] = 0x00;
			shutterSpeedNumerator[2] = 0x00;
			
			switch(shutterSpeed){
				case 1:
					shutterSpeedNumerator[3] = 0x01;
					break;
				case 2:
					shutterSpeedNumerator[3] = 0x02;
					break;
				case 4:
					shutterSpeedNumerator[3] = 0x04;
					break;
				case 8:
					shutterSpeedNumerator[3] = 0x08;
					break;
				case 16:
					shutterSpeedNumerator[3] = 0x10;
					break;
				case 30:
					shutterSpeedNumerator[3] = 0x1E;
					break;
				case 60:
					shutterSpeedNumerator[3] = 0x3C;
					break;
				case 125:
					shutterSpeedNumerator[3] = 0x7D;
					break;
				case 250:
					shutterSpeedNumerator[3] = 0xFA;
					break;
				case 500:
					shutterSpeedNumerator[2] = 0x01;
					shutterSpeedNumerator[3] = 0xFA;
					break;
				case 1000:
					shutterSpeedNumerator[2] = 0x03;
					shutterSpeedNumerator[3] = 0xE8;
					break;
			}
			
			// Add numerator/denominator to data field
			setData(shutterSpeedNumerator, 4);
			setData(shutterSpeedDenominator, 4);
		}
};

class IFD{
	private:
		short numFields;	// short (2 bytes) so it can be incremented
		vector<IFDField> fields;
		unsigned char offsetNextIFD[4];
		vector<unsigned char> dataArea;
		
	public:
		IFD(){
			numFields = 0x0000;
			memcpy(offsetNextIFD, ifdOffset, 4);
		}
		
		vector<unsigned char> getIFD(){
			vector<unsigned char> ifdBytes;
			
			// Add numFields as bytes
			ifdBytes.push_back((numFields >> 8) & 0xFF);
			ifdBytes.push_back(numFields & 0xFF);
			
			// Add each field
			for(int i = 0; i < fields.size(); i++){
				vector<unsigned char> field = fields.at(i).getField();
				ifdBytes.insert(end(ifdBytes), begin(field), end(field));
			}
			
			// Add offset to next IFD
			for(int i = 0; i < 4; i++){
				ifdBytes.push_back(offsetNextIFD[i]);
			}
			
			// Add data area
			ifdBytes.insert(end(ifdBytes), begin(dataArea), end(dataArea));
			
			return ifdBytes;
		}
		
		// Adds a new IFD field
		void addField(IFDField newField){
			numFields++;
			fields.push_back(newField);
		}
		
		// Adds a new IFD field and data greater than 4 bytes in data area
		void addField(IFDFieldWithData newField){
			numFields++;
			fields.push_back(newField);
			
			// Retrieve data byte array and size
			vector<unsigned char> data = newField.getData();
			
			// Add each byte of field data to data area
			for(int i = 0; i < data.size(); i++){
				dataArea.push_back(data.at(i));
			}
			
			// TODO Calculate offsets
			// TODO Update offset in field
		}
		
		// TODO Update offsets function
		
		// Returns size of IFD
		short getSize(){
			short ifdSize = 0x00;
			
			ifdSize += sizeof(numFields);
			ifdSize += (numFields * 12);		// Each IFD field is 12 bytes long
			ifdSize += sizeof(offsetNextIFD);
			
			// Add sizes of data
			ifdSize += dataArea.size();
			
			return ifdSize;
		}
		
		// Retrieves the next offset (in bytes) where data can be inserted
		// Offsets for IFD entries are calculated from the beginning of the TIFF header
		void getDataOffset(){
			// TODO
		}
};


class APP1{
	private:
		// APP1
		unsigned char app1Marker[2];
		unsigned char app1Size[2];
		unsigned char exifMarker[6];
	
		// TIFF header
		unsigned char endianess[2];
		unsigned char tiffMarker[2];
		unsigned char offset0IFD[4];
		
		// IFDs
		IFD ifd0;
		IFD exifIFD;
	
	public:
		APP1(IFD zero, IFD exif){
			memcpy(app1Marker, app1Tag, 2);
			memcpy(exifMarker, exifID, 6);
			
			memcpy(endianess, bigEndianID, 2);
			memcpy(tiffMarker, tiffID, 2);
			unsigned char eightBytesOffset[4] = {0x00, 0x00, 0x00, 0x08};	// 8 byte offset to 0th IFD
			memcpy(offset0IFD, eightBytesOffset, 4);
			
			ifd0 = zero;
			exifIFD = exif;
		}
		
		// Return everything as byte vector to write to file
		vector<unsigned char> getApp1(){
			vector<unsigned char> app1Bytes;
			
			// Add APP1 header
			for(int i = 0; i < 2; i++)
				app1Bytes.push_back(app1Marker[i]);
			for(int i = 0; i < 2; i++)
				app1Bytes.push_back(app1Size[i]);
			for(int i = 0; i < 6; i++)
				app1Bytes.push_back(exifMarker[i]);
			
			// Add TIFF header
			for(int i = 0; i < 2; i++)
				app1Bytes.push_back(endianess[i]);
			for(int i = 0; i < 2; i++)
				app1Bytes.push_back(tiffMarker[i]);
			for(int i = 0; i < 4; i++)
				app1Bytes.push_back(offset0IFD[i]);
			
			// Add 0IFD
			vector<unsigned char> ifd0Bytes = ifd0.getIFD();
			app1Bytes.insert(end(app1Bytes), begin(ifd0Bytes), end(ifd0Bytes));
			
			// Add EXIF IFD
			vector<unsigned char> exifBytes = exifIFD.getIFD();
			app1Bytes.insert(end(app1Bytes), begin(exifBytes), end(exifBytes));
			
			return app1Bytes;
		}
		
		// Sets the APP1 size bytes; size of APP1 headers + IFD0 + EXIF IFD
		void setApp1Size(short newSize){
			app1Size[0] = (newSize >> 8) & 0xFF;
			app1Size[1] = newSize & 0xFF;
		}
		
		// Get size of entire APP1 (APP1 headers, IFD0, EXIF IFD) in bytes
		short getTotalSize(){
			short totalSize = 0x0000;
			
			totalSize += getHeadersSize();
			totalSize += ifd0.getSize();
			totalSize += exifIFD.getSize();
			
			return totalSize;
		}
		
		// Get size of the all the APP1 headers (APP1, Exif, TIFF) in bytes
		short getHeadersSize(){
			short headersSize = 0x0000;
			
			headersSize += sizeof(app1Marker);
			headersSize += sizeof(app1Size);
			
			headersSize += sizeof(exifMarker);
			
			headersSize += getTiffHeaderSize();
			
			return headersSize;
		}
		
		// Get size of the TIFF header in bytes
		short getTiffHeaderSize(){
			short headerSize = 0x0000;
			
			headerSize += sizeof(endianess);
			headerSize += sizeof(tiffMarker);
			headerSize += sizeof(offset0IFD);
			
			return headerSize;
		}
		
		int getIFD0Size(){
			return ifd0.getSize();			
		}
	
};


int main(){
	cout << hex;	// Print in hex
	
	// Open JPG as binary file
	ifstream jpg("./img/img016.jpg", ios::binary | ios::ate);
	ofstream jpgExif("./img/img016exif.jpg", ios::binary | ios::trunc);
	
	
	IFD ifd0;
	ExifIFDField eifd;
	ifd0.addField(eifd);
	
	IFD exifIFD;
	ApertureIFDField aper;
	aper.setApertureData(14);
	exifIFD.addField(aper);
	aper.getField();
	
	
	ShutterSpeedIFDField ss;
	ss.setShutterSpeedData(250);
	exifIFD.addField(ss);
	
	APP1 app1(ifd0, exifIFD);
	app1.setApp1Size(app1.getTotalSize() - 2);	// Subtract 2 since APP1 marker (0xFFE1) is not included
	
	cout << "ifd0 size: 0x" << ifd0.getSize() << endl;
	cout << "exifIfd size: 0x" << exifIFD.getSize() << endl;
	cout << "APP1 total size: 0x" << app1.getTotalSize() << endl;
	cout << endl << endl << endl;
	
	
	
	
	
	// TODO Calculate offsets for data area in EXIF IFD, and EXIF OFFSET in IFD0, then update offsets
	app1.getTiffHeaderSize();
	app1.getIFD0Size();
	
	
	
	
	
	
	// Export APP1 header
	vector<unsigned char> app = app1.getApp1();
	// Convert to byte array for writing
	unsigned char appBytes[app.size()];
	copy(app.begin(), app.end(), appBytes);
	
	
	printBytes(appBytes, app.size());	// TODO remove
	
	/*
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
	
	*/

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