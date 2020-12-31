#include<fstream>
#include<iostream>
#include<vector>

// EXIF/TIFF Tags
unsigned char exifIFDTag[2] = {0x87, 0x69};
unsigned char apertureIFDTag[2] = {0x82, 0x9D};
unsigned char shutterSpeedIFDTag[2] = {0x82, 0x9A};

// IFD
unsigned char ifdOffset[4] = {0x00, 0x00, 0x00, 0x00};

// APP1 Header
unsigned char app1Tag[2] = {0xFF, 0xE1};
unsigned char exifID[6] = {0x45, 0x78, 0x69, 0x66, 0x00, 0x00};	// "Exif  "

// TIFF Header
unsigned char littleEndianID[2] = {0x49, 0x49};	// "II"
unsigned char bigEndianID[2] = {0x4D, 0x4D};	// "MM"
unsigned char tiffID[2] = {0x00, 0x2A};			// 42

using namespace std;

// Prints byte array in 2-byte format
void printBytes(unsigned char arr[], int len){
	for(int i = 0; i < len; i+=2){
		printf("%02x", arr[i]);
		printf("%02x", arr[i+1]);
		cout << " ";
	}
}

// Hardcoded printing of APP1 segment
// TODO Remove or rewrite to account for variable size segments
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

// Converts int to byte array
// Must create an byte[4] buffer where byte array is needed
void intToByteArray(int i, unsigned char* bytes){
	bytes[0] = (i >> 24) & 0xFF;
	bytes[1] = (i >> 16) & 0xFF;
	bytes[2] = (i >> 8) & 0xFF;
	bytes[3] = i & 0xFF;
}

class IFDField{
	private:
		unsigned char tagID[2];
		unsigned char typeID[2];
		unsigned char count[4];
		unsigned char value[4];
		
	public:
		// Return field as a vector of bytes
		vector<unsigned char> get(){
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
		
		// Set field Tag ID
		void setTagID(unsigned char newTag[]){
			memcpy(tagID, newTag, 2);
		}
		
		// Set field Type ID
		void setTypeID(unsigned char newType[]){
			memcpy(typeID, newType, 2);
		}
		
		// Sets field component count
		void setCount(unsigned char newCount[]){
			memcpy(count, newCount, 4);
		}
		
		// Sets field value or offset (from beginning of TIFF header)
		void setValue(unsigned char newValue[]){
			memcpy(value, newValue, 4);
		}
		
		// Returns the value
		unsigned char* getValue(){
			return value;
		}
		
		// Checks if this field has tagID
		bool isTag(unsigned char id[]){
			if(tagID[0] == id[0] && tagID[1] == id[1])
				return true;
			
			return false;
		}
};

class IFDFieldWithData : public IFDField{
	private:
		vector<unsigned char> data;
	
	public:
		// Adds byte array to data vector
		void setData(unsigned char newData[], int dataLength){
			for(int i = 0; i < dataLength; i++){
				data.push_back(newData[i]);
			}
		}
		
		// Returns the data vector
		vector<unsigned char> getData(){
			return data;
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

class ApertureIFDField : public IFDFieldWithData{
	private:
		unsigned char apertureNumerator[4];
		unsigned char apertureDenominator[4];
	
	public:
		// Creates an aperture IFD field for the EXIF IFD and adds aperture information
		// to the data area
		// Offset needs to be calculated before field is added to IFD, then set after
		// the field is added using IFDFieldWithData.setValue()
		ApertureIFDField(int aperture){
			unsigned char type[2] = {0x00, 0x05};				// Type 5 = rational
			unsigned char count[4] = {0x00, 0x00, 0x00, 0x01};	// Count = 1
			
			setTagID(apertureIFDTag);
			setTypeID(type);
			setCount(count);
			setApertureData(aperture);
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
			
			// Switch recognizes full f-stops
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
		// Creates a shutter speed IFD field for the EXIF IFD and adds shutter
		// speed information to the data area
		// Offset needs to be calculated before field is added to IFD, then set after
		// the field is added using IFDFieldWithData.setValue()
		ShutterSpeedIFDField(int shutterSpeed){
			unsigned char type[2] = {0x00, 0x05};				// Type 5 = rational
			unsigned char count[4] = {0x00, 0x00, 0x00, 0x01};	// Count = 1
			
			setTagID(shutterSpeedIFDTag);
			setTypeID(type);
			setCount(count);
			setShutterSpeedData(shutterSpeed);
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
			
			// Standard shutter speeds from 1/1000s to 1s are recognized
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
		short numFields;				// short (2 bytes) so it can be incremented
		vector<IFDField> fields;
		unsigned char offsetNextIFD[4];
		vector<unsigned char> dataArea;
		
	public:
		// Constructor
		IFD(){
			numFields = 0x0000;
			memcpy(offsetNextIFD, ifdOffset, 4);	// Set offset to next IFD to default (const 0x0000 0000)
		}
		
		// Returns entire IFD as a vector of bytes
		vector<unsigned char> get(){
			vector<unsigned char> ifdBytes;
			
			// Add numFields as bytes
			ifdBytes.push_back((numFields >> 8) & 0xFF);
			ifdBytes.push_back(numFields & 0xFF);
			
			// Add each field
			for(int i = 0; i < fields.size(); i++){
				vector<unsigned char> field = fields.at(i).get();
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
			updateOffsets();
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
			
			updateOffsets();
		}
		
		// Updates offsets after addition of new field
		void updateOffsets(){
			// Last field in vector was just added, the offset is already correct
			// so it does not need to be updated
			for(int i = 0; i < fields.size() - 1; i++){
				
				// Checks if field value is an offset; adding a field changes this offset
				if(	fields.at(i).isTag(exifIFDTag) || 
					fields.at(i).isTag(apertureIFDTag) || 
					fields.at(i).isTag(shutterSpeedIFDTag)){
					// Convert value to int for addition
					int newOffset = (	fields.at(i).getValue()[0] << 24 |
										fields.at(i).getValue()[1] << 16 |
										fields.at(i).getValue()[2] << 8 |
										fields.at(i).getValue()[3]);
					
					newOffset += 12;	// New field adds 12 bytes
					
					// Convert newOffset back to byte array for writing
					unsigned char newOffsetBytes[4];
					intToByteArray(newOffset, newOffsetBytes);
					
					fields.at(i).setValue(newOffsetBytes);		// Write new offset to field value
				}
			}
		}
		
		// Sets the value of a field by tagID
		void setFieldValue(unsigned char tagID[], unsigned char newValue[]){
			for(int i = 0; i < fields.size(); i++){
				// Updates value of field if found
				if(fields.at(i).isTag(tagID)){
					fields.at(i).setValue(newValue);
					return;
				}
			}
			
			// Field with field tag not found
			cout << "[ERROR] Field not found!" << endl;	// TODO produce error instead?
		}
		
		// Returns size of IFD
		short getSize(){
			short ifdSize = 0x00;
			
			ifdSize += sizeof(numFields);
			ifdSize += (numFields * 12);		// Each IFD field is 12 bytes long
			ifdSize += sizeof(offsetNextIFD);
			ifdSize += dataArea.size();
			
			return ifdSize;
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
		
		// Returns the length of the APP1 header
		short getSize(){
			return 0x000A;	// 2 + 2 + 6 = 10 = 0xA
		}
		
		// Returns APP1 header as a vector of bytes
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
		
		// Returns the length of the TIFF header
		short getSize(){
			return 0x0008;	// 2 + 2 + 4 = 8 = 0x8
		}
		
		// Returns the TIFF header as a vector of bytes
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
	private:
		APP1Header* app1Header;
		TIFFHeader* tiffHeader;
		IFD ifd0;
		IFD exifIFD;
		
	public:
		// Creates a new APP1 object with the following:
		// APP1 header
		// TIFF header set to big-endian
		// IFD0 (with EXIF Offset field, pointing to EXIF IFD)
		// EXIF IFD (with no fields)
		APP1(){
			app1Header = new APP1Header();
			tiffHeader = new TIFFHeader(1);	// 1 for big-endian
			
			// Add EXIF Offset field to IFD0, and set the value
			ifd0.addField(ExifIFDField());
			
			unsigned char intBuffer[4];	// Buffer for int conversion
			int exifOffset = tiffHeader->getSize() + ifd0.getSize();
			intToByteArray(exifOffset, intBuffer);
			ifd0.setFieldValue(exifIFDTag, intBuffer);
		}
		
		// Returns entire APP1 segment as a vector of bytes
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
		
		// Updates bytes in APP1 header to reflect current size
		void updateSegSize(){
			app1Header->setSize(getSize() - 2);	// Subtract the APP1 marker (0xFFE1)
		}
		
		// Get size of entire APP1 (APP1 headers, IFD0, EXIF IFD) in bytes (including APP1 marker)
		short getSize(){
			short size = 0x0000;
			
			size += app1Header->getSize();
			size += tiffHeader->getSize();
			size += ifd0.getSize();
			size += exifIFD.getSize();
			
			return size;
		}
		
		// Computes the next offset where data can be entered in the EXIF IFD
		// Note: must be computed before adding the next field with data!
		short getNextDataOffset(){
			// -app1Header since offset is calculated starting at the TIFF header
			// +12 to account for the new field being added
			return getSize() - app1Header->getSize() + 12;
		}
		
		// Adds EXIF metadata to APP1 segment
		// tagID - EXIF tag ID of the kind of metadata
		// value - EXIF data defined by film-exif (see documentation)
		void addMetadata(unsigned char tagID[], int value){
			// Get offset to next available data area as byte array
			unsigned char intBuffer[4];
			intToByteArray(getNextDataOffset(), intBuffer);
			
			// Add field to EXIF IFD corresponding to tagID, with arg value
			if(	tagID[0] == apertureIFDTag[0] &&
				tagID[1] == apertureIFDTag[1])
				exifIFD.addField(ApertureIFDField(value));
			else if(tagID[0] == shutterSpeedIFDTag[0] &&
					tagID[1] == shutterSpeedIFDTag[1])
				exifIFD.addField(ShutterSpeedIFDField(value));
			
			// Set offset to point to correct data
			exifIFD.setFieldValue(tagID, intBuffer);
		}
};


int main(){
	cout << hex;				// Print in hex
	
	// Create APP1 segment
	APP1 app1;
	app1.addMetadata(apertureIFDTag, 14);
	app1.addMetadata(shutterSpeedIFDTag, 125);
	
	// Export APP1
	vector<unsigned char> app = app1.getApp1();
	
	// Convert to byte array for writing
	unsigned char appBytes[app.size()];
	copy(app.begin(), app.end(), appBytes);
	
	prettyPrintAPP1(appBytes, app.size());	// TODO remove
	
	
	
	
	
	
	
	
	
	
	
	// Open JPG as binary file
	ifstream jpg("./img/img016.jpg", ios::binary | ios::ate);
	ofstream jpgExif("./img/img016exif.jpg", ios::binary | ios::trunc);
	
	
	// Get filesize in bytes
	int filesize = jpg.tellg();
	jpg.seekg(0, ios::beg);		// Reset for reading
	
	
	// Read file in 2-byte chunks
	unsigned char buf[2];		// 2 byte buffer
	for(int i = 0; i < 10; i++){		// TODO hardcoded, need to look for end of APP0
		jpg.read((char*)&buf, 2);
		jpgExif.write((char*)buf, 2);
	}
	
	// Write APP1
	jpgExif.write((char*)&appBytes, app1.getSize());
	
	
	// Write rest of the file
	for(int i = 10; i < filesize; i++){
		jpg.read((char*)&buf, 2);
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