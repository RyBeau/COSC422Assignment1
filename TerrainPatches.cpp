/*
 * COSC442 Assignment 1
 * Terrain Modelling
 * Ryan Beaumont 31591316
 * Adapted from the terrain modelling programming exercise.
 *
 * */
#define  GLM_FORCE_RADIANS
#include <iostream>
#include <sstream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "loadTGA.h"
#include <glm/gtc/type_ptr.hpp>
using namespace std;

//Globals
GLuint vaoID;
GLuint mvpMatrixLoc;
float CDR = 3.14159265/180.0;     //Conversion from degrees to rad (required in GLM 0.9.6)
float verts[100*3];       //10x10 grid (100 vertices)
GLushort elems[81*4];       //Element array for 81 quad patches
glm::mat4 projView;
glm::mat4 proj, view;   //Projection and view matrices
GLuint eyePosLoc;
GLuint mvMatrixLoc, norMatrixLoc; //Model-view and normal transformation matrix

//Wireframe Toggle
bool wireframe = false;

//Texture Globals
GLuint heightMap;
GLuint texID[5];
GLuint snowHeightLoc;
GLuint waterHeightLoc;
GLuint lgtLoc;
float currentWaterHeight = 2.0;
float currentSnowHeight = 6.0;

//Fog
GLuint fogLoc;
bool enableFog = false;

//Tessellation
GLuint tessellationModeLoc;
bool highTessellation= true;

//Camera Globals
float speed = 2.0;
float camX = 0.0;
float camZ = 30.0;
float camY = 20.0;
float angle = -90.0 * CDR;
glm::vec3 cameraPos   = glm::vec3(camX, camY, camZ);
glm::vec3 cameraFront = glm::vec3(camX+cos(angle), camY, camZ+sin(angle));
glm::vec3 cameraUp = glm::vec3(0.0, 1.0, 0.0);

//Generate vertex and element data for the terrain floor
void generateData()
{
	int indx, start;
	//verts array
	for(int i = 0; i < 10; i++)   //100 vertices on a 10x10 grid
	{
		for(int j = 0; j < 10; j++)
		{
			indx = 10*i + j;
			verts[3*indx] = 10*i - 45;		//x
			verts[3*indx+1] = 0;			//y
			verts[3*indx+2] = -10*j;		//z
		}
	}

	//elems array
	for(int i = 0; i < 9; i++)
	{
		for(int j = 0; j < 9; j++)
		{
			indx = 9*i +j;
			start = 10*i + j;
			elems[4*indx] = start;
			elems[4*indx+1] = start+10;
			elems[4*indx+2] = start+11;
			elems[4*indx+3] = start+1;			
		}
	}
}

/*
 * Loads the MtRupehu.tga and MtCook.tga height maps.
 * */
void loadHeightMaps(){
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID[0]);
    loadTGA("./HeightMaps/MtRuapehu.tga");

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texID[1]);
    loadTGA("./HeightMaps/MtCook.tga");


    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

/*
 * Loads the textures used in the height based texturing of the height map.
 * */
void loadTextures(){
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texID[2]);
    loadTGA("./Textures/grass.tga");

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, texID[3]);
    loadTGA("./Textures/water.tga");


    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, texID[4]);
    loadTGA("./Textures/snow.tga");


    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

//Loads all TGA files
void loadTGAs()
{
    glGenTextures(5, texID);
    loadHeightMaps();
    loadTextures();
}


//Loads a shader file and returns the reference to a shader object
GLuint loadShader(GLenum shaderType, string filename)
{
	ifstream shaderFile(filename.c_str());
	if(!shaderFile.good()) cout << "Error opening shader file." << endl;
	stringstream shaderData;
	shaderData << shaderFile.rdbuf();
	shaderFile.close();
	string shaderStr = shaderData.str();
	const char* shaderTxt = shaderStr.c_str();

	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderTxt, NULL);
	glCompileShader(shader);
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);
		const char *strShaderType = NULL;
		cerr <<  "Compile failure in shader: " << strInfoLog << endl;
		delete[] strInfoLog;
	}
	return shader;
}

/*
 * Calculates all the transformation matrices used throughout the shader stages.
 * Also calculates the light position in eye space.
 * */
void calculateMatrices(){
    glm::vec4 light = glm::vec4(-50.0, 10.0, 60.0, 1.0);
    glm::mat4 mvpMatrix = proj * view;   //The model-view-projection transformation
    glm::mat4 invMatrix = glm::inverse(view);  //Inverse of model-view matrix for normal transformation
    glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, &mvpMatrix[0][0]);
    glUniformMatrix4fv(norMatrixLoc, 1, GL_TRUE, &invMatrix[0][0]);
    glUniformMatrix4fv(mvMatrixLoc, 1, GL_FALSE, &view[0][0]);
    glm::vec4 lightEye = view * light;
    glUniform4fv(lgtLoc, 1, &lightEye[0]);
}

//Initialise the shader program, create and load buffer data
void initialise()
{

//--------Textures-----------
	loadTGAs();
	//--------Load shaders----------------------
	GLuint shaderv = loadShader(GL_VERTEX_SHADER, "TerrainPatches.vert");
	GLuint shaderf = loadShader(GL_FRAGMENT_SHADER, "TerrainPatches.frag");
	GLuint shaderc = loadShader(GL_TESS_CONTROL_SHADER, "TerrainPatches.cont.glsl");
	GLuint shadere = loadShader(GL_TESS_EVALUATION_SHADER, "TerrainPatches.eval.glsl");
	GLuint shaderg = loadShader(GL_GEOMETRY_SHADER, "TerrainPatches.geom");

	//--------Attach shaders---------------------
	GLuint program = glCreateProgram();
	glAttachShader(program, shaderv);
	glAttachShader(program, shaderf);
	glAttachShader(program, shaderc);
	glAttachShader(program, shadere);
	glAttachShader(program, shaderg);

	glLinkProgram(program);

	GLint status;
	glGetProgramiv (program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
		fprintf(stderr, "Linker failure: %s\n", strInfoLog);
		delete[] strInfoLog;
	}
	glUseProgram(program);

	// Sets uniform variable locations.
	eyePosLoc = glGetUniformLocation(program, "eyePos");
	mvpMatrixLoc = glGetUniformLocation(program, "mvpMatrix");
	mvMatrixLoc = glGetUniformLocation(program, "mvMatrix");
	norMatrixLoc = glGetUniformLocation(program, "norMatrix");
	heightMap = glGetUniformLocation(program, "heightMap");
	lgtLoc = glGetUniformLocation(program, "lightPos");
	fogLoc = glGetUniformLocation(program, "fogEnabled");
	tessellationModeLoc = glGetUniformLocation(program, "highTessellation");
	glUniform1i(heightMap, 0);

//--------Compute matrices----------------------
	proj = glm::perspective(30.0f*CDR, 1.25f, 20.0f, 500.0f);  //perspective projection matrix
	view = glm::lookAt(cameraPos, cameraFront, cameraUp); //view matrix
	projView = proj * view;  //Product matrix
	glUniform3fv(eyePosLoc, 1, glm::value_ptr(cameraPos));

// Setup texture samplers and water and snow height
    GLuint grassLoc = glGetUniformLocation(program, "grassSampler");
    glUniform1i(grassLoc, 2);
    GLuint waterLoc = glGetUniformLocation(program, "waterSampler");
    glUniform1i(waterLoc, 3);
    GLuint snowLoc = glGetUniformLocation(program, "snowSampler");
    glUniform1i(snowLoc, 4);
    snowHeightLoc = glGetUniformLocation(program, "snowHeight");
    glUniform1f(snowHeightLoc, currentSnowHeight);
    waterHeightLoc = glGetUniformLocation(program, "waterHeight");
    glUniform1f(waterHeightLoc, currentWaterHeight);
//---------Load buffer data-----------------------
	generateData();

	GLuint vboID[2];
	glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);

    glGenBuffers(2, vboID);

    glBindBuffer(GL_ARRAY_BUFFER, vboID[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);  // Vertex position

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboID[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elems), elems, GL_STATIC_DRAW);

    glBindVertexArray(0);

	glPatchParameteri(GL_PATCH_VERTICES, 4);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

//Display function to compute uniform values based on transformation parameters and to draw the scene
void display()
{
    calculateMatrices();
    glUniform1i(tessellationModeLoc, highTessellation);
    glUniform1i(fogLoc, enableFog);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glBindVertexArray(vaoID);
	glDrawElements(GL_PATCHES, 81*4, GL_UNSIGNED_SHORT, NULL);
	glFlush();
}

/*
 * Changes the currently used height map.
 * */
void changeHeightMap(int texture){
    glUniform1i(heightMap, texture);
}

/*
 * Toggles between solid and wireframe modes.
 * */
void toggleWireframe(){
    if(wireframe){
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    wireframe = !wireframe;
}

/*
 * Adjusts the snow height within the bounds of the maximum height and
 *  the current water level.
 * */
void changeSnowHeight(int direction){
    if(currentSnowHeight > 0.5 + currentWaterHeight && direction == -1){
        currentSnowHeight += 0.1 * direction;
        glUniform1f(snowHeightLoc, currentSnowHeight);
    } else if (direction == 1 && currentSnowHeight < 9.0){
        currentSnowHeight += 0.1 * direction;
        glUniform1f(snowHeightLoc, currentSnowHeight);
    }
}

/*
 * Adjusts the water height within the bounds of the minimum height and
 *  the current snow level.
 * */
void changeWaterHeight(int direction){
    if(currentSnowHeight - 0.5 > currentWaterHeight && direction == 1){
        currentWaterHeight += 0.1 * direction;
        glUniform1f(waterHeightLoc, currentWaterHeight);
    } else if (direction == -1 && currentWaterHeight > 2.0){
        currentWaterHeight += 0.1 * direction;
        glUniform1f(waterHeightLoc, currentWaterHeight);
    }
}

/*
 * Key press function for standard keys
 * */
void onKeyPress(unsigned char key, int x, int y){
    switch (key) {
        case '1':
            changeHeightMap(0);
            break;
        case '2':
            changeHeightMap(1);
            break;
        case ' ':
            fprintf(stderr, "Space");
            toggleWireframe();
            break;
        case 'w':
            changeSnowHeight(1);
            break;
        case 's':
            changeSnowHeight(-1);
            break;
        case 'q':
            changeWaterHeight(1);
            break;
        case 'a':
            changeWaterHeight(-1);
            break;
        case 'f':
            enableFog = !enableFog;
            break;
        case 't':
            highTessellation = ! highTessellation;
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

/*
 * Calculates the new position of the camera when moved with the up and down arrow keys.
 * */
void calculateMove(float direction) {
    float lookX = cameraFront[0];
    float lookZ = cameraFront[2];
    float moveX = lookX - camX;
    float moveZ = lookZ - camZ;
    if (abs(moveX)>abs(moveZ)){
        moveZ /= abs(moveX);
        moveX /= abs(moveX);
    } else {
        moveX /= abs(moveZ);
        moveZ /= abs(moveZ);
    }
    float newX = camX + moveX * direction * speed;
    float newZ = camZ + moveZ * direction * speed;
    if (newX > 150.0){
        newX = 150.0;
        moveX = 0.0;
    }
    if (newX < -150.0){
        newX = -150.0;
        moveX = 0.0;
    }
    if (newZ < -150.0){
        newZ = -150.0;
        moveZ = 0.0;
    }
    if (newZ > 120.0){
        newZ = 120.0;
        moveZ = 0.0;
    }
    camX = newX;
    camZ = newZ;

    cameraPos = glm::vec3 (camX,camY,camZ);
    cameraFront += glm::vec3 (moveX * direction * speed,0.0,moveZ * direction * speed);
    view = glm::lookAt(cameraPos, cameraFront, cameraUp);
    glUniform3fv(eyePosLoc, 1, glm::value_ptr(cameraPos));
}

/*
 * Rotates the camera when the left and right arrow keys are pressed.
 * */
void rotateCamera(float direction){
    angle += direction * 5 * CDR;
    cameraFront = glm::vec3(camX+cos(angle), camY, camZ+sin(angle));
    view = glm::lookAt(cameraPos, cameraFront, cameraUp);
}

/*
 * Key press function for the special keys.
 * */
void onSpecialKey(int key, int x, int y){
    switch(key){
        case GLUT_KEY_UP:
            calculateMove(1);
            break;
        case GLUT_KEY_DOWN:
            calculateMove(-1);
            break;
        case GLUT_KEY_LEFT:
            rotateCamera(-1);
            break;
        case GLUT_KEY_RIGHT:
            rotateCamera(1);
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_DEPTH);
	glutInitWindowSize(1000, 800);
	glutCreateWindow("Terrain");
	glutInitContextVersion (4, 2);
	glutInitContextProfile ( GLUT_CORE_PROFILE );

	if(glewInit() == GLEW_OK)
	{
		cout << "GLEW initialization successful! " << endl;
		cout << " Using GLEW version " << glewGetString(GLEW_VERSION) << endl;
	}
	else
	{
		cerr << "Unable to initialize GLEW  ...exiting." << endl;
		exit(EXIT_FAILURE);
	}

	initialise();
	glutKeyboardFunc(onKeyPress);
	glutDisplayFunc(display);
    glutSpecialFunc(onSpecialKey);
	glutMainLoop();
}

