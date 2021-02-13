/**********************************/
/* Lighting					      
   (C) Bedrich Benes 2021         
   Diffuse and specular per fragment.
   bbenes@purdue.edu               */
/**********************************/

#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <string.h>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <string>
#include <vector>			//Standard template library class
#include <GL/glew.h>
#include <GL/glut.h>
//glm
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/half_float.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include "shaders.h"    
#include "shapes.h"    
#include "lights.h"    

#pragma warning(disable : 4996)
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "glut32.lib")

using namespace std;

bool needRedisplay=false;
ShapesC* sphere;
ShapesC* cube;

//shader program ID
GLuint shaderProgram;
GLfloat ftime=0.f;
glm::mat4 view=glm::mat4(1.0);
glm::mat4 proj=glm::perspective(80.0f,//fovy
				  		        1.0f,//aspect
						        0.01f,1000.f); //near, far

const GLfloat bulletSpeed = 1.0f;
const GLfloat bulletSize = 0.2f;
const GLfloat bladeLength = 2.0f;
const glm::vec3 up = glm::vec3(0.0, 1.0f, 0.0);
const GLfloat rotTimeMult = -20.0f;


glm::vec3 eye      = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 eyeDir   = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 eyeUp    = up;

// Flags for keypresses to enable smooth motion
GLboolean forwardPressed = false;
GLboolean backwardPressed = false;
GLboolean leftPressed = false;
GLboolean rightPressed = false;

GLfloat cameraRotationAngle = 0.0f;
glm::vec3 cameraTranslation = glm::vec3(0.0f, 0.0f, 0.0f);

vector <pair<glm::mat4, Windmill>> windmills;
vector <Bullet> bullets;

class ShaderParamsC
{
public:
	GLint modelParameter;		//modeling matrix
	GLint modelViewNParameter;  //modeliview for normals
	GLint viewParameter;		//viewing matrix
	GLint projParameter;		//projection matrix
	//material
	GLint kaParameter;			//ambient material
	GLint kdParameter;			//diffuse material
	GLint ksParameter;			//specular material
	GLint shParameter;			//shinenness material
} params;


LightC light;


//the main window size
GLint wWindow=800;
GLint hWindow=800;

float sh=200;

/*********************************
Some OpenGL-related functions
**********************************/

//called when a window is reshaped
void Reshape(int w, int h)
{
	glViewport(0,0,w, h);       
    glEnable(GL_DEPTH_TEST);
	//remember the settings for the camera
    wWindow=w;
    hWindow=h;
}

void Arm(glm::mat4 m)
{
	//let's use instancing
	m=glm::translate(m,glm::vec3(0,0.5,0.0));
	m=glm::scale(m,glm::vec3(1.0f,1.0f,1.0f));
	sphere->SetModel(m);
	//now the normals
	glm::mat3 modelViewN=glm::mat3(view*m);
	modelViewN= glm::transpose(glm::inverse(modelViewN));
	sphere->SetModelViewN(modelViewN);
	sphere->Render();

	m=glm::translate(m,glm::vec3(0.0,0.5,0.0));
	m=glm::rotate(m,-20.0f*ftime,glm::vec3(0.0,0.0,1.0));
	m=glm::translate(m,glm::vec3(0.0,1.5,0.0));
	sphere->SetModel(glm::scale(m,glm::vec3(0.5f,1.0f,0.5f)));

	modelViewN=glm::mat3(view*m);
	modelViewN= glm::transpose(glm::inverse(modelViewN));
	sphere->SetModelViewN(modelViewN);
	sphere->Render();
}

void DrawWindmill(glm::mat4 model, Windmill wm)
{
	// Draw the center sphere
	model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
	sphere->SetModel(model);
	glm::mat3 modelViewN = glm::mat3(view * model);
	modelViewN = glm::transpose(glm::inverse(modelViewN));
	sphere->SetModelViewN(modelViewN);
	sphere->Render();

	// To change speed when blades lost, check numBlades and assign a multiplier to rotTimeMult
	GLint bladeID = 0;

	for (vector<GLboolean>::iterator it = wm.blades.begin(); it != wm.blades.end(); ++it, ++bladeID) {
		//Create new model matrix so next blade isn't screwed up
		glm::mat4 bladeModel = model;
		if (*it) { // If the blade is still alive 

			// Draw a blade
			GLfloat rotAngle = ((360.0f / wm.numBlades) * bladeID) + (ftime * rotTimeMult);
			 
			bladeModel = glm::rotate(bladeModel, rotAngle, glm::vec3(0.0, 0.0, 1.0));
			bladeModel = glm::translate(bladeModel, glm::vec3(0.0, 1.0, 0.0));
			sphere->SetModel(glm::scale(bladeModel, glm::vec3(0.75f, bladeLength, 0.75f)));

			glm::mat3 bladeModelViewN = glm::mat3(view * bladeModel);
			bladeModelViewN = glm::transpose(glm::inverse(bladeModelViewN));
			sphere->SetModelViewN(bladeModelViewN);
			sphere->Render();

		}
	}

}

void DrawBullet(Bullet bullet)
{
	// Save the old color values so we can put them back
	glm::vec3 kaOld = sphere->GetKa();
	glm::vec3 kdOld = sphere->GetKd();
	glm::vec3 ksOld = sphere->GetKs();
	float shOld = sphere->GetSh();

	// Make the bullet sphere look like pewter
	sphere->SetKa(glm::vec3(0.105882f, 0.058824f, 0.113725f));
	sphere->SetKd(glm::vec3(0.427451f, 0.470588f, 0.541176f));
	sphere->SetKs(glm::vec3(0.333333f, 0.333333f, 0.521569f));
	sphere->SetSh(9.84615f);

	// Draw tiny sphere at position of bullet
	glm::vec3 currentPos = bullet.startPos + ((ftime - bullet.startTime) * bulletSpeed * bullet.direction);
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, currentPos);
	sphere->SetModel(glm::scale(model, glm::vec3(bulletSize, bulletSize, bulletSize)));
	glm::mat3 modelViewN = glm::mat3(view * model);
	modelViewN = glm::transpose(glm::inverse(modelViewN));
	sphere->SetModelViewN(modelViewN);
	sphere->Render();


	// Put old colors back on the sphere
	sphere->SetKa(kaOld);
	sphere->SetKd(kdOld);
	sphere->SetKs(ksOld);
	sphere->SetSh(shOld);
}

void FireBullet() 
{
	glm::vec3 movedEye = eye + cameraTranslation;
	glm::vec3 movedEyeDir = glm::rotate(eyeDir, cameraRotationAngle, up) + cameraTranslation;
	glm::vec3 direction = glm::normalize(movedEyeDir - movedEye);
	glm::vec3 startPos = movedEye;
	Bullet tmp = Bullet();
	tmp.shoot(ftime, startPos, direction);
	bullets.push_back(tmp);
}

void Colissions()
{
	vector<vector<pair<glm::mat4, Windmill>>::iterator> WindmillsToKill;
	// Check every bullet with every blade, blades have a bounding sphere around them of radius 2 
	for(auto windmillit = windmills.begin(); windmillit != windmills.end(); ++windmillit)
	{
		Windmill* wm = &((*windmillit).second);
		glm::mat4 wmModel = (*windmillit).first;

		GLint bladeID = 0;
		for (vector<GLboolean>::iterator bladeit = wm->blades.begin(); bladeit != wm->blades.end(); ++bladeit, ++bladeID){
			if (!wm->blades[bladeID]){ continue; } // Only check blades that are visible

			// Need to get point of center of blade
			glm::mat4 bladeModel = wmModel;
			GLfloat rotAngle = ((360.0f / wm->numBlades) * bladeID) + (ftime * rotTimeMult);
			bladeModel = glm::rotate(bladeModel, rotAngle, glm::vec3(0.0, 0.0, 1.0));
			bladeModel = glm::translate(bladeModel, glm::vec3(0.0, 2.0, 0.0));
			glm::vec3 bladeCenter = glm::vec3(bladeModel * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

			// Iterate through all the bullets and check bounding sphere proximity
			vector<vector<Bullet>::iterator> BulletsToKill;
			for (vector<Bullet>::iterator it = bullets.begin(); it != bullets.end(); ++it) {
				glm::vec3 bulletPos = it->startPos + ((ftime - it->startTime) * bulletSpeed * it->direction);

				if (glm::distance(bulletPos, bladeCenter) < (bulletSize + bladeLength - 0.75f)) { // HIT!
					// Kill the bullet and that blade of the windmill
					BulletsToKill.push_back(it);
					*bladeit = false;
					wm->bladesLeft--;

				} else if(glm::distance(bulletPos, bladeCenter) > 400 && false){ BulletsToKill.push_back(it); } // Cull bullets that are far away
			}
			// Cull bullets that hit something or are far away
			for (auto& bullet : BulletsToKill) { 
				bullets.erase(bullet); 
			}
		}
		if (wm->bladesLeft <= 0) { WindmillsToKill.push_back(windmillit); }
	}
	// Cull windmills with no blades left
	for (auto& windmill : WindmillsToKill) { windmills.erase(windmill); }
}

//the main rendering function
void RenderObjects()
{
	//const int range=3;
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glColor3f(0,0,0);
	glPointSize(2);
	glLineWidth(1);
	//set the projection and view once for the scene
	glUniformMatrix4fv(params.projParameter,1,GL_FALSE,glm::value_ptr(proj));

	glm::vec3 movedEye = eye + cameraTranslation; // Eye after being translated
	glm::vec3 movedEyeDir = glm::rotate(eyeDir, cameraRotationAngle, up) + cameraTranslation; // Eye direction after being translated... and rotated
	view=glm::lookAt(movedEye,//eye
				     movedEyeDir,  //destination
				     eyeUp); //up)


	glUniformMatrix4fv(params.viewParameter,1,GL_FALSE,glm::value_ptr(view));
	//set the light
	static glm::vec4 pos;
	pos.x=20*sin(ftime/12);pos.y=-10;pos.z=20*cos(ftime/12);pos.w=1;
	light.SetPos(pos);
	light.SetShaders();

	// Render all the windmills
	for (auto& windmill : windmills){ DrawWindmill(windmill.first, windmill.second); }

	// Render all the bullets
	for (auto& bullet : bullets) { DrawBullet(bullet); }
}
	
void Idle(void)
{
    glClearColor(0.1,0.1,0.1,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    ftime+=0.05;

	// Adjust movement if keys are pressed
	if (forwardPressed) {
		//Get rotated eye and eyedir
		//glm::rotate(eyeDir, cameraRotationAngle, glm::vec3(0.0f, 0.1f, 0.0f));
		glm::vec3 eyeDirRotated = glm::rotate(eyeDir, cameraRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
		cameraTranslation += glm::normalize(eyeDirRotated - eye) / 10.0f;
	}
	else if (backwardPressed) {
		glm::vec3 eyeDirRotated = glm::rotate(eyeDir, cameraRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
		cameraTranslation -= glm::normalize(eyeDirRotated - eye) / 10.0f;
	}
	if (leftPressed) {
		cameraRotationAngle += 1.0f;
	}
	else if (rightPressed) {
		cameraRotationAngle -= 1.0f;
	}
    glUseProgram(shaderProgram);

	Colissions();

    RenderObjects();
    glutSwapBuffers();  
}

void Display(void)
{

}

//keyboard callback
void Kbd(unsigned char a, int x, int y)
{
	switch(a)
	{
 	  case 27 : exit(0);break;
	  case 'r': 
	  case 'R': {sphere->SetKd(glm::vec3(1,0,0));break;}
	  case 'g': 
	  case 'G': {sphere->SetKd(glm::vec3(0,1,0));break;}
	  case 'b': 
	  case 'B': {sphere->SetKd(glm::vec3(0,0,1));break;}
	  case 'w': 
	  case 'W': {sphere->SetKd(glm::vec3(0.7,0.7,0.7));break;}
	  case '+': {sphere->SetSh(sh+=1);break;}
	  case '-': {sphere->SetSh(sh-=1);if (sh<1) sh=1;break;}
	  case 32: { FireBullet();break;}
	}
	cout << "shineness="<<sh<<endl;
	glutPostRedisplay();
}


//special keyboard callback
void SpecKbdPress(int a, int x, int y)
{
   	switch(a)
	{
 	  case GLUT_KEY_LEFT: {
		  leftPressed = true;
		  break;
		  }
	  case GLUT_KEY_RIGHT: {
		  rightPressed = true;
		  break;
		  }
 	  case GLUT_KEY_DOWN: {
		  backwardPressed = true;
		  break;
		  }
	  case GLUT_KEY_UP:{
		  forwardPressed = true;
		  break;
		  }

	}
	glutPostRedisplay();
}

//called when a special key is released
void SpecKbdRelease(int a, int x, int y)
{
	switch(a)
	{
	case GLUT_KEY_LEFT: {
		leftPressed = false;
		break;
	}
	case GLUT_KEY_RIGHT: {
		rightPressed = false;
		break;
	}
	case GLUT_KEY_DOWN: {
		backwardPressed = false;
		break;
	}
	case GLUT_KEY_UP: {
		forwardPressed = false;
		break;
	}
	}
	glutPostRedisplay();
}


void Mouse(int button,int state,int x,int y)
{
	cout << "Location is " << "[" << x << "'" << y << "]" << endl;
}


void InitializeProgram(GLuint *program)
{
	std::vector<GLuint> shaderList;

//load and compile shaders 	
	shaderList.push_back(CreateShader(GL_VERTEX_SHADER,   LoadShader("shaders/phong.vert")));
	shaderList.push_back(CreateShader(GL_FRAGMENT_SHADER, LoadShader("shaders/phong.frag")));

//create the shader program and attach the shaders to it
	*program = CreateProgram(shaderList);

//delete shaders (they are on the GPU now)
	std::for_each(shaderList.begin(), shaderList.end(), glDeleteShader);

	params.modelParameter=glGetUniformLocation(*program,"model");
	params.modelViewNParameter=glGetUniformLocation(*program,"modelViewN");
	params.viewParameter =glGetUniformLocation(*program,"view");
	params.projParameter =glGetUniformLocation(*program,"proj");
	//now the material properties
	params.kaParameter=glGetUniformLocation(*program,"mat.ka");
	params.kdParameter=glGetUniformLocation(*program,"mat.kd");
	params.ksParameter=glGetUniformLocation(*program,"mat.ks");
	params.shParameter=glGetUniformLocation(*program,"mat.sh");
	//now the light properties
	light.SetLaToShader(glGetUniformLocation(*program,"light.la"));
	light.SetLdToShader(glGetUniformLocation(*program,"light.ld"));
	light.SetLsToShader(glGetUniformLocation(*program,"light.ls"));
	light.SetLposToShader(glGetUniformLocation(*program,"light.lPos"));
}

void InitShapes(ShaderParamsC* params)
{
	//create one unit sphere in the origin
	sphere = new SphereC(50, 50, 1);
	sphere->SetKa(glm::vec3(0.1, 0.1, 0.1));
	sphere->SetKs(glm::vec3(0, 0, 1));
	sphere->SetKd(glm::vec3(0.7, 0.7, 0.7));
	sphere->SetSh(sh);
	sphere->SetModel(glm::mat4(1.0));
	sphere->SetModelMatrixParamToShader(params->modelParameter);
	sphere->SetModelViewNMatrixParamToShader(params->modelViewNParameter);
	sphere->SetKaToShader(params->kaParameter);
	sphere->SetKdToShader(params->kdParameter);
	sphere->SetKsToShader(params->ksParameter);
	sphere->SetShToShader(params->shParameter);

	// Create some random points and make windmills from them
	GLint numWindmills = 4;
	srand(time(0));
	for (int i = 0; i < numWindmills; i++)
	{
		glm::mat4 model = glm::mat4(1.0);
		glm::vec3 position = glm::vec3((rand() % 30) - 15, 0.0f,( rand() % 80) - 100);
		model = glm::translate(model, position);
		cout << "Making windmill at: \n" << glm::to_string(position) << endl;
		Windmill w = Windmill(4);
		windmills.push_back(pair<glm::mat4, Windmill>(model, w));
	}
}

int main(int argc, char **argv)
{ 
  glutInitDisplayString("stencil>=2 rgb double depth samples");
  glutInit(&argc, argv);
  glutInitWindowSize(wWindow,hWindow);
  glutInitWindowPosition(500,100);
  glutCreateWindow("Model View Projection GLSL");
  GLenum err = glewInit();
  if (GLEW_OK != err){
   fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
  }
  glutDisplayFunc(Display);
  glutIdleFunc(Idle);
  glutMouseFunc(Mouse);
  glutReshapeFunc(Reshape);
  glutKeyboardFunc(Kbd); //+ and -
  glutSpecialUpFunc(SpecKbdRelease); //smooth motion
  glutSpecialFunc(SpecKbdPress);
  glutIgnoreKeyRepeat(true);
  InitializeProgram(&shaderProgram);
  InitShapes(&params);
  glutMainLoop();
  return 0;        
}
	