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
	for(int i = 0; i < len; i+=2){
		printf("%02x", arr[i]);
		printf("%02x", arr[i+1]);
		cout << " ";
	}
}

void prettyPrintAPP1(unsigned char arr[], int len){
	for(int i = 0; i < len; i+=2){
		printf("%02x", arr[i]);
		printf("%02x", arr[i+1]);
		cout << " ";
		
		if(	i == 0 ||
			i == 2 ||
			i == 8 ||
			i == 10 ||
			i == 12 ||
			i == 16 ||
			i == 18 ||
			i == 30 ||
			i == 34 ||
			i == 36 ||
			i == 48 ||
			i == 60 ||
			i == 64){
				cout << endl;
				
			}
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
		
		void setTagID(unsigned char newTag[]){
			memcpy(tagID, newTag, 2);
		}
		
		void setTypeID(unsigned char newType[]){
			memcpy(typeID, newType, 2);
		}
		
		void setCount(unsigned char newCount[]){
			memcpy(count, newCount, 4);
		
		}
		
		// Set value or offset (if data greater than 4 bytes; offset from beginning of TIFF header)
		void setValue(unsigned char newValue[]){
			memcpy(value, newValue, 4);
		}
		
		// Checks if this field is tagID
		bool isTag(unsigned char id[]){
			if(tagID[0] == id[0] && tagID[1] == id[1])
				return true;
			
			return false;
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
			unsigned char empty[] = {0x00, 0x00, 0x00, 0x00};
			setValue(empty);
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
		
		vector<unsigned char> get(){
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
		// Sets the value of a field by tagID
		void setFieldValue(unsigned char tagID[], unsigned char newValue[]){
			// Look for field
			bool fieldFound = false;
			for(int i = 0; i < fields.size(); i++){
				// Updates value of field if found
				if(fields.at(i).isTag(tagID)){
					fieldFound = true;
					
					fields.at(i).setValue(newValue);
					
				}
			}
			if(!fieldFound){
			
			
			
				// TODO produce error instead?
				cout << "[ERROR] Field not found!" << endl << endl;
			}
		}
		
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

class APP1Header{
	private:
		unsigned char segMarker[2];
		unsigned char size[2];
		unsigned char exifMarker[6];
	
	public:
		APP1Header(){
			memcpy(segMarker, app1Tag, 2);
			memcpy(exifMarker, exifID, 6);
		}
		
		// Sets the APP1 size bytes ((APP1 headers - 0xFFE1 marker) + IFD0 + EXIF IFD)
		void setSize(short newSize){
			size[0] = (newSize >> 8) & 0xFF;
			size[1] = newSize & 0xFF;
		}
		
		short getLength(){
			return 0x000a;
		}
		
		// TODO try to make this return byte array instead?
		vector<unsigned char> get(){
			vector<unsigned char> header;
			
			for(int i = 0; i < 2; i++)
				header.push_back(segMarker[i]);
			for(int i = 0; i < 2; i++)
				header.push_back(size[i]);
			for(int i = 0; i < 6; i++)
				header.push_back(exifMarker[i]);
				
			return header;
		}
};

class TIFFHeader{
	private:
		unsigned char endianess[2];
		unsigned char tiffMarker[2];
		unsigned char offset0IFD[4];
	
	public:
		// Constructor; arg for endianess (0 for little, 1 for big)
		TIFFHeader(short end){
			if(end == 0)
				memcpy(endianess, littleEndianID, 2);
			else if(end == 1)
				memcpy(endianess, bigEndianID, 2);
				
			memcpy(tiffMarker, tiffID, 2);
			
			unsigned char eightBytesOffset[4] = {0x00, 0x00, 0x00, 0x08};	// 8 byte offset to 0th IFD
			memcpy(offset0IFD, eightBytesOffset, 4);
		}
		
		short getLength(){
			return 0x0008;
		}
		
		vector<unsigned char> get(){
			vector<unsigned char> header;
			
			for(int i = 0; i < 2; i++)
				header.push_back(endianess[i]);
			for(int i = 0; i < 2; i++)
				header.push_back(tiffMarker[i]);
			for(int i = 0; i < 4; i++)
				header.push_back(offset0IFD[i]);
				
			return header;
		}
};

class APP1{
	public:
		APP1Header* app1Header;
		TIFFHeader* tiffHeader;
		IFD ifd0;
		IFD exifIFD;
		
		short app1TotalSize;
		
		APP1(){
			app1Header = new APP1Header();
			tiffHeader = new TIFFHeader(1);
		}
		
		// Return everything as byte vector to write to file
		vector<unsigned char> getApp1(){
			updateSegSize();
			
			vector<unsigned char> app1Bytes;
			
			// Add APP1 header
			vector<unsigned char> app1HeaderBytes = app1Header->get();
			app1Bytes.insert(end(app1Bytes), begin(app1HeaderBytes), end(app1HeaderBytes));
			
			// Add TIFF header
			vector<unsigned char> tiffHeaderBytes = tiffHeader->get();
			app1Bytes.insert(end(app1Bytes), begin(tiffHeaderBytes), end(tiffHeaderBytes));
			
			// Add 0IFD
			vector<unsigned char> ifd0Bytes = ifd0.get();
			app1Bytes.insert(end(app1Bytes), begin(ifd0Bytes), end(ifd0Bytes));
			
			// Add EXIF IFD
			vector<unsigned char> exifBytes = exifIFD.get();
			app1Bytes.insert(end(app1Bytes), begin(exifBytes), end(exifBytes));
			
			return app1Bytes;
		}
		
		// Updates bytes in APP1 header to reflect size
		void updateSegSize(){
			app1Header->setSize(getTotalSize() - 2);	// Subtract the 0xFFE1 marker
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
			
			headersSize += app1Header->getLength();
			headersSize += tiffHeader->getLength();
			
			return headersSize;
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
	
	APP1 app1;
	app1.ifd0.addField(ExifIFDField());	// Add EXIF Offset field to IFD0
	
	
	
	
	// TODO remove print IFD0
	cout << endl << "ifd0" << endl;
	vector<unsigned char> ifd = app1.ifd0.get();
	// Convert to byte array for writing
	unsigned char ifdBytes[ifd.size()];
	copy(ifd.begin(), ifd.end(), ifdBytes);
	printBytes(ifdBytes, ifd.size());
	cout << endl << endl;
	
	
	
	
	
	
	// Update EXIF Offset
	// TODO Make this a function
	int newOffset = app1.tiffHeader->getLength() + app1.ifd0.getSize();
	unsigned char newOffsetBytes[4];
	newOffsetBytes[0] = (newOffset >> 24) & 0xFF;
	newOffsetBytes[1] = (newOffset >> 16) & 0xFF;
	newOffsetBytes[2] = (newOffset >> 8) & 0xFF;
	newOffsetBytes[3] = newOffset & 0xFF;
	app1.ifd0.setFieldValue(exifIFDTag, newOffsetBytes);
	
	
	
	
	
	
	
	// TODO remove print IFD0
	cout << endl << "ifd0" << endl;
	ifd = app1.ifd0.get();
	// Convert to byte array for writing
	copy(ifd.begin(), ifd.end(), ifdBytes);
	printBytes(ifdBytes, ifd.size());
	cout << endl << endl;
	
	
	
	
	
	
	
	// TODO Add constructor with info?
	app1.exifIFD.addField(ApertureIFDField());
	app1.exifIFD.addField(ShutterSpeedIFDField());
	
	
	
	
	cout << app1.app1Header->getLength() << endl;
	
	
	// TODO Calculate offsets for data area in EXIF IFD, and EXIF OFFSET in IFD0, then update offsets
	
	
	
	
	
	
	// Export APP1 header
	vector<unsigned char> app = app1.getApp1();
	// Convert to byte array for writing
	unsigned char appBytes[app.size()];
	copy(app.begin(), app.end(), appBytes);
	
	
	prettyPrintAPP1(appBytes, app.size());	// TODO remove
	
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