//A C++ filestream module for reading fog script files

#include  <stdio.h>
#include  <stdlib.h>
#include  <math.h>
#include  <ctype.h>
#include  <string.h>
#include  <iostream.h>
#include  <iomanip> 
#include  <fstream>

using namespace std;

extern "C" struct r_fog
{
	float red;
	float green;
	float blue;
	float start;
	float end;
	float density;
} fog;

extern "C" void R_ReadFogScript(char config_file[128]);

extern "C" char map_music[128];

extern "C" void R_ReadMusicScript(char config_file[128]);

void R_ReadFogScript(char config_file[128])
{
	ifstream infile;
	char a_string[128];

	infile.open(config_file);

	if(infile.good()) {
		infile.getline(a_string, 128);
		fog.red = atof(a_string); 
		infile.getline(a_string, 128);
		fog.green = atof(a_string); 
		infile.getline(a_string, 128);
		fog.blue = atof(a_string); 
		infile.getline(a_string, 128);
		fog.start = atof(a_string); 
		infile.getline(a_string, 128);
		fog.end = atof(a_string); 
		infile.getline(a_string, 128);
		fog.density = atof(a_string); 
		infile.close();
	}

	return;
}

void R_ReadMusicScript(char config_file[128])
{
	ifstream infile;

	infile.open(config_file);

	if(infile.good()) {
		infile.getline(map_music, 128);
		infile.close();
	}

	return;

}
