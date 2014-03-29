/*************************************************************
 *
 * John Donovan
 *
 *
 *
 **************************************************************
 * Special thanks to:
 *      +   Swiftless's tutorial for getting mouseMotion
 *          working with looking around
 *
 *      +   Nature Wizard Tutorial 8 for the particle
 *          engine!
 **************************************************************/
using namespace std;

#include <cmath>
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <vector>
#include <fstream>
#include <string>
#include <vector>
#include<unistd.h>
//#include "character.h"
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

//Custom Header Files
#include "Constants.h"
#include "Particle.h"
#include "BitMapFile.h"
#include "LockableDoor.h"


/* GLOBALS */
bool* keyStates = new bool[256];

//for lookat
static float eyeX = 0.0;
static float eyeY = 2.0;
static float eyeZ = -5.0;
static float centerX = 0.0;
static float centerY = 2.0;
static float centerZ = -10.0;
static float theta = 90.0;
static float phi = 0.0;
static float upX = 0.0;
static float upY = 1.0;
static float upZ = 0.0;
static bool sprint = false;
static bool croutch = false;
static bool slowWalk = false;

static int animationPeriod = 100;
//TEXTURE GLOABLS

int wrap = 1;
static unsigned int texture[10];

static int lastX = 500;
static int lastY = 500;
//for FOG
static int fogMode = GL_LINEAR; // Fog mode.
static float fogDensity = 0.01; // Fog density.
static float fogStart = 0.0; // Fog start z value.
static float fogEnd = 100.0; // Fog end z value.
static float fogColor[4] = {0.25, 0.2, 0.2, 1.0};



//lighting
static int localViewer = 1; // Local viewpoint?
static bool flashLightOn = false;
static float flashLightBattery = 100.0;
//for looking arround
static float fire = 0.0;

static float openFactor = 0.0;
//PICKING AND SELECTING VARIABLES
static int hits; // Number of entries in hit buffer.
static unsigned int buffer[1024]; // Hit buffer.
static unsigned int closestName = 0; // Name of closest hit.

static int DoorCode[3] = {1,0,1};
static int keypad[3] = {0,0,0};


//Person Attributes
static bool isJumping=false;
static float jumpVal= JMP;
string myName;

//static particle testParticle= *(new particle());
static particle ourParticles[MAXPARTICLES];


lockableDoor lockedDoors[3];


void evolveParticle(int i)
{
	ourParticles[i].lifetime = ourParticles[i].lifetime - ourParticles[i].decay;
	if(ourParticles[i].lifetime <= 0 || ourParticles[i].ypos + ourParticles[i].yspeed >=1.0)
	{
		ourParticles[i].lifetime = 100;
		ourParticles[i].xpos = 15.0 + float(rand()%15);
		ourParticles[i].zpos = -32.0 - float(rand()%4);
		ourParticles[i].ypos = 0.0;
	}
	ourParticles[i].xpos = ourParticles[i].xpos + ourParticles[i].xspeed;
	ourParticles[i].ypos = ourParticles[i].ypos + ourParticles[i].yspeed;
	ourParticles[i].zpos = ourParticles[i].zpos + ourParticles[i].zspeed;
	fire += 0.5;
	ourParticles[i].xspeed = sin((fire*PI)/180.0);
}

void drawParticle(particle i)
{
	float emitLight[] = {1.0,1.0,1.0,1.0};
	float emitNoLight[] = {0.0,0.0,0.0,1.0};
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emitLight);
	float delta=.5*(1.0-i.ypos)/1.0;
	glPushMatrix();
	glColor3f(1.0,1.0,1.0);
	glBindTexture(GL_TEXTURE_2D, texture[8]);
	glFrontFace(GL_CCW);
	glBegin(GL_TRIANGLE_STRIP);
	glNormal3f(0.0,0.0,-1.0);glTexCoord2f((30-i.xpos-delta)/60.0, (10-i.ypos-delta)/10); glVertex3f(i.xpos-delta, i.ypos - delta, i.zpos);
	glNormal3f(0.0,0.0,-1.0);glTexCoord2f((30-i.xpos+delta)/60.0, (10-i.ypos-delta)/10);glTexCoord2f(1.0,0.0); glVertex3f(i.xpos+delta, i.ypos-delta, i.zpos);
	glNormal3f(0.0,0.0,-1.0);glTexCoord2f((30-i.xpos)/60.0, (10-i.ypos+delta)/10);glTexCoord2f(1.0,1.0) ;glVertex3f(i.xpos, i.ypos+delta, i.zpos);
	glEnd();
	glFrontFace(GL_CW);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emitNoLight);
	glPopMatrix();
}


void createParticle(int i)
{
	ourParticles[i] = *(new particle());
}

BitMapFile *getBMPData(string filename)
{
    BitMapFile *bmp = new BitMapFile;
    unsigned int size, offset, headerSize;
    
    // Read input file name.
    ifstream infile(filename.c_str(), ios::binary);
    if(infile)
        cout << "file named : " << filename << " exists" << endl;
    else
        cout << "file named : " << filename << " does not exist" << endl;
    
    // Get the starting point of the image data.
    infile.seekg(10);
    infile.read((char *) &offset, 4);
    
    // Get the header size of the bitmap.
    infile.read((char *) &headerSize,4);
    
    // Get width and height values in the bitmap header.
    infile.seekg(18);
    infile.read( (char *) &bmp->sizeX, 4);
    infile.read( (char *) &bmp->sizeY, 4);
    
    // Allocate buffer for the image.
    size = bmp->sizeX * bmp->sizeY * 24;
    bmp->data = new unsigned char[size];
    
    // Read bitmap data.
    infile.seekg(offset);
    infile.read((char *) bmp->data , size);
    
    // Reverse color from bgr to rgb.
    int temp;
    for (int i = 0; i < size; i += 3)
    {
        temp = bmp->data[i];
        bmp->data[i] = bmp->data[i+2];
        bmp->data[i+2] = temp;
    }
    return bmp;
}


void initLocks()
{
	for(int i=0; i < 3; i++)
	{
		lockedDoors[i].locked = true;
		lockedDoors[i].openFactor = 0.0;
	}
}


void smartLoadExternalTextures()
{
    BitMapFile *image;
    string fileNames[] = {"rock.bmp", "grass.bmp", "brickwork_texture.bmp",
                        "metalDoor.bmp", "metalPanel.bmp", "grungeWalls.bmp",
                        "interiorFloors.bmp", "tile.bmp", "fire.bmp", "keycard.bmp"
    };
    vector<string> filevec(fileNames, fileNames + sizeof(fileNames) / sizeof(string));
    vector<string>::iterator itr = filevec.begin();
    while(itr != filevec.end())
    {
        image = getBMPData(*itr);
        // Bind wood image to texture index[0].
        glBindTexture(GL_TEXTURE_2D, texture[0]);
        // if wrap is true, the texture repeats, ignoring integer part
        //	... false, the texture does not go beyond 1.
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap ? GL_REPEAT : GL_CLAMP );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap ? GL_REPEAT : GL_CLAMP );
        // select modulate to mix texture with color for shading
        glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
        // when texture area is small, bilinear filter the closest mipmap
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
        // when texture area is large, bilinear filter the first mipmap
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        // when texture area is small, bilinear filter the closest mipmap
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
        // when texture area is large, bilinear filter the first mipmap
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->sizeX, image->sizeY, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, image->data);
        // build our texture mipmaps
        gluBuild2DMipmaps( GL_TEXTURE_2D, 3, image->sizeX, image->sizeY,GL_RGB, GL_UNSIGNED_BYTE, image->data );
        ++itr;
    }
}

// Load external textures.
void loadExternalTextures()
{
    // Local storage for bmp image data.
    BitMapFile *image[10];
    
    // Load the textures.
    //first texture
    string path = "/Users/johndonovan/Desktop/bmp/rock.bmp";
    image[0] = getBMPData(path);
    
    // Bind wood image to texture index[0].
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    // if wrap is true, the texture repeats, ignoring integer part
    //	... false, the texture does not go beyond 1.
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap ? GL_REPEAT : GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap ? GL_REPEAT : GL_CLAMP );
    
    // select modulate to mix texture with color for shading
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image[0]->sizeX, image[0]->sizeY, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, image[0]->data);
    
    // build our texture mipmaps
    gluBuild2DMipmaps( GL_TEXTURE_2D, 3, image[0]->sizeX, image[0]->sizeY,GL_RGB, GL_UNSIGNED_BYTE, image[0]->data );
    
    
    //second texture Grass
    image[1] = getBMPData("/Users/johndonovan/Desktop/bmp/grass.bmp");
   
    
    // Bind smallgrass image to texture index[1].
    glBindTexture(GL_TEXTURE_2D, texture[1]);
    // if wrap is true, the texture repeats, ignoring integer part
    //	... false, the texture does not go beyond 1.
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap ? GL_REPEAT : GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap ? GL_REPEAT : GL_CLAMP );
    
    // select modulate to mix texture with color for shading
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image[1]->sizeX, image[1]->sizeY, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, image[1]->data);
    
    // build our texture mipmaps
    gluBuild2DMipmaps( GL_TEXTURE_2D, 3, image[1]->sizeX, image[1]->sizeY,GL_RGB, GL_UNSIGNED_BYTE, image[1]->data );
    
    //Third texture Brick
    image[2] = getBMPData("/Users/johndonovan/Desktop/bmp/brickwork-texture.bmp");
    
    // Bind smallgrass image to texture index[1].
    glBindTexture(GL_TEXTURE_2D, texture[2]);
    // if wrap is true, the texture repeats, ignoring integer part
    //	... false, the texture does not go beyond 1.
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap ? GL_REPEAT : GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap ? GL_REPEAT : GL_CLAMP );
    
    // select modulate to mix texture with color for shading
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image[2]->sizeX, image[2]->sizeY, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, image[2]->data);
    
    // build our texture mipmaps
    gluBuild2DMipmaps( GL_TEXTURE_2D, 3, image[2]->sizeX, image[2]->sizeY,GL_RGB, GL_UNSIGNED_BYTE, image[2]->data );
    
    //Fourth texture Metal Door
    image[3] = getBMPData("/Users/johndonovan/Desktop/bmp/metalDoor.bmp");
    
    // Bind smallgrass image to texture index[1].
    glBindTexture(GL_TEXTURE_2D, texture[3]);
    // if wrap is true, the texture repeats, ignoring integer part
    //	... false, the texture does not go beyond 1.
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap ? GL_REPEAT : GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap ? GL_REPEAT : GL_CLAMP );
    
    // select modulate to mix texture with color for shading
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image[3]->sizeX, image[3]->sizeY, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, image[3]->data);
    
    // build our texture mipmaps
    gluBuild2DMipmaps( GL_TEXTURE_2D, 3, image[3]->sizeX, image[3]->sizeY,GL_RGB, GL_UNSIGNED_BYTE, image[3]->data );
    
    //Fifth texture for door Panel
    image[4] = getBMPData("/Users/johndonovan/Desktop/bmp/metalPanel.bmp");
    
    // Bind smallgrass image to texture index[1].
    glBindTexture(GL_TEXTURE_2D, texture[4]);
    // if wrap is true, the texture repeats, ignoring integer part
    //	... false, the texture does not go beyond 1.
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap ? GL_REPEAT : GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap ? GL_REPEAT : GL_CLAMP );
    
    // select modulate to mix texture with color for shading
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image[4]->sizeX, image[4]->sizeY, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, image[4]->data);
    
    // build our texture mipmaps
    gluBuild2DMipmaps( GL_TEXTURE_2D, 3, image[4]->sizeX, image[4]->sizeY,GL_RGB, GL_UNSIGNED_BYTE, image[4]->data );
    
    //**
    
    //Sixth texture for grungyWalls
    image[5] = getBMPData("/Users/johndonovan/Desktop/bmp/grungeWall.bmp");
    
    // Bind smallgrass image to texture index[1].
    glBindTexture(GL_TEXTURE_2D, texture[5]);
    // if wrap is true, the texture repeats, ignoring integer part
    //	... false, the texture does not go beyond 1.
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap ? GL_REPEAT : GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap ? GL_REPEAT : GL_CLAMP );
    
    // select modulate to mix texture with color for shading
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image[5]->sizeX, image[5]->sizeY, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, image[5]->data);
    
    // build our texture mipmaps
    gluBuild2DMipmaps( GL_TEXTURE_2D, 3, image[5]->sizeX, image[5]->sizeY,GL_RGB, GL_UNSIGNED_BYTE, image[5]->data );
    
    //**
    
    //Seventh texture for interior floor
    image[6] = getBMPData("/Users/johndonovan/Desktop/bmp/interiorFloor.bmp");
    
    // Bind smallgrass image to texture index[1].
    glBindTexture(GL_TEXTURE_2D, texture[6]);
    // if wrap is true, the texture repeats, ignoring integer part
    //	... false, the texture does not go beyond 1.
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap ? GL_REPEAT : GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap ? GL_REPEAT : GL_CLAMP );
    
    // select modulate to mix texture with color for shading
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image[6]->sizeX, image[6]->sizeY, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, image[6]->data);
    
    // build our texture mipmaps
    gluBuild2DMipmaps( GL_TEXTURE_2D, 3, image[6]->sizeX, image[6]->sizeY,GL_RGB, GL_UNSIGNED_BYTE, image[6]->data );
    
    //**
    
    //Eight texture for interior cieling
    image[7] = getBMPData("/Users/johndonovan/Desktop/bmp/tile.bmp");
    
    // Bind smallgrass image to texture index[1].
    glBindTexture(GL_TEXTURE_2D, texture[7]);
    // if wrap is true, the texture repeats, ignoring integer part
    //	... false, the texture does not go beyond 1.
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap ? GL_REPEAT : GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap ? GL_REPEAT : GL_CLAMP );
    
    // select modulate to mix texture with color for shading
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image[7]->sizeX, image[7]->sizeY, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, image[7]->data);
    
    // build our texture mipmaps
    gluBuild2DMipmaps( GL_TEXTURE_2D, 3, image[7]->sizeX, image[7]->sizeY,GL_RGB, GL_UNSIGNED_BYTE, image[7]->data );
    
    //Ninth texture for interior cieling
    image[8] = getBMPData("/Users/johndonovan/Desktop/bmp/fire.bmp");
    
    // Bind smallgrass image to texture index[1].
    glBindTexture(GL_TEXTURE_2D, texture[8]);
    // if wrap is true, the texture repeats, ignoring integer part
    //	... false, the texture does not go beyond 1.
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap ? GL_REPEAT : GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap ? GL_REPEAT : GL_CLAMP );
    
    // select modulate to mix texture with color for shading
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image[8]->sizeX, image[8]->sizeY, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, image[8]->data);
    
    // build our texture mipmaps
    gluBuild2DMipmaps( GL_TEXTURE_2D, 3, image[8]->sizeX, image[8]->sizeY,GL_RGB, GL_UNSIGNED_BYTE, image[8]->data );
    
    //Tenth texture for interior cieling
    image[9] = getBMPData("/Users/johndonovan/Desktop/bmp/keycard.bmp");
    
    // Bind smallgrass image to texture index[1].
    glBindTexture(GL_TEXTURE_2D, texture[9]);
    // if wrap is true, the texture repeats, ignoring integer part
    //	... false, the texture does not go beyond 1.
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap ? GL_REPEAT : GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap ? GL_REPEAT : GL_CLAMP );
    
    // select modulate to mix texture with color for shading
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image[9]->sizeX, image[9]->sizeY, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, image[9]->data);
    
    // build our texture mipmaps
    gluBuild2DMipmaps( GL_TEXTURE_2D, 3, image[9]->sizeX, image[9]->sizeY,GL_RGB, GL_UNSIGNED_BYTE, image[9]->data );
    
    
}


bool isInWorld()
{
	float radiusFromCenter = sqrt( (eyeX-0)*(eyeX-0) + (eyeZ+52)*(eyeZ+52) );
	//float buffer=-3.0;
	//float *dirVec = getDirectionVector();
    
	
	if (radiusFromCenter /*+ buffer */ >= 48.0)
	{
        
		return false;
	}
	
	return true;
}

float *getDirectionVector3()
{
	float *dirVec = new float[3];
	/*dirVec[0] = 1.0;
     dirVec[1] = 2.0;
     dirVec[2] = 3.0;*/
	float radius = sqrt(((centerX-eyeX)*(centerX-eyeX))+((centerZ-eyeZ)*(centerZ-eyeZ))+((centerY-eyeY)*(centerY-eyeY)));
	//x
	dirVec[0] = (centerX- eyeX)/radius;
	dirVec[1] = (centerY-eyeY)/radius;
	dirVec[2] = (centerZ-eyeZ)/radius;
	return dirVec;
}

float *getDirectionVector()
{
	float *dirVec = new float[2];
	/*dirVec[0] = 1.0;
     dirVec[1] = 2.0;
     dirVec[2] = 3.0;*/
	float radius = sqrt(((centerX-eyeX)*(centerX-eyeX))+((centerZ-eyeZ)*(centerZ-eyeZ)));
	//x
	dirVec[0] = (centerX- eyeX)/radius;
	dirVec[1] = (centerZ-eyeZ)/radius;
	return dirVec;
}

void scaleVector(float scaleFactor, float vector[])
{
	vector[0] = vector[0]*scaleFactor;
	vector[1] = vector[1]*scaleFactor;
}

int getRegion()
{
	if(eyeZ >= -14.0)
	{
		return 1;
	}
	
	if(eyeZ < -14.0 && eyeZ > -19.0)
	{
		return 2;
	}
	
	if(eyeZ < -19.0 && eyeZ  > -34.0 && eyeX > -20.0 && eyeX < 15.0)
	{
		return 3;
	}
	
	return 0;
	
}

bool collisionInRegion(int region)
{
    return false;
	cout << "CURRENTLY IN REGION: " << region << endl;
	if(region==1)
	{
		if((eyeZ <= -14.0 & eyeX < -1.0) || (eyeZ <= -14.0 & eyeX > 1.0))
		{
			
			return true;
		}
	}
	
	if(region==2)
	{
		if(( eyeX > 1.0 || eyeX < -1.0) ||(eyeZ <= -19.0 && lockedDoors[0].locked))
		{
			cout << "collision" << endl;
			return true;
		}
	}
    
	if(region==3)
	{
		if (!croutch && (eyeX < -20.0 || eyeX >15.0 || eyeZ <= -34.0 || (eyeZ > -19.0 && eyeX < -1.0) || ( eyeZ > -19.0 && eyeX > 1.0)))
		{
			cout << "Collission in region 3" << endl;
			cout << "EyeX: " << eyeX<< "eyeZ: " <<eyeZ<<endl;
			return true;
		}
	}
	if(region==4)//Does not take doors into account yet
	{
		if((eyeX < 30.0 && eyeX > 15.0) && (eyeZ < -32.0 && eyeZ > -36.0) && eyeY<=2.0)
		{
			cout << "PITTED" << endl;
			return true;
		}
		if(eyeX<=15.0 || eyeX >= 30.0 || eyeZ <= -49.0|| eyeZ >= -19.0)
		{
			return true;
		}
	}
	return false;
}

void keyOperations()
{
	float *dirVec = getDirectionVector();
	if(sprint)
	{
		scaleVector(2.0, dirVec);
	}
	if(keyStates['q'])//slowWalk)
	{
		scaleVector(0.5, dirVec);
	}
	if(!isJumping)
	{
		if(keyStates['e'])//(croutch)
		{
			croutch = !croutch;
			if(croutch)
			{
				eyeY -= 1.0;
				centerY -=1.0;
			}
			else
			{
				eyeY += 1.0;
				centerY += 1.0;
			}
		}
	}
	int region = getRegion();
	
	//Exit the Game
	if(keyStates[27])
	{
		glutSetKeyRepeat(GLUT_KEY_REPEAT_ON);
		exit(0);
	}
	
	//Walk Forward
	if(keyStates['w'])
	{
		eyeX+=dirVec[0];
		eyeZ+=dirVec[1];
		centerX+=dirVec[0];
		centerZ+=dirVec[1];
		if(!isInWorld() || collisionInRegion(region))
		{
			eyeX-=1.5*dirVec[0];
			eyeZ-=1.5*dirVec[1];
			centerX-=1.5*dirVec[0];
			centerZ-=1.5*dirVec[1];
		}
	}
	//Walk Backwards
	if(keyStates['s'])
	{
		eyeX-=dirVec[0];
		eyeZ-=dirVec[1];
		centerX-=dirVec[0];
		centerZ-=dirVec[1];
		if(!isInWorld() || collisionInRegion(region))
		{
			eyeX+=1.5*dirVec[0];
			eyeZ+=1.5*dirVec[1];
			centerX+=1.5*dirVec[0];
			centerZ+=1.5*dirVec[1];
		}
	}
	//Strafe Left
	if(keyStates['a'])
	{
		eyeX+=dirVec[1];
		eyeZ-=dirVec[0];
		centerX+=dirVec[1];
		centerZ-=dirVec[0];
		if(!isInWorld() || collisionInRegion(region))
		{
            eyeX-=1.5*dirVec[1];
            eyeZ+=1.5*dirVec[0];
            centerX-=1.5*dirVec[1];
            centerZ+=1.5*dirVec[0];
		}
	}
	//Strafe Right
	if(keyStates['d'])
	{
		eyeX-=dirVec[1];
		eyeZ+=dirVec[0];
		centerX-=dirVec[1];
		centerZ+=dirVec[0];
		if(!isInWorld() || collisionInRegion(region))
		{
			eyeX+=1.5*dirVec[1];
			eyeZ-=1.5*dirVec[0];
			centerX+=1.5*dirVec[1];
			centerZ-=1.5*dirVec[0];
		}
	}
	if(keyStates[SPACEBAR])
	{
		isJumping = true;
	}
}

void flashLightDrain()
{
	if(flashLightBattery - DRAINRATE > 0.0)
        flashLightBattery -= 0.5;
	else
        flashLightBattery = 0.0;
	
	if(flashLightBattery == 0.0)
	{
		flashLightOn = false;
	}
}

void flashLightCharge()
{
	if(flashLightBattery + CHARGERATE < 100.0)
        flashLightBattery += 0.25;
	else
        flashLightBattery = 100.0;
}

void drawSky() //We are inside the sphere
{
	glPushMatrix();
	glColor3f(0.0,0.2,0.9);
	//glBindTexture(GL_TEXTURE_2D, texture[2]);
	glTranslatef(0.0, 5.0, -52.0);
	glutSolidSphere(50.0,100.0,100.0);
	glPopMatrix();
}

void drawPit()
{
	glDisable(GL_LIGHTING);
	//glDisable(GL_TEXTURE);
	glColor3f(1.0,1.0,1.0);
	glBindTexture(GL_TEXTURE_2D, texture[8]);
	glBegin(GL_POLYGON);
	glTexCoord2f(0.0, 0.0); glVertex3f(15.0, 0.011, -32.0);
	glTexCoord2f(1.0,0.0); glVertex3f(30.0, 0.011, -32.0);
	glTexCoord2f(1.0,1.0);glVertex3f(30.0, 0.011, -36.0);
	glTexCoord2f(1.0,0.0);glVertex3f(15.0, 0.011, -36.0);
	glEnd();
	glColor3f(1.0,1.0,1.0);
	glEnable(GL_LIGHTING);
	//glEnable(GL_TEXTURE);
}

void interiorFloor()
{
    float matShine[] = {50.0};
    glPushMatrix();
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShine);
	
	glColor3f(1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, texture[6]);
 	float x,z;
	for (z=-19.0; z >-49.0; z-=1.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(x=-30.0; x <= 30.0; x+=5.0)
		{
			glNormal3f(0.0,-1.0,0.0);glTexCoord2f(20*((x+30)/60.0), -10*(z+19.0)/30.0); glVertex3f(x,0.01,z);
			glNormal3f(0.0,-1.0,0.0);glTexCoord2f(20*((x+30)/60.0), -10*(z+18.0)/30.0); glVertex3f(x,0.01,z-1.0);
		}
		glEnd();
	}
	glPopMatrix();
}

void interiorSideWall()
{
	float z;
	glBegin(GL_TRIANGLE_STRIP);
	for(z=-19.0; z >= -49.0 ; z-=5.0)
	{
		glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19.0)/30.0, 0); glVertex3f(-30.0,0.0,z);
		glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19.0)/30.0, 1); glVertex3f(-30.0,10.0,z);
	}
	glEnd();
}

void interiorBackWall()
{
	float x;
	glFrontFace(GL_CW);
	glBegin(GL_TRIANGLE_STRIP);
	for(x =-30.0 ; x <= 30.0 ; x+=5.0)
	{
		glNormal3f(0.0,0.0,1.0); glTexCoord2f((x+30.0)/60.0, 0.0); glVertex3f(x,0.0,-49.0);
		glNormal3f(0.0,0.0,1.0); glTexCoord2f((x+30.0)/60.0, 1.0); glVertex3f(x,10.0,-49.0);
	}
	glEnd();
	glFrontFace(GL_CCW);
}

void interiorDivider1()
{
	float x;
	glFrontFace(GL_CW);
	glBegin(GL_TRIANGLE_STRIP);
	for(x =-30.0 ; x <= 15.0 ; x+=5.0)
	{
		glNormal3f(0.0,0.0,1.0); glTexCoord2f((x+30.0)/60.0, 0.0); glVertex3f(x,0.0,-34.0);
		glNormal3f(0.0,0.0,1.0); glTexCoord2f((x+30.0)/60.0, 1.0); glVertex3f(x,10.0,-34.0);
	}
	glEnd();
	glFrontFace(GL_CCW);
}

void interiorDivider2() //crawl space;
{
	glPushMatrix();
	glColor3f(1.0, 1.0, 1.0);
	float z,y,x;
	glFrontFace(GL_CCW);
	for(y=0.0; y<=10.0;y+=1.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(z=-19; z >= -25.5; z-=0.5)
		{
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, y/10); glVertex3f(-15.0, y, z);
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, (y+1.0)/10);glVertex3f(-15.0,y+1.0, z);
		}
		glEnd();
	}
	for(y=2.0; y<=10.0;y+=1.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(z=-25.5; z >= -27.5; z-=0.5)
		{
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, y/10); glVertex3f(-15.0, y, z);
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, (y+1.0)/10);glVertex3f(-15.0,y+1.0, z);
		}
		glEnd();
	}
	for(y=0.0; y<=10.0;y+=1.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(z=-27.5; z >= -34.0; z-=0.5)
		{
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, y/10); glVertex3f(-15.0, y, z);
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, (y+1.0)/10);glVertex3f(-15.0,y+1.0, z);
		}
		glEnd();
	}
	//back
	for(y=0.0; y<=10.0;y+=1.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(z=-19; z >= -25.5; z-=0.5)
		{
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, y/10); glVertex3f(-20.0, y, z);
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, (y+1.0)/10);glVertex3f(-20.0,y+1.0, z);
		}
		glEnd();
	}
	for(y=2.0; y<=10.0;y+=1.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(z=-25.5; z >= -27.5; z-=0.5)
		{
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, y/10); glVertex3f(-20.0, y, z);
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, (y+1.0)/10);glVertex3f(-20.0,y+1.0, z);
		}
		glEnd();
	}
	for(y=0.0; y<=10.0;y+=1.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(z=-27.5; z >= -34.0; z-=0.5)
		{
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, y/10); glVertex3f(-20.0, y, z);
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, (y+1.0)/10);glVertex3f(-20.0,y+1.0, z);
		}
		glEnd();
	}
	glFrontFace(GL_CW);
	// interior
	glBindTexture(GL_TEXTURE_2D, texture[4]);
	//left
	for (y=0; y < 2.0; y+=1.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(x=-20.0; x <= -15.0; x+=1.0)
		{
			glNormal3f(0.0, 0.0,1.0);glTexCoord2f(5*((x+30)/60.0), y/10.0); glVertex3f(x,y,-25.5);
			glNormal3f(0.0, 0.0,1.0);glTexCoord2f(5*((x+30)/60.0), (y+1.0)/10.0); glVertex3f(x,y+1.0,-25.5);
		}
		glEnd();
	}
	//reight
	for (y=0; y < 2.0; y+=1.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(x=-20.0; x <= -15.0; x+=1.0)
		{
			glNormal3f(0.0, 0.0,1.0);glTexCoord2f(5*((x+30)/60.0), y/10.0); glVertex3f(x,y,-27.5);
			glNormal3f(0.0, 0.0,1.0);glTexCoord2f(5*((x+30)/60.0), (y+1.0)/10.0); glVertex3f(x,y+1.0,-27.5);
		}
		glEnd();
	}
	//top
	glFrontFace(GL_CCW);
	for (z=-25.5; z >-27.5; z-=1.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(x=-20.0; x <= -15.0; x+=1.0)
		{
			glNormal3f(0.0,-1.0,0.0);glTexCoord2f(20*((x+30)/60.0), -10*(z+19.0)/30.0); glVertex3f(x,2.0,z);
			glNormal3f(0.0,-1.0,0.0);glTexCoord2f(20*((x+30)/60.0), -10*(z+18.0)/30.0); glVertex3f(x,2.0,z-1.0);
		}
		glEnd();
	}
	glFrontFace(GL_CW);
	glPopMatrix();
	glBindTexture(GL_TEXTURE_2D, texture[5]);
}

void interiorDivider3()
{
	glPushMatrix();
	glColor3f(1.0, 1.0, 1.0);
	float z,y;
	glFrontFace(GL_CCW);
	for(y=0.0; y<=10.0;y+=5.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(z=-19; z >= -25.5; z-=0.5)
		{
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, y/10); glVertex3f(15.0, y, z);
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, (y+5.0)/10);glVertex3f(15.0,y+5.0, z);
		}
		glEnd();
	}
	
	for(y=5.0; y<=10.0;y+=5.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(z=-25.5; z >= -27.5; z-=0.5)
		{
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, y/10); glVertex3f(15.0, y, z);
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, (y+5.0)/10);glVertex3f(15.0,y+5.0, z);
		}
		glEnd();
	}
	
	for(y=0.0; y<=10.0;y+=5.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(z=-27.5; z >= -40.5; z-=0.5)
		{
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, y/10); glVertex3f(15.0, y, z);
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, (y+5.0)/10);glVertex3f(15.0,y+5.0, z);
		}
		glEnd();
	}
	//**
	
	for(y=0.0; y<=10.0;y+=5.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(z=-19; z >= -25.5; z-=0.5)
		{
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, y/10); glVertex3f(15.5, y, z);
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, (y+5.0)/10);glVertex3f(15.5,y+5.0, z);
		}
		glEnd();
	}
	
	for(y=5.0; y<=10.0;y+=5.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(z=-25.5; z >= -27.5; z-=0.5)
		{
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, y/10); glVertex3f(15.5, y, z);
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, (y+5.0)/10);glVertex3f(15.5,y+5.0, z);
		}
		glEnd();
	}
	
	for(y=0.0; y<=10.0;y+=5.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(z=-27.5; z >= -40.5; z-=0.5)
		{
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, y/10); glVertex3f(15.5, y, z);
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, (y+5.0)/10);glVertex3f(15.5,y+5.0, z);
		}
		glEnd();
	}
	
	for(y=5.0; y<=10.0;y+=5.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(z=-40.5; z >= -42.5; z-=0.5)
		{
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, y/10); glVertex3f(15.5, y, z);
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, (y+5.0)/10);glVertex3f(15.5,y+5.0, z);
		}
		glEnd();
	}
	
	for(y=0.0; y<=10.0;y+=5.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(z=-42.5; z >= -49.0; z-=0.5)
		{
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, y/10); glVertex3f(15.5, y, z);
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, (y+5.0)/10);glVertex3f(15.5,y+5.0, z);
		}
		glEnd();
	}
	
	
	//**
	for(y=5.0; y<=10.0;y+=5.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(z=-40.5; z >= -42.5; z-=0.5)
		{
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, y/10); glVertex3f(15.0, y, z);
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, (y+5.0)/10);glVertex3f(15.0,y+5.0, z);
		}
		glEnd();
	}
	
	for(y=0.0; y<=10.0;y+=5.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(z=-42.5; z >= -49.0; z-=0.5)
		{
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, y/10); glVertex3f(15.0, y, z);
			glNormal3f(-1.0,0.0,0.0); glTexCoord2f((z+19)/30.0, (y+5.0)/10);glVertex3f(15.0,y+5.0, z);
		}
		glEnd();
	}
	
	glBegin(GL_POLYGON);
    glNormal3f(0.0,0.0,1.0); glTexCoord2f(0.0,0.0); glVertex3f(15.0, 0.0, -25.5);
    glNormal3f(0.0,0.0,1.0); glTexCoord2f(1.0,0.0); glVertex3f(15.5, 0.0, -25.5);
    glNormal3f(0.0,0.0,1.0); glTexCoord2f(1.0,1.0); glVertex3f(15.5, 5.0, -25.5);
    glNormal3f(0.0,0.0,1.0); glTexCoord2f(0.0,1.0); glVertex3f(15.0, 5.0, -25.5);
	glEnd();
	glBegin(GL_POLYGON);
    glNormal3f(0.0,0.0,1.0); glTexCoord2f(0.0,0.0); glVertex3f(15.0, 0.0, -27.5);
    glNormal3f(0.0,0.0,1.0); glTexCoord2f(1.0,0.0); glVertex3f(15.5, 0.0, -27.5);
    glNormal3f(0.0,0.0,1.0); glTexCoord2f(1.0,1.0); glVertex3f(15.5, 5.0, -27.5);
    glNormal3f(0.0,0.0,1.0); glTexCoord2f(0.0,1.0); glVertex3f(15.0, 5.0, -27.5);
	glEnd();
	glFrontFace(GL_CW);
	glBegin(GL_POLYGON);
    glNormal3f(0.0,-1.0,0.0); glTexCoord2f(0.0,0.0); glVertex3f(15.0, 5.0, -26.5);
    glNormal3f(0.0,-1.0,0.0); glTexCoord2f(1.0,0.0); glVertex3f(15.5, 5.0, -26.5);
    glNormal3f(0.0,-1.0,0.0); glTexCoord2f(1.0,1.0); glVertex3f(15.5, 5.0, -27.5);
    glNormal3f(0.0,-1.0,0.0); glTexCoord2f(0.0,1.0); glVertex3f(15.0, 5.0, -27.5);
	glEnd();
	
	glPushMatrix();
	glTranslatef(0.0,0.0,-15.0);
	glBegin(GL_POLYGON);
    glNormal3f(0.0,0.0,1.0); glTexCoord2f(0.0,0.0); glVertex3f(15.0, 0.0, -25.5);
    glNormal3f(0.0,0.0,1.0); glTexCoord2f(1.0,0.0); glVertex3f(15.5, 0.0, -25.5);
    glNormal3f(0.0,0.0,1.0); glTexCoord2f(1.0,1.0); glVertex3f(15.5, 5.0, -25.5);
    glNormal3f(0.0,0.0,1.0); glTexCoord2f(0.0,1.0); glVertex3f(15.0, 5.0, -25.5);
	glEnd();
	glBegin(GL_POLYGON);
    glNormal3f(0.0,0.0,1.0); glTexCoord2f(0.0,0.0); glVertex3f(15.0, 0.0, -27.5);
    glNormal3f(0.0,0.0,1.0); glTexCoord2f(1.0,0.0); glVertex3f(15.5, 0.0, -27.5);
    glNormal3f(0.0,0.0,1.0); glTexCoord2f(1.0,1.0); glVertex3f(15.5, 5.0, -27.5);
    glNormal3f(0.0,0.0,1.0); glTexCoord2f(0.0,1.0); glVertex3f(15.0, 5.0, -27.5);
	glEnd();
	glFrontFace(GL_CW);
	glBegin(GL_POLYGON);
    glNormal3f(0.0,-1.0,0.0); glTexCoord2f(0.0,0.0); glVertex3f(15.0, 5.0, -26.5);
    glNormal3f(0.0,-1.0,0.0); glTexCoord2f(1.0,0.0); glVertex3f(15.5, 5.0, -26.5);
    glNormal3f(0.0,-1.0,0.0); glTexCoord2f(1.0,1.0); glVertex3f(15.5, 5.0, -27.5);
    glNormal3f(0.0,-1.0,0.0); glTexCoord2f(0.0,1.0); glVertex3f(15.0, 5.0, -27.5);
	glEnd();
	glPopMatrix();
	glFrontFace(GL_CCW);
	glPopMatrix();
	
}

void interiorFrontWalls()
{
	float x,y;
	glFrontFace(GL_CW);
	
	//
	for (y=0; y < 10.0; y+=5.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(x=-30.0; x <= -1.0; x+=1.0)
		{
			glNormal3f(0.0, 0.0,1.0);glTexCoord2f(5*((x+30)/60.0), y/10.0); glVertex3f(x,y,-19.0);
			glNormal3f(0.0, 0.0,1.0);glTexCoord2f(5*((x+30)/60.0), (y+5.0)/10.0); glVertex3f(x,y+5.0,-19.0);
		}
		glEnd();
	}
	for (y=5; y < 10.0; y+=5.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(x=-1.0; x <= 1.0; x+=1.0)
		{
			glNormal3f(0.0, 0.0,1.0);glTexCoord2f(5*((x+30)/60.0), y/10.0); glVertex3f(x,y,-19.0);
			glNormal3f(0.0, 0.0,1.0);glTexCoord2f(5*((x+30)/60.0), (y+5.0)/10.0); glVertex3f(x,y+5.0,-19.0);
		}
		glEnd();
	}
	for (y=0; y < 10.0; y+=5.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(x=1.0; x <= 30.0; x+=1.0)
		{
			glNormal3f(0.0, 0.0,1.0);glTexCoord2f(5*((x+30)/60.0), y/10.0); glVertex3f(x,y,-19.0);
			glNormal3f(0.0, 0.0,1.0);glTexCoord2f(5*((x+30)/60.0), (y+5.0)/10.0); glVertex3f(x,y+5.0,-19.0);
		}
		glEnd();
	}
	glFrontFace(GL_CCW);
}

void cieling()
{
	glBindTexture(GL_TEXTURE_2D, texture[7]);
	float x,z;
	for (z=-19.0; z >-49.0; z-=1.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(x=-30.0; x <= 30.0; x+=5.0)
		{
			glNormal3f(0.0,-1.0,0.0);glTexCoord2f(5*((x+30)/60.0), -5*(z+19.0)/30.0); glVertex3f(x,10.0,z);
			glNormal3f(0.0,-1.0,0.0);glTexCoord2f(5*((x+30)/60.0), -5*(z+18.0)/30.0); glVertex3f(x,10.0,z-1.0);
		}
		glEnd();
	}
}

void interiorWalls()
{
	glPushMatrix();
	glColor3f(1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, texture[5]);
	interiorSideWall();
	glPushMatrix();
	glTranslatef(60.0,0.0,0.0);
	interiorSideWall();
	glPopMatrix();
	interiorBackWall();
	glPopMatrix();
	interiorDivider1();
	interiorDivider2();
	interiorDivider3();
	interiorFrontWalls();
}

void drawWall()
{
	glPushMatrix();
	glColor3f(1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	glBegin(GL_POLYGON);
	glNormal3f(0.0,0.0,1.0); glTexCoord2f(0.0, 0.0); glVertex3f(-5.0, 0.0, -14.0);
	glNormal3f(0.0,0.0,1.0); glTexCoord2f(5.0, 0.0); glVertex3f(5.0, 0.0, -14.0);
	glNormal3f(0.0,0.0,1.0); glTexCoord2f(5.0, 5.0); glVertex3f(5.0,10.0,-14.0);
	glNormal3f(0.0,0.0,1.0); glTexCoord2f(0.0, 5.0); glVertex3f(-5.0, 10.0, -14.0);
	glEnd();
	glPopMatrix();
	
	glColor3f(1.0, 1.0, 1.0);
}

void drawWallWithDoorHole()
{
	glPushMatrix();
	glColor3f(1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	float x,y;
	glFrontFace(GL_CW);
	
	//
	for (y=0; y < 10.0; y+=5.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(x=-30.0; x <= -1.0; x+=1.0)
		{
			glNormal3f(0.0, 0.0,1.0);glTexCoord2f(5*((x+30)/60.0), y/10.0); glVertex3f(x,y,-14.0);
			glNormal3f(0.0, 0.0,1.0);glTexCoord2f(5*((x+30)/60.0), (y+5.0)/10.0); glVertex3f(x,y+5.0,-14.0);
		}
		glEnd();
	}
	for (y=5; y < 10.0; y+=5.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(x=-1.0; x <= 1.0; x+=1.0)
		{
			glNormal3f(0.0, 0.0,1.0);glTexCoord2f(5*((x+30)/60.0), y/10.0); glVertex3f(x,y,-14.0);
			glNormal3f(0.0, 0.0,1.0);glTexCoord2f(5*((x+30)/60.0), (y+5.0)/10.0); glVertex3f(x,y+5.0,-14.0);
		}
		glEnd();
	}
	for (y=0; y < 10.0; y+=5.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(x=1.0; x <= 30.0; x+=1.0)
		{
			glNormal3f(0.0, 0.0,1.0);glTexCoord2f(5*((x+30)/60.0), y/10.0); glVertex3f(x,y,-14.0);
			glNormal3f(0.0, 0.0,1.0);glTexCoord2f(5*((x+30)/60.0), (y+5.0)/10.0); glVertex3f(x,y+5.0,-14.0);
		}
		glEnd();
	}
	glEnd();
	glPopMatrix();
	glFrontFace(GL_CCW);
}

void drawEntranceSideWall()
{
	//Left Entrance Wall
	glPushMatrix();
	glColor3f(1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	glBegin(GL_POLYGON);
	glNormal3f(1.0,0.0,0.0); glTexCoord2f(0.0, 0.0); glVertex3f(-1.0,0.0,-14.0);
	glNormal3f(1.0,0.0,0.0); glTexCoord2f(1.0, 0.0); glVertex3f(-1.0,0.0,-19.0);
	glNormal3f(1.0,0.0,0.0); glTexCoord2f(1.0, 1.0); glVertex3f(-1.0,5.0,-19.0);
	glNormal3f(1.0,0.0,0.0); glTexCoord2f(0.0, 1.0); glVertex3f(-1.0,5.0,-14.00);
	glEnd();
	glPopMatrix();
}

void lockButton1()
{
	glDisable(GL_TEXTURE);
    
    
	if(keypad[0] == 0)
        glColor3f(1.0,0.0,0.0);
	if(keypad[0] == 1)
        glColor3f(0.0,1.0,0.0);
	glBegin(GL_POLYGON);
	glNormal3f(1.0,0.0,0.0);glVertex3f(-0.79,1.65,-16.15);
	glNormal3f(1.0,0.0,0.0);glVertex3f(-0.79,1.65,-16.45);
	glNormal3f(1.0,0.0,0.0);glVertex3f(-0.79,1.9,-16.45);
	glNormal3f(1.0,0.0,0.0);glVertex3f(-0.79 ,1.9,-16.15);
	glEnd();
	glEnable(GL_TEXTURE);
    
}

void lockButton2()
{
    
	glFrontFace(GL_CW);
	glDisable(GL_TEXTURE);
	if(keypad[1] == 0)
	{
		glColor3f(1.0,0.0,0.0);
	}
	if(keypad[1] == 1)
		glColor3f(0.0,1.0,0.0);
	glBegin(GL_POLYGON);
	glNormal3f(1.0,0.0,0.0);glVertex3f(-0.79,1.65,-16.85);
	glNormal3f(1.0,0.0,0.0);glVertex3f(-0.79,1.65,-16.6);
	glNormal3f(1.0,0.0,0.0);glVertex3f(-0.79,1.9,-16.6);
	glNormal3f(1.0,0.0,0.0);glVertex3f(-0.79 ,1.9,-16.85);
	glEnd();
	glEnable(GL_TEXTURE);
    
}

/*void lockButton3()
 {
 glFrontFace(GL_CCW);
 glDisable(GL_TEXTURE);
 if(keypad[2] == 0)
 glColor3f(1.0,0.0,0.0);
 if(keypad[2] == 1)
 glColor3f(0.0,1.0,0.0);
 glLoadName(1);
 glBegin(GL_POLYGON);
 glNormal3f(1.0,0.0,0.0);glVertex3f(-0.79,2.10,-16.25);
 glNormal3f(1.0,0.0,0.0);glVertex3f(-0.79,2.10,-16.5);
 glNormal3f(1.0,0.0,0.0);glVertex3f(-0.79,2.35,-16.5);
 glNormal3f(1.0,0.0,0.0);glVertex3f(-0.79 ,2.35,-16.25);
 glEnd();
 glEnable(GL_TEXTURE);
 }*/

void drawDoorLock()
{
	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, texture[4]);
	//Front Face
	glBegin(GL_QUADS);
	glNormal3f(1.0,0.0,0.0);glTexCoord2f(0.0,0.0); glVertex3f(-0.8,1.5,-16.0);
	glNormal3f(1.0,0.0,0.0);glTexCoord2f(1.0,0.0); glVertex3f(-0.8,1.5,-17.0);
	glNormal3f(1.0,0.0,0.0);glTexCoord2f(1.0,1.0); glVertex3f(-0.8,2.5,-17.0);
	glNormal3f(1.0,0.0,0.0);glTexCoord2f(0.0,1.0); glVertex3f(-0.8,2.5,-16.0);
	glEnd();
	//Left Face
	glBegin(GL_QUADS);
	glNormal3f(0.0,0.0,-1.0);glTexCoord2f(0.0,0.0); glVertex3f(-1.0,1.5,-16.0);
	glNormal3f(0.0,0.0,-1.0);glTexCoord2f(1.0,0.0); glVertex3f(-0.8,1.5,-16.0);
	glNormal3f(0.0,0.0,-1.0);glTexCoord2f(1.0,1.0); glVertex3f(-0.8,2.5,-16.0);
	glNormal3f(0.0,0.0,-1.0);glTexCoord2f(0.0,0.0); glVertex3f(-1.0,2.5,-16.0);
	glEnd();
	//Right Face
	glBegin(GL_QUADS);
	glNormal3f(0.0,0.0,-1.0);glTexCoord2f(0.0,0.0); glVertex3f(-1.0,1.5,-17.0);
	glNormal3f(0.0,0.0,-1.0);glTexCoord2f(1.0,0.0); glVertex3f(-0.8,1.5,-17.0);
	glNormal3f(0.0,0.0,-1.0);glTexCoord2f(1.0,1.0); glVertex3f(-0.8,2.5,-17.0);
	glNormal3f(0.0,0.0,-1.0);glTexCoord2f(0.0,0.0); glVertex3f(-1.0,2.5,-17.0);
	glEnd();
	//Bottom Face
	glBegin(GL_QUADS);
	glNormal3f(0.0,-1.0,0.0);glTexCoord2f(0.0,0.0); glVertex3f(-1.0,1.5,-16.0);
	glNormal3f(0.0,-1.0,0.0);glTexCoord2f(1.0,0.0); glVertex3f(-0.8,1.5,-16.0);
	glNormal3f(0.0,-1.0,0.0);glTexCoord2f(1.0,1.0); glVertex3f(-0.8,1.5,-17.0);
	glNormal3f(0.0,-1.0,0.0);glTexCoord2f(0.0,1.0); glVertex3f(-1.0,1.5,-17.0);
	glEnd();
	//Top Face
	glBegin(GL_QUADS);
	glNormal3f(0.0,1.0,0.0);glTexCoord2f(0.0,0.0); glVertex3f(-1.0,2.5,-16.0);
	glNormal3f(0.0,1.0,0.0);glTexCoord2f(1.0,0.0); glVertex3f(-0.8,2.5,-16.0);
	glNormal3f(0.0,1.0,0.0);glTexCoord2f(1.0,1.0); glVertex3f(-0.8,2.5,-17.0);
	glNormal3f(0.0,1.0,0.0);glTexCoord2f(0.0,1.0); glVertex3f(-1.0,2.5,-17.0);
	glEnd();
	
	//lockButtons();
    glPopMatrix();
}

void drawEntranceRoof()
{
	//Left Entrance Wall
	//is normal reversed??
	glPushMatrix();
	glColor3f(1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	glBegin(GL_POLYGON);
	glNormal3f(0.0,-1.0,0.0); glTexCoord2f(0.0, 0.0); glVertex3f(-1.0,5.0,-14.0);
	glNormal3f(0.0,-1.0,0.0); glTexCoord2f(1.0, 0.0); glVertex3f(1.0,5.0,-14.0);
	glNormal3f(0.0,-1.0,0.0); glTexCoord2f(1.0, 1.0); glVertex3f(1.0,5.0,-19.0);
	glNormal3f(0.0,-1.0,0.0); glTexCoord2f(0.0, 1.0); glVertex3f(-1.0,5.0,-19.00);
	glEnd();
	glPopMatrix();
}

void drawDoor()
{
	glPushMatrix();
	glTranslatef(0.0,5.0*openFactor,0.0);
	glBindTexture(GL_TEXTURE_2D, texture[3]);
	glBegin(GL_POLYGON);
	glNormal3f(0.0,0.0,1.0); glTexCoord2f(0.0,0.0); glVertex3f(-1.0,0.0,-14.1);
	glNormal3f(0.0,0.0,1.0); glTexCoord2f(1.0,0.0); glVertex3f(1.0,0.0,-14.1);
	glNormal3f(0.0,0.0,1.0); glTexCoord2f(1.0,1.0); glVertex3f(1.0,5.0,-14.1);
	glNormal3f(0.0,0.0,1.0); glTexCoord2f(0.0,1.0); glVertex3f(-1.0,5.0,-14.1);
	glEnd();
	glPopMatrix();
}

void drawLockedDoor1()
{
	glPushMatrix();
	glTranslatef(0.0,5.0*lockedDoors[0].openFactor,0.0);
	glBindTexture(GL_TEXTURE_2D, texture[3]);
	glBegin(GL_POLYGON);
	glNormal3f(0.0,0.0,1.0); glTexCoord2f(0.0,0.0); glVertex3f(-1.0,0.0,-18.9);
	glNormal3f(0.0,0.0,1.0); glTexCoord2f(1.0,0.0); glVertex3f(1.0,0.0,-18.9);
	glNormal3f(0.0,0.0,1.0); glTexCoord2f(1.0,1.0); glVertex3f(1.0,5.0,-18.9);
	glNormal3f(0.0,0.0,1.0); glTexCoord2f(0.0,1.0); glVertex3f(-1.0,5.0,-18.9);
	glEnd();
	glPopMatrix();
}

void drawLockedDoor2()
{
	glPushMatrix();
	glTranslatef(0.0,5.0*lockedDoors[1].openFactor,0.0);
	glBindTexture(GL_TEXTURE_2D, texture[3]);
	glBegin(GL_POLYGON);
	glNormal3f(-1.0,0.0,0.0); glTexCoord2f(0.0,0.0); glVertex3f(15.25,0.0,-27.5);
	glNormal3f(-1.0,0.0,0.0); glTexCoord2f(1.0,0.0); glVertex3f(15.25,0.0,-25.5);
	glNormal3f(-1.0,0.0,0.0); glTexCoord2f(1.0,1.0); glVertex3f(15.25,5.0,-25.5);
	glNormal3f(-1.0,0.0,0.0); glTexCoord2f(0.0,1.0); glVertex3f(15.25,5.0,-27.5);
	glEnd();
	glPopMatrix();
}

// Routine to draw a bitmap character string.
void writeBitmapString(void *font, char *string)
{
	char *c;
	for (c = string; *c != '\0'; c++) glutBitmapCharacter(font, *c);
}

void drawWalkWay()
{	glFrontFace(GL_CW);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	for(float z =-5.0; z >= -14.0; z -= 2.0f)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(float x = -2.0; x <= 2.0; x += 1.0f)
		{
			glNormal3f(0.0, 1.0, 0.0); glTexCoord2f(0.0,0.0); glVertex3f(x,0.01,z);
			glNormal3f(0.0, 1.0, 0.0); glTexCoord2f(0.0,1.0); glVertex3f(x,0.01,z-1.0);
			glNormal3f(0.0, 1.0, 0.0); glTexCoord2f(1.0,0.0); glVertex3f(x+0.5,0.01,z);
			glNormal3f(0.0, 1.0, 0.0); glTexCoord2f(1.0,1.0); glVertex3f(x+0.5,0.01,z-1.0);
		}
		glEnd();
	}
	glFrontFace(GL_CCW);
}

//Draws my text
void drawName()
{
	//Convert String to character array for writeBitMapString
	char *writeable=new char[myName.size()+1];
	writeable[myName.size()]=0;
	memcpy(writeable,myName.c_str(),myName.size());
	glPushMatrix();
	glColor3f(1.0,0.0,0.0);
	glScalef(2.0, 2.0, 2.0);
	glRasterPos2f(-20.0,22.5 );
	writeBitmapString(GLUT_BITMAP_HELVETICA_18, writeable);
	glPopMatrix();
	glColor3f(1.0,1.0,1.0);
}

void drawMeter()
{
	//Frame
	glColor3f(0.0,0.0,0.0);
	glBegin(GL_POLYGON);
	glVertex2f(45.4, 47.9);
	glVertex2f(29.6, 47.9);
	glVertex2f(29.6, 42.1);
	glVertex2f(45.4,42.1);
	glEnd();
	//Meter
	if(flashLightBattery > 0)
	{
		glColor3f(1.0,0.0,0.0);
		glBegin(GL_POLYGON);
		glVertex2f(30.0+15.0*(flashLightBattery/100.0), 47.5);
		glVertex2f(30.0, 47.5);
		glVertex2f(30.0, 42.5);
		glVertex2f(30.0+15.0*(flashLightBattery/100.0),42.5);
		glEnd();
	}
}

void drawKey()
{
	glPushMatrix();
	glTranslatef(-27.5, 0.0, -32.0);
	glRotatef( (fire*PI)/180.0 , 0.0, 1.0, 0.0);
	glBindTexture(GL_TEXTURE_2D, texture[9]);
	glBegin(GL_POLYGON);
	glTexCoord2f(0.0,0.0); glVertex3f(-0.25, 0.5, 0.0);
	glTexCoord2f(1.0, 0.0); glVertex3f(0.25, 0.5, 0.0);
	glTexCoord2f(1.0, 1.0); glVertex3f(0.25, 1.0, 0.0);
	glTexCoord2f(0.0,1.0); glVertex3f(-0.25, 1.0, 0.0);
	glEnd();
	glPopMatrix();
}

void drawKeyInHud()
{
	glEnable(GL_TEXTURE);
	glColor3f(1.0,1.0,1.0);
	glBindTexture(GL_TEXTURE_2D, texture[9]);
	glBegin(GL_POLYGON);
	glTexCoord2f(0.0, 0.0); glVertex2f(20.0, 42.5);
	glTexCoord2f(1.0,0.0); glVertex2f(26.0, 42.5);
	glTexCoord2f(1.0,1.0); glVertex2f(26.0, 47.5);
	glTexCoord2f(0.0,1.0); glVertex2f(20.0, 47.5);
	glEnd();
	glDisable(GL_TEXTURE);
}

void drawHUD()
{
	float t=0;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-50.0, 50.0, -50.0, 50.0, -5.0, 100.0);
	glMatrixMode(GL_MODELVIEW);
	//glClearDepth(1);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE);
	glDisable(GL_FOG);
	//White Transparent Frame
	glColor4f(1.0,1.0,1.0,0.2);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_POLYGON);
	glVertex2f(-50.0,40.0);
	glVertex2f(50.0,40.0);
	glVertex2f(50.0,50.0);
	glVertex2f(-50.0,50.0);
	glEnd();
	if(!lockedDoors[1].locked)
		drawKeyInHud();
	glLineWidth(2.0);
	//Crosshair
	glBegin(GL_LINE_LOOP);
	glColor3f(1.0,0.0,0.0);
	for(int i=0; i < 100; ++i)
	{
		glVertex2f(0+2*cos(t),2+2*sin(t));
		t+=2*PI/100;
	}
	glEnd();
	glLineWidth(2.0);
	drawMeter();
	drawName();
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0,1.0,0.1,102.0);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE);
	glEnable(GL_FOG);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	
}

void drawGrass()
{
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	float x,z;
	for (z=0.0; z >-100.0; z-=1.0)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for(x=-50.0; x <= 50.0; x+=5.0)
		{
			glNormal3f(0.0,-1.0,0.0);glTexCoord2f(10*(x+30)/60.0, -10*(z+19.0)/30.0); glVertex3f(x,0.0,z);
			glNormal3f(0.0,-1.0,0.0);glTexCoord2f(10*(x+30)/60.0, -10*(z+18.0)/30.0); glVertex3f(x,0.0,z-1.0);
		}
		glEnd();
	}
}


void drawSelectable()
{
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
	glPushMatrix();
    gluLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);
    glLoadName(1);
    lockButton1();
	glLoadName(2);
    lockButton2();
	//glLoadName(3);
    //lockButton3();
    glPopName();
}

// Main Drawing routine.
// --Calls other drawing routines
void drawScene(void)
{
	glDisable(GL_LIGHT1);
	keyOperations();
	drawSelectable();
	//Light property vectors
	float lightAmb[] = {0.0, 0.0, 0.0, 1.0};
	float lightDifAndSpec0[] = {0.0, 0.0, 0.0, 1.0};
	float lightPos0[] = {0.0, 10.0, 15.0, 1.0};
	float globAmb[] = {0.1, 0.1, 0.1, 1.0 };
	
	float radius = sqrt((centerX-eyeX)*(centerX-eyeX) + (centerY-eyeY)*(centerY-eyeY) + (centerZ-eyeZ)*(centerZ-eyeZ));
	float distanceToKey = sqrt((eyeX+27.5)*(eyeX+27.5) + (eyeZ+32.0)*(eyeZ+32.0));
	cout << distanceToKey << endl;
	if(distanceToKey < 1.0)
	{
		cout << "GOT DA KEY" << endl;
		lockedDoors[1].locked = false;
	}
	float spotAmb[] = {0.0,0.0,0.0,1.0};
	float spotDifAndSpec[] = {1.0,1.0,1.0,1.0};
	float spotPos[] = {eyeX,eyeY, eyeZ, 1.0};
	float spotDirection[]= { (centerX-eyeX)/radius, (centerY-eyeY)/radius, (centerZ-eyeZ)/radius};
	float spotExponent = 15.0; // Spotlight exponent = attenuation.
	float spotAngle = 90.0;
	
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globAmb); // Global ambient light.
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE); // Enable two-sided lighting. NEED THIS?
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, localViewer); // Enable local viewpoint
	
	//Spotlight Properties::LIGHT1
	if(flashLightOn && flashLightBattery > 0.0)
	{
		glLightfv(GL_LIGHT1, GL_AMBIENT, spotAmb);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, spotDifAndSpec);
		glLightfv(GL_LIGHT1, GL_SPECULAR, spotDifAndSpec);
		glLightfv(GL_LIGHT1, GL_POSITION, spotPos);
		glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, spotAngle);
		glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, spotDirection);
		glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, spotExponent);
		glEnable(GL_LIGHT1);
		flashLightDrain();
	}
	else if (flashLightBattery < 100.0)
	{
		flashLightCharge();
	}
	
	glEnable(GL_FOG);
	glHint(GL_FOG_HINT, GL_NICEST);
	glFogfv(GL_FOG_COLOR, fogColor);
	glFogi(GL_FOG_MODE, fogMode);
	glFogf(GL_FOG_START, fogStart);
	glFogf(GL_FOG_END, fogEnd);
	glFogf(GL_FOG_DENSITY, fogDensity);
	
    //drawParticle(testParticle);
	for(int i = 0; i < MAXPARTICLES; i++)
	{
		drawParticle(ourParticles[i]);
		evolveParticle(i);
	}
	drawSky();
	drawWallWithDoorHole();
	drawDoor();
	drawLockedDoor1();
	drawLockedDoor2();
	drawEntranceRoof();
	drawEntranceSideWall();
	drawDoorLock();
	glPushMatrix();
	glTranslatef(2.0,0.0,0.0);
	drawEntranceSideWall();
	glBegin(GL_POLYGON);
	glEnd();
	glPopMatrix();
	interiorWalls();
	interiorFloor();
	drawPit();
	glDisable(GL_TEXTURE);
	cieling();
	glEnable(GL_TEXTURE);
	if(lockedDoors[1].locked)
        drawKey();
	drawGrass();
	drawWalkWay();
	glPopMatrix();
	glPushMatrix();
	drawHUD();
	glPopMatrix();
	//glutWarpPointer	(500,500);
	glutSwapBuffers();
    
}

// Process hit buffer to find record with smallest min-z value.
void findClosestHit(int hits, unsigned int buffer[])
{
    unsigned int *ptr, minZ;
    minZ= 0xffffffff; // 2^32 - 1
    ptr = buffer;
    closestName = 0;
    for (int i = 0; i < hits; i++)
    {
        ptr++;
        if (*ptr < minZ)
        {
            minZ = *ptr;
            ptr += 2;
            closestName = *ptr;
            ptr++;
        }
        else ptr += 3;
    }
}


//mouseControl for control screen
void mouseControl(int button, int state, int x, int y)
{
	
	int viewport[4]; // Viewport data.
    if((button==GLUT_LEFT_BUTTON) && (state==GLUT_DOWN))
    {
		glGetIntegerv(GL_VIEWPORT, viewport);   // Get viewport data.
		glSelectBuffer(1024, buffer);           // Specify buffer to write hit records in selection mode
		(void) glRenderMode(GL_SELECT);         // Enter selection mode.
		
		// Save the viewing volume defined in the resize routine.
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		
		// Define a viewing volume corresponding to selecting in 3 x 3 region around the cursor.
		glLoadIdentity();
		gluPickMatrix((float)500.0, (float)(viewport[3]-500.0),1.0,1.0,viewport);
		gluPerspective(45.0,1.0,0.1,102.0);     // Copied from the reshape routine.
		
		glMatrixMode(GL_MODELVIEW);             // Return to modelview mode before drawing.
		glLoadIdentity();
		
		glInitNames();  // Initializes the name stack to empty.
		glPushName(0);  // Puts name 0 on top of stack.
		
		drawSelectable();
		
		hits = glRenderMode(GL_RENDER); // Return to rendering mode, returning number of hits.
		cout << "HITS: " << hits << endl;
		// Restore viewing volume of the resize routine and return to modelview mode.
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		
		// Determine closest of the hit objects (if any).
		findClosestHit(hits, buffer);
		glutPostRedisplay();
        
        if(closestName == 1)
        {
			keypad[0] = !keypad[0];
        }
		if(closestName == 2)
        {
			keypad[1] = !keypad[1];
        }
		if(closestName == 3)
        {
			keypad[2] = !keypad[2];
        }
    }
	if(keypad[0] == 1 && keypad[1]==1)
		lockedDoors[0].locked=false;
	glutPostRedisplay();
    
}


// Mouse motion callback routine.
void mouseMotion(int x, int y)
{
	int  difX, difY;
	float radius;
	difX = x-lastX;
	difY = y-lastY;
	theta += difX;
	phi += difY;
	lastX=x;
	lastY=y;
	if (theta >= 360.0){theta = 0.0;}
	radius = sqrt(((centerX-eyeX)*(centerX-eyeX))+((centerZ-eyeZ)*(centerZ-eyeZ)));
	centerZ = eyeZ - radius*sin((theta*PI)/180.0);
	centerX = eyeX- radius*cos((theta*PI)/180.0);
	centerY = eyeY + radius*sin((phi*PI)/180.0);
}



// Timer function.
//known issues: 1st jump larger than rest..
void animate(int value)
{
	float distanceToDoor1 = sqrt((eyeX-0)*(eyeX-0) + (eyeZ+14)*(eyeZ+14));
	float distanceToDoor2 = sqrt((eyeX-0)*(eyeX-0) + (eyeZ+19.0)*(eyeZ+19.0));
	float distanceToDoor3 = sqrt((eyeX-15.0)*(eyeX-15.0) + (eyeZ+26.5)*(eyeZ+26.5));
	if(isJumping)
	{
		cout << "jumpVal: " << jumpVal << endl;
		eyeY+= 2*(-0.5*jumpVal);
		centerY+= 2*(-0.5*jumpVal);
		jumpVal+=0.4;
		if(jumpVal >= 1.1)
		{
			isJumping = false;
			jumpVal = JMP;
		}
		cout << "test: " << eyeY << endl;
	}
	
	if(distanceToDoor1 <= 4.0)
	{
		openFactor += 0.1;
		if(openFactor>=1.0)
			openFactor=1.0;
	}
	else if (openFactor != 0.0)
	{
		openFactor -= 0.1;
		if(openFactor<= 0.0)
			openFactor = 0.0;
	}
	
	if(!lockedDoors[0].locked)
	{
		if(distanceToDoor2 <= 4.0)
		{
            if(lockedDoors[0].openFactor < 1.0)
                lockedDoors[0].openFactor+=0.1;
		}
	}
	
	if(!lockedDoors[1].locked)
	{
		//if close enough to door
		if(distanceToDoor3 <= 4.0)
		{
            if(lockedDoors[1].openFactor< 1.0)
                lockedDoors[1].openFactor+=0.1;
		}
		//else if penfacotr not 0 , close the door
	}
	
	glutTimerFunc(animationPeriod, animate, 1);
	glutPostRedisplay();
}

//at start of program no keys pressed...
void initKeyStates()
{
	for(int i=0; i < 256; i++)
	{
		keyStates[i] = false;
	}
}



// Initialization routine.
void setup(void)
{
    //white background
    glClearColor(1.0, 1.0, 1.0, 0.0);
	glEnable(GL_NORMALIZE);  // normalize vectors for proper shading
    glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	// Enable color material mode.
	glEnable(GL_TEXTURE_2D);
	// Create texture index array.
	glGenTextures(10, texture);
	//set up table top texture
	loadExternalTextures();
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glDepthMask(GL_TRUE);
	//glutSetCursor(GLUT_CURSOR_CROSSHAIR);
	glutSetCursor(GLUT_CURSOR_NONE);
	glutWarpPointer	(500,500);
    //glEnableClientState(GL_VERTEX_ARRAY);
    //theMan = new character(0.0,5.0,-30.0);
	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);
	for(int i = 0; i < MAXPARTICLES; i++)
	{
		createParticle(i);
	}
	initKeyStates();
	initLocks();
	//initParticle(testParticle);
}

// OpenGL window reshape routine.
void resize(int w, int h)
{
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0,1.0,0.1,102.0);
    glMatrixMode(GL_MODELVIEW);
}

// Routine to output interaction instructions to the C++ window.
void printInteraction(void)
{
    cout << "Welcome to my WalkAbout!" << endl;
	cout << "This is a simple first person game." <<endl;
	cout << "The goal is to open the locked doors and make it to the end...alive." << endl;
	cout << "For the first door, you must enter the correct pass code" << endl;
	cout << "W walks forward,\nS walks backwards,\nA strafes left and,\nD strafes right" << endl;
	cout << "Look Around using the mouse." << endl;
	cout << "To start the game, please enter your name: " ;
	cin >>  myName;
    //add directions for interaction
}


//Controls special Keys, such as the arrow keys
void specialKey(int key, int x, int y)
{
    glutPostRedisplay();
}

//handle keyboard input
void keyPressed(unsigned char key, int x, int y)
{
	cout << glutGetModifiers() << endl;
	//**THIS IS MESSED UP!!!!!!!!!!!!!!!!!!!
	if(glutGetModifiers()==GLUT_ACTIVE_SHIFT)
	{
		sprint=true;
		cout << "SPRINT" << endl;
	}
	else sprint=false;
	if(glutGetModifiers()==GLUT_ACTIVE_CTRL)
	{
		croutch=true;
	}
	if(glutGetModifiers()==GLUT_ACTIVE_ALT)
	{
		slowWalk=true;
	}
	if(key=='f'){flashLightOn = !flashLightOn;cout << "FlashLight: " << flashLightOn << endl;} else
        keyStates[key] = true;
    glutPostRedisplay();
}

void keyUp(unsigned char key, int x, int y)
{
	keyStates[key] = false;
    glutPostRedisplay();
}

// Main routine.
int main(int argc, char **argv) 
{
    srand ( time(NULL) );
    printInteraction();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE| GLUT_RGB | GLUT_DEPTH); 
    glutInitWindowSize(1000, 1000);
    glutInitWindowPosition(0, 0); 
    glutCreateWindow("WALK ABOUT");
    setup(); 
    glutDisplayFunc(drawScene); 
    glutReshapeFunc(resize);  
    glutKeyboardFunc(keyPressed);
	glutKeyboardUpFunc(keyUp);
	glutSpecialFunc(specialKey);
    glutMouseFunc(mouseControl);
    glutPassiveMotionFunc(mouseMotion);
    glutTimerFunc(5, animate, 1);
    glutMainLoop();
    return 0;  
}