#include<iostream>
#include<fstream>
#include<string>
#include<vector>

using namespace std;

struct Frame{
	double aperture;
	double shutterSpeed;
};

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

string removeXmlTags(string line, string tag){
	line = line.substr(0, (line.length() - tag.length() - 1));			// Remove end tag (-1 for the extra backslash)
	line = line.substr(tag.length(), (line.length() - tag.length()));	// Remove start tag
	return line;
}

int main(int argc, char* argv[]){
	
	// TODO arg handling; check for valid xml file and filepath
	if(argc <= 2){
		cout << "Usage: " << argv[0] << " <xml> <filepath>" << endl;
		return 0;
	}
	
	vector<Frame> roll;
	
	// Parse input XML file
	ifstream xml(argv[1]);
	if(xml.is_open()){
		
		string line;
		while(getline(xml, line)){
			
			line = removeLeadingWhitespace(line);
			
			// If current line is an "exp" tag, parse the exposure
			if(line.compare("<exp>") == 0){
				Frame newFrame;
				
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
				cout << endl;
			}
			
			
		}
		xml.close();
	}
	else{
		cout << "Error: file does not exist!" << endl;
		return 0;
	}
	
	// TODO remove this
	// Write out roll vector's information
	for(int i = 0; i < roll.size(); i++){
		cout << roll.at(i).aperture << endl << roll.at(i).shutterSpeed << endl << endl;
	}


	// TODO Assign tags to files
	
	return 0;
}