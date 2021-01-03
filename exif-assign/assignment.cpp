#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<dirent.h>
#include<regex>

#include "app1.h"

using namespace std;

struct XmlFrame{
	int aperture;
	int shutterSpeed;
};

// Removes the leading whitespace before a line in XML file
string removeLeadingWhitespace(string line){	
	bool whitespace = true;
	int i = 0;
	while(whitespace){
		if(line[i] != '\t'){
			line = line.substr(i, line.length()-i);
			whitespace = false;
		}
		i++;
	}
	return line;
}

// Returns the info between XML tags as a string
// eg. <xml>info</xml> returns "xml"
string removeXmlTags(string line, string tag){
	line = line.substr(0, (line.length() - tag.length() - 1));			// Remove end tag (-1 for the extra backslash)
	line = line.substr(tag.length(), (line.length() - tag.length()));	// Remove start tag
	return line;
}

// Parse XML file into a vector of XmlFrame elements
vector<XmlFrame> parseXml(string filepath){
	vector<XmlFrame> roll;
	
	ifstream xml(filepath);
	if(xml.is_open()){
		
		string line;
		while(getline(xml, line)){		
			line = removeLeadingWhitespace(line);
			
			// If current line is an "exp" tag, parse the exposure
			if(line.compare("<exp>") == 0){
				XmlFrame newFrame;
				
				// Parse frameNumber
				getline(xml, line);
				
				// Parse aperture
				getline(xml, line);
				newFrame.aperture = stod(removeXmlTags(removeLeadingWhitespace(line), "<aperture>"));

				// Parse shutterspeed
				getline(xml, line);
				newFrame.shutterSpeed = stod(removeXmlTags(removeLeadingWhitespace(line), "<shutterSpeed>"));
				
				// Add newly parsed exposure information to vector
				roll.push_back(newFrame);
			}	
		}
		xml.close();
	}
	else{
		perror("Could not open XML file");
		exit(0);
	}
	
	return roll;
}

// Deletes existing metadata from input JPG and produces an output JPG with
// metadata generated from XmlFrame
// Filepaths are assumed to be correct (checked in main() function)
void writeMetadata(string inFilepath, string outFilepath, XmlFrame metadata){
	// Open JPGs as binary file
	ifstream jpg(inFilepath, ios::binary | ios::ate);
	ofstream jpgExif(outFilepath, ios::binary | ios::trunc);
	
	// Get input JPG filesize in bytes
	int filesize = jpg.tellg();
	jpg.seekg(0, ios::beg);		// Reset for reading
	
	unsigned char buf[2];
	
	// Read SOI (0xFFD8) from input JPG and write to output JPG
	jpg.read((char*)&buf, 2);
	jpgExif.write((char*)buf, 2);
	
	
	// Create APP1 segment with metadata
	APP1 app1;
	app1.addMetadata(apertureIFDTag, metadata.aperture);
	app1.addMetadata(shutterSpeedIFDTag, metadata.shutterSpeed);
	unsigned char appBytes[app1.getSize()];
	app1.get(appBytes);		// APP1 segment exported to appBytes byte array for writing
	
	
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
}

// Returns a vector of filenames from a directory path
// TODO make sure vector is ordered correctly
vector<string> getFilenames(const char* path){
	vector<string> filenames;
	
	DIR* dir;
	struct dirent *diread;
	if((dir = opendir(path)) != NULL){
		while((diread = readdir(dir)) != NULL){
			string filename = diread->d_name;
			
			// Adds filename to vector if it is a JPG file
			if(regex_match(filename, regex("[a-zA-Z0-9]+\\.(jpe?g|JPE?G)") ))
				filenames.push_back(filename);
		}
	}
	else{
		perror("Could not open image directory");	// Error, directory could not be opened
		exit(0);
	}
	
	closedir(dir);
	return filenames;
}

int main(int argc, char* argv[]){
	
	// Parse arguments
	if(argc != 4){
		cout << "Usage: " << argv[0] << " <xml-filepath> <images-directory> <output-directory>" << endl;
		return 0;
	}
	string imgPath = argv[2];
	string outPath = argv[3];
	
	// Verify that file or directory exists; quits program if cannot be opened
	vector<XmlFrame> roll = parseXml(argv[1]);
	vector<string> filenames = getFilenames(imgPath.c_str());
	getFilenames(outPath.c_str());
	
	
	// Check if number of XML entries match the number of files to be assigned metadata
	if(roll.size() != filenames.size()){
		printf("There are %lu recorded exposures but there are %lu image files.\n", roll.size(), filenames.size());
		
		if(roll.size() > filenames.size()){
			int leftover = roll.size() - filenames.size();
			printf("%d recorded exposures will not be assigned to files.\n", leftover);
		}
		else{
			int missing = filenames.size() - roll.size();
			printf("%d image files will not be assigned metadata.\n", missing);
		}
		printf("Continue anyways? (y/n)\n");
		
		char forceContinue;
		cin >> forceContinue;
		if(forceContinue != 'y'){
			printf("Quitting...\n\n");
			return 0;
		}
	}
	
	// Assign metadata to files
	for(int i = 0; i < filenames.size(); i++){
		string filename = filenames.at(i);
		string inFilepath = imgPath + "/" + filename;
		string outFilepath = outPath + "/" + filename;
		
		if(i > roll.size())	// No more recorded exposures, quit
			break;
		
		// Status messages
		printf("\nAssigning Exif (%d of %lu)\n", (i+1), filenames.size());
		printf("\tInput:\t\t%s\n", inFilepath.c_str());
		printf("\tOutput:\t\t%s\n\n", outFilepath.c_str());
		printf("\tAperture:\t%f\n", (roll.at(i).aperture / 10.0));
		printf("\tShutter Speed:\t%f\n\n", (roll.at(i).shutterSpeed / 1000.0));
		
		// Write to output file
			writeMetadata(inFilepath, outFilepath, roll.at(i));
		
	}

	return 0;
}