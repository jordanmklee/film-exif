#include<iostream>
#include<fstream>
#include<sstream>
#include<string>

using namespace std;

struct Frame{
	int frameNumber;
	// aperture and shutterSpeed are stored as strings as they do not need to be evaluated on,
	// and are therefore easier to write to XML
	string aperture;
	string shutterSpeed;
};

string frameToXml(Frame exposure){
	
	string xml = "\t<exp>\n";
	xml.append("\t\t<frameNumber>" + to_string(exposure.frameNumber) + "</frameNumber>\n");
	xml.append("\t\t<aperture>" + exposure.aperture + "</aperture>\n");
	xml.append("\t\t<shutterSpeed>" + exposure.shutterSpeed + "</shutterSpeed>\n");
	xml.append("\t</exp>\n");
	
	return xml;
}

int main(){
	
	// Welcome message
	cout << "===[ film-exif Metadata Recording Tool ]===" << endl;
	cout << "Recording for a new roll of film..." << endl;
	cout << "Enter \"0\" at any time to quit" << endl << endl;
	
	// Open file stream to save xml data
	ofstream xml;
	xml.open("roll.xml");
	xml << "<roll>\n";
	
	// Manually enter frame information
	bool cont = true;
	int frameNumber = 0;
	while(cont){
		string aperture;
		string shutterSpeed;
		
		// Get user input for aperture and shutter speed info
		// Note: Does not check for valid aperture and shutter speed values!
		// TODO Change to a menu system instead of asking for specific values
		cout << "Enter the aperture of the exposure:" << endl;
		getline(cin, aperture);
		
		if(aperture.compare("0") == 0){
			cout << "Quitting..." << endl << endl;
			cont = false;
			break;
		}
		
		cout << "Enter the shutter speed of the exposure:" << endl;
		getline(cin, shutterSpeed);
		
		if(shutterSpeed.compare("0") == 0){
			cout << "Quitting..." << endl << endl;
			cont = false;
			break;
		}
		cout << "Recorded frame [" << frameNumber << "] with f/" << aperture << " and " << shutterSpeed << "s" << endl;
		cout << "=======================================\n\n\n" << endl;
		
		
		
		// Create new frame struct for writing
		Frame exposure;
		exposure.aperture = aperture;
		exposure.shutterSpeed = shutterSpeed;
		exposure.frameNumber = frameNumber;
		
		// Convert exposure to XML and write to file
		xml << (frameToXml(exposure));
		frameNumber++;
	}
	
	// Write closing roll XML tag and close file stream
	xml << "</roll>";
	xml.close();
	
	return 0;
}

