#include <iostream>
#include <cmath>
#include <cstdio>
#include <cstdlib>

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// Other Libs
#include "Graphics/stb_image.h"

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//Load Models
#include "SOIL2/SOIL2.h"

// Other includes
#include "Graphics/Shader.h"
#include "Core/Camera.h"
#include "Graphics/Model.h"

// Function prototypes
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();
void Animation();

// Funciones Keyframes
void saveFrame(void);
void resetElements(void);
void interpolation(void);

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Camara original (libre: WASD + mouse). Tecla KP 0
Camera camera(glm::vec3(21.292f, 1.498f, -2.335f));
GLfloat lastX = WIDTH / 2.0f;
GLfloat lastY = HEIGHT / 2.0f;
bool keys[1024];
bool firstMouse = true;

// Camaras fijas (teclas KP 1 a KP 4): solo giran con el mouse
const int NUM_CAMERA = 4;
glm::vec3 cameraPositions[NUM_CAMERA] = {
	glm::vec3(20.95f, 1.80f, -2.58f),
	glm::vec3(8.92f, 1.07f, -2.61f),
	glm::vec3(2.30f, 1.07f, -2.46f),
	glm::vec3(-1.30f, 1.64f, 0.37f)
};

// Punto inicial al que mira cada camara fija (solo para calcular su angulo inicial)
glm::vec3 cameraTargets[NUM_CAMERA] = {
	glm::vec3(0.0f, 1.5f, -2.5f),
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(0.0f, 1.0f, -2.0f)
};

// Angulos de giro de cada camara fija (grados)
float cameraYaw[NUM_CAMERA];
float cameraPitch[NUM_CAMERA];
const float cameraSensitivity = 0.1f;

int correntCamera = 0;        // indice de la camara fija activa
bool useFreeCamera = true;    // true = camara original libre
bool showInfoPanel = false;

float rotacionPeces = 0.0f;
float boatT = 0.0f;
bool boatMoving = false;
float bgBoat1Angle = 0.0f;
float bgBoat2Angle = 180.0f;

// Variables de animacion
float charPosX = 0.0f, charPosY = 0.0f, charPosZ = 0.0f;
float charRotY = 0.0f;

float torsoPitch = 0.0f;
float headRot = 0.0f;
float leftArmRot = 0.0f, rightArmRot = 0.0f;
float leftLegRot = 0.0f, rightLegRot = 0.0f;

// Pivotes articulares medidos de los .obj (coordenadas de la escena).
// Como el eje de rotacion es Z, la coordenada Z del pivote no influye.
const glm::vec3 pivotHip(-0.7571f, 1.3808f, -0.8875f);       // base del torso
const glm::vec3 pivotNeck(-0.7570f, 1.4669f, -0.8877f);      // base de la cabeza
const glm::vec3 pivotLShoulder(-0.7538f, 1.4618f, -0.8505f); // hombro izquierdo
const glm::vec3 pivotRShoulder(-0.7538f, 1.4618f, -0.9247f); // hombro derecho
const glm::vec3 pivotLHip(-0.7586f, 1.3808f, -0.8761f);      // cadera izquierda
const glm::vec3 pivotRHip(-0.7587f, 1.3808f, -0.8991f);      // cadera derecha

// Rota una parte alrededor de su articulacion sobre el eje Z:
// M = P * T(p) * Rz(angulo) * T(-p)
static glm::mat4 rotAround(const glm::mat4& parent, const glm::vec3& pivot, float degrees) {
	glm::mat4 m = glm::translate(parent, pivot);
	m = glm::rotate(m, glm::radians(degrees), glm::vec3(0.0f, 0.0f, 1.0f));
	m = glm::translate(m, -pivot);
	return m;
}

// Convierte yaw y pitch (grados) en un vector de direccion de vista
static glm::vec3 frontFromAngles(float yawDeg, float pitchDeg) {
	glm::vec3 f;
	f.x = cos(glm::radians(yawDeg)) * cos(glm::radians(pitchDeg));
	f.y = sin(glm::radians(pitchDeg));
	f.z = sin(glm::radians(yawDeg)) * cos(glm::radians(pitchDeg));
	return glm::normalize(f);
}

typedef struct _frame {
	float charPosX, charPosY, charPosZ, charRotY;
	float incX, incY, incZ, charRotYInc;

	float torsoPitch, torsoPitchInc;
	float headRot, headRotInc;
	float leftArmRot, leftArmRotInc, rightArmRot, rightArmRotInc;
	float leftLegRot, leftLegRotInc, rightLegRot, rightLegRotInc;
} FRAME;

#define MAX_FRAMES 10
FRAME KeyFrame[MAX_FRAMES];
int FrameIndex = 0;
bool play = false;
int playIndex = 0;
int i_max_steps = 90;
int i_curr_steps = 0;

void saveFrame(void) {
	if (FrameIndex >= MAX_FRAMES) return;
	KeyFrame[FrameIndex] = { charPosX, charPosY, charPosZ, charRotY, 0,0,0,0, torsoPitch, 0, headRot, 0, leftArmRot, 0, rightArmRot, 0, leftLegRot, 0, rightLegRot, 0 };
	printf("Frame guardado %d\n", FrameIndex);
	FrameIndex++;
}

void resetElements(void) {
	charPosX = KeyFrame[0].charPosX; charPosY = KeyFrame[0].charPosY; charPosZ = KeyFrame[0].charPosZ;
	charRotY = KeyFrame[0].charRotY; torsoPitch = KeyFrame[0].torsoPitch; headRot = KeyFrame[0].headRot;
	leftArmRot = KeyFrame[0].leftArmRot; rightArmRot = KeyFrame[0].rightArmRot;
	leftLegRot = KeyFrame[0].leftLegRot; rightLegRot = KeyFrame[0].rightLegRot;
}

void interpolation(void) {
	KeyFrame[playIndex].incX = (KeyFrame[playIndex + 1].charPosX - KeyFrame[playIndex].charPosX) / i_max_steps;
	KeyFrame[playIndex].incY = (KeyFrame[playIndex + 1].charPosY - KeyFrame[playIndex].charPosY) / i_max_steps;
	KeyFrame[playIndex].incZ = (KeyFrame[playIndex + 1].charPosZ - KeyFrame[playIndex].charPosZ) / i_max_steps;
	KeyFrame[playIndex].charRotYInc = (KeyFrame[playIndex + 1].charRotY - KeyFrame[playIndex].charRotY) / i_max_steps;
	KeyFrame[playIndex].torsoPitchInc = (KeyFrame[playIndex + 1].torsoPitch - KeyFrame[playIndex].torsoPitch) / i_max_steps;
	KeyFrame[playIndex].headRotInc = (KeyFrame[playIndex + 1].headRot - KeyFrame[playIndex].headRot) / i_max_steps;
	KeyFrame[playIndex].leftArmRotInc = (KeyFrame[playIndex + 1].leftArmRot - KeyFrame[playIndex].leftArmRot) / i_max_steps;
	KeyFrame[playIndex].rightArmRotInc = (KeyFrame[playIndex + 1].rightArmRot - KeyFrame[playIndex].rightArmRot) / i_max_steps;
	KeyFrame[playIndex].leftLegRotInc = (KeyFrame[playIndex + 1].leftLegRot - KeyFrame[playIndex].leftLegRot) / i_max_steps;
	KeyFrame[playIndex].rightLegRotInc = (KeyFrame[playIndex + 1].rightLegRot - KeyFrame[playIndex].rightLegRot) / i_max_steps;
}

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

int main()
{
	glfwInit();
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Proyecto final", nullptr, nullptr);
	if (nullptr == window) { glfwTerminate(); return EXIT_FAILURE; }
	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glewExperimental = GL_TRUE;
	if (GLEW_OK != glewInit()) return EXIT_FAILURE;
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	// Angulos iniciales de cada camara fija a partir de su punto de mira
	for (int i = 0; i < NUM_CAMERA; i++) {
		glm::vec3 d = glm::normalize(cameraTargets[i] - cameraPositions[i]);
		cameraPitch[i] = glm::degrees(asin(d.y));
		cameraYaw[i] = glm::degrees(atan2(d.z, d.x));
	}

	Shader lightingShader("assets/shader/lighting.vs", "assets/shader/lighting.frag");
	Shader overlayShader("assets/shader/overlay.vs", "assets/shader/overlay.frag");

	Model Isla((char*)"assets/modelos/Isla.obj");
	Model Casas((char*)"assets/modelos/casas.obj");
	Model Cementerio((char*)"assets/modelos/cementerio.obj");
	Model Panteon((char*)"assets/modelos/panteonM.obj");
	Model Iglesia((char*)"assets/modelos/iglesia.obj");
	Model Muelles((char*)"assets/modelos/muelles.obj");
	Model SK((char*)"assets/modelos/SK.obj");
	Model Boat((char*)"assets/modelos/boat.obj");
	Model BodyBoat((char*)"assets/modelos/BodyBoat.obj");
	Model PaddleLeft((char*)"assets/modelos/PaddleLeft.obj");
	Model PaddleRight((char*)"assets/modelos/PaddleRight.obj");
	Model Velas((char*)"assets/modelos/velas.obj");
	Model Velas2((char*)"assets/modelos/velas2.obj");
	Model Fuente((char*)"assets/modelos/fuente.obj");
	Model Pasto((char*)"assets/modelos/pasto.obj");
	Model Peces((char*)"assets/modelos/peces.obj");
	Model Peces1((char*)"assets/modelos/peces1.obj");
	Model Peces2((char*)"assets/modelos/peces2.obj");
	Model Monumento((char*)"assets/modelos/monumento.obj");
	Model Head((char*)"assets/modelos/head.obj");
	Model Torso((char*)"assets/modelos/torso.obj");
	Model LeftArm((char*)"assets/modelos/left_Arm.obj");
	Model RightArm((char*)"assets/modelos/right_Arm.obj");
	Model LeftLeg((char*)"assets/modelos/left_Leg.obj");
	Model RightLeg((char*)"assets/modelos/right_Leg.obj");
	Model Camino((char*)"assets/modelos/caminos.obj");
	Model Altar1((char*)"assets/modelos/altar1.obj");
	Model Entrada((char*)"assets/modelos/entrada.obj");
	Model Entrada2((char*)"assets/modelos/entrada2.obj");
	Model Entrada3((char*)"assets/modelos/entrada3.obj");
	Model Entrada4((char*)"assets/modelos/entrada4.obj");
	Model FlorCamino1((char*)"assets/modelos/florCamino1.obj");
	Model FlorCamino2((char*)"assets/modelos/florCamino2.obj");
	Model FlorTumba((char*)"assets/modelos/florTumba.obj");
	Model FlorTumba2((char*)"assets/modelos/florTumba2.obj");
	Model Pasillo1((char*)"assets/modelos/pasillo1.obj");

	// Cuadro informativo - Camara 1
	GLuint infoTexture = SOIL_load_OGL_texture("assets/texturas/info_camara1.png", SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
	GLuint hintInfoTexture = SOIL_load_OGL_texture("assets/texturas/hint_info.png", SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
	GLuint hintFreeTexture = SOIL_load_OGL_texture("assets/texturas/hint_free.png", SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
	GLuint hintAnimationTexture = SOIL_load_OGL_texture("assets/texturas/hint_animation.png", SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
	GLuint infoTexture2 = SOIL_load_OGL_texture("assets/texturas/info_camara2.png", SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
	GLuint hintInfoTexture2 = SOIL_load_OGL_texture("assets/texturas/hint_info_cam2.png", SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
	GLuint infoTexture3 = SOIL_load_OGL_texture("assets/texturas/info_camara3.png", SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
	GLuint hintInfoTexture3 = SOIL_load_OGL_texture("assets/texturas/hint_info_cam3.png", SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
	GLuint infoTexture4 = SOIL_load_OGL_texture("assets/texturas/info_camara4.png", SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
	GLuint hintInfoTexture4 = SOIL_load_OGL_texture("assets/texturas/hint_info_cam4.png", SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
	GLuint hintAnimationTexture4 = SOIL_load_OGL_texture("assets/texturas/hint_animation_cam4.png", SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);

	if (infoTexture == 0)
		std::cout << "ERROR: No se pudo cargar info_camara1.png" << std::endl;
	if (hintInfoTexture == 0)
		std::cout << "ERROR: No se pudo cargar hint_info.png" << std::endl;
	if (hintFreeTexture == 0)
		std::cout << "ERROR: No se pudo cargar hint_free.png" << std::endl;
	if (hintAnimationTexture == 0)
		std::cout << "ERROR: No se pudo cargar hint_animation.png" << std::endl;
	if (infoTexture2 == 0)
		std::cout << "ERROR: No se pudo cargar info_camara2.png" << std::endl;
	if (hintInfoTexture2 == 0)
		std::cout << "ERROR: No se pudo cargar hint_info_cam2.png" << std::endl;
	if (infoTexture3 == 0)
		std::cout << "ERROR: No se pudo cargar info_camara3.png" << std::endl;
	if (hintInfoTexture3 == 0)
		std::cout << "ERROR: No se pudo cargar hint_info_cam3.png" << std::endl;
	if (infoTexture4 == 0)
		std::cout << "ERROR: No se pudo cargar info_camara4.png" << std::endl;
	if (hintInfoTexture4 == 0)
		std::cout << "ERROR: No se pudo cargar hint_info_cam4.png" << std::endl;
	if (hintAnimationTexture4 == 0)
		std::cout << "ERROR: No se pudo cargar hint_animation_cam4.png" << std::endl;

	glBindTexture(GL_TEXTURE_2D, infoTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	float overlayVertices[] = {
		// Posicion          // Textura
		-0.90f, -0.90f,     0.0f, 0.0f,
		 0.90f, -0.90f,     1.0f, 0.0f,
		 0.90f, -0.25f,     1.0f, 1.0f,
		-0.90f, -0.90f,     0.0f, 0.0f,
		 0.90f, -0.25f,     1.0f, 1.0f,
		-0.90f, -0.25f,     0.0f, 1.0f
	};

	GLuint overlayVAO, overlayVBO;
	glGenVertexArrays(1, &overlayVAO);
	glGenBuffers(1, &overlayVBO);
	glBindVertexArray(overlayVAO);
	glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(overlayVertices), overlayVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	// Aviso pequeño: informacion cultural
	float hintInfoVertices[] = {
		 0.07f, -0.94f,   0.0f, 0.0f,
		 0.94f, -0.94f,   1.0f, 0.0f,
		 0.94f, -0.61f,   1.0f, 1.0f,
		 0.07f, -0.94f,   0.0f, 0.0f,
		 0.94f, -0.61f,   1.0f, 1.0f,
		 0.07f, -0.61f,   0.0f, 1.0f
	};

	GLuint hintInfoVAO, hintInfoVBO;
	glGenVertexArrays(1, &hintInfoVAO);
	glGenBuffers(1, &hintInfoVBO);
	glBindVertexArray(hintInfoVAO);
	glBindBuffer(GL_ARRAY_BUFFER, hintInfoVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(hintInfoVertices), hintInfoVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	// Aviso pequeño: exploracion libre
	float hintFreeVertices[] = {
		-0.30f, -0.94f,   0.0f, 0.0f,
		 0.94f, -0.94f,   1.0f, 0.0f,
		 0.94f, -0.64f,   1.0f, 1.0f,
		-0.30f, -0.94f,   0.0f, 0.0f,
		 0.94f, -0.64f,   1.0f, 1.0f,
		-0.30f, -0.64f,   0.0f, 1.0f
	};

	GLuint hintFreeVAO, hintFreeVBO;
	glGenVertexArrays(1, &hintFreeVAO);
	glGenBuffers(1, &hintFreeVBO);
	glBindVertexArray(hintFreeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, hintFreeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(hintFreeVertices), hintFreeVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	// Aviso: iniciar animacion de la barca
	float hintAnimationVertices[] = {
		-0.94f, -0.94f,   0.0f, 0.0f,
		-0.07f, -0.94f,   1.0f, 0.0f,
		-0.07f, -0.61f,   1.0f, 1.0f,
		-0.94f, -0.94f,   0.0f, 0.0f,
		-0.07f, -0.61f,   1.0f, 1.0f,
		-0.94f, -0.61f,   0.0f, 1.0f
	};

	GLuint hintAnimationVAO, hintAnimationVBO;
	glGenVertexArrays(1, &hintAnimationVAO);
	glGenBuffers(1, &hintAnimationVBO);
	glBindVertexArray(hintAnimationVAO);
	glBindBuffer(GL_ARRAY_BUFFER, hintAnimationVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(hintAnimationVertices), hintAnimationVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	overlayShader.Use();
	glUniform1i(glGetUniformLocation(overlayShader.Program, "overlayTexture"), 0);

	lightingShader.Use();
	glUniform1i(glGetUniformLocation(lightingShader.Program, "Material.difuse"), 0);
	glUniform1i(glGetUniformLocation(lightingShader.Program, "Material.specular"), 1);
	glm::mat4 projection = glm::perspective(camera.GetZoom(), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 100.0f);

	// 0. De pie
	KeyFrame[0] = { 0.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 0.0f,    // torsoPitch
					0.0f, 0.0f,    // headRot
					0.0f, 0.0f,    // leftArmRot
					0.0f, 0.0f,    // rightArmRot
					0.0f, 0.0f,    // leftLegRot
					0.0f, 0.0f };  // rightLegRot

	// 1. Manos juntas al frente, cabeza ligeramente baja
	KeyFrame[1] = KeyFrame[0];
	KeyFrame[1].leftArmRot = -15.0f;
	KeyFrame[1].rightArmRot = -15.0f;
	KeyFrame[1].headRot = 5.0f;

	// 2. Reverencia: torso inclinado 30 grados hacia adelante
	KeyFrame[2] = KeyFrame[1];
	KeyFrame[2].torsoPitch = 30.0f;
	KeyFrame[2].headRot = 10.0f;

	// 3. Regreso a la postura de manos juntas
	KeyFrame[3] = KeyFrame[1];

	// 4. Reposo
	KeyFrame[4] = KeyFrame[0];

	FrameIndex = 5;

	while (!glfwWindowShouldClose(window))
	{
		GLfloat currentFrame = (GLfloat)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
		DoMovement();
		Animation();

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		lightingShader.Use();

		// Datos de la camara activa (se calculan una sola vez por frame)
		glm::vec3 curPos;
		glm::vec3 curFront;
		glm::mat4 view;

		if (useFreeCamera) {
			curPos = camera.GetPosition();
			curFront = camera.GetFront();
			view = camera.GetViewMatrix();
		}
		else {
			curPos = cameraPositions[correntCamera];
			curFront = frontFromAngles(cameraYaw[correntCamera], cameraPitch[correntCamera]);
			view = glm::lookAt(curPos, curPos + curFront, glm::vec3(0.0f, 1.0f, 0.0f));
		}

		glUniform1i(glGetUniformLocation(lightingShader.Program, "diffuse"), 0);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "viewPos"), curPos.x, curPos.y, curPos.z);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.ambient"), 0.08f, 0.10f, 0.16f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.diffuse"), 0.18f, 0.22f, 0.32f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.specular"), 0.20f, 0.24f, 0.35f);

		float time = (float)glfwGetTime();

		float flicker0 = 0.70f + 0.25f * sin(time * 2.8f) + 0.12f * sin(time * 6.1f);
		float flicker1 = 0.72f + 0.23f * sin(time * 3.1f + 1.0f) + 0.12f * sin(time * 6.8f);
		float flicker2 = 0.69f + 0.26f * sin(time * 2.6f + 2.0f) + 0.11f * sin(time * 6.4f);
		float flicker3 = 0.71f + 0.24f * sin(time * 2.9f + 3.0f) + 0.12f * sin(time * 7.0f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].position"), -2.4264f, 1.6475f, 0.11014f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].ambient"), 0.05f * flicker0, 0.02f * flicker0, 0.005f * flicker0);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].diffuse"), 0.80f * flicker0, 0.30f * flicker0, 0.05f * flicker0);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].specular"), 0.90f, 0.40f, 0.10f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].linear"), 0.50f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].quadratic"), 0.75f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].position"), -2.4322f, 1.6560f, -0.8175f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].ambient"), 0.05f * flicker1, 0.02f * flicker1, 0.005f * flicker1);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].diffuse"), 0.80f * flicker1, 0.30f * flicker1, 0.05f * flicker1);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].specular"), 0.90f, 0.40f, 0.10f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].linear"), 0.50f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].quadratic"), 0.75f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].position"), -1.6344f, 1.5845f, -0.21061f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].ambient"), 0.05f * flicker2, 0.02f * flicker2, 0.005f * flicker2);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].diffuse"), 0.80f * flicker2, 0.30f * flicker2, 0.05f * flicker2);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].specular"), 0.90f, 0.40f, 0.10f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].linear"), 0.50f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].quadratic"), 0.75f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].position"), -1.0226f, 1.5204f, -1.0543f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].ambient"), 0.05f * flicker3, 0.02f * flicker3, 0.005f * flicker3);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].diffuse"), 0.80f * flicker3, 0.30f * flicker3, 0.05f * flicker3);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].specular"), 0.90f, 0.40f, 0.10f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].linear"), 0.50f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].quadratic"), 0.75f);

		float flicker4 = 0.82f + 0.15f * sin(time * 2.1f + 0.5f) + 0.05f * sin(time * 5.0f);
		float flicker5 = 0.84f + 0.14f * sin(time * 2.3f + 1.5f) + 0.05f * sin(time * 5.4f);
		float flicker6 = 0.83f + 0.15f * sin(time * 2.0f + 2.5f) + 0.05f * sin(time * 4.8f);
		float flicker7 = 0.84f + 0.14f * sin(time * 2.2f + 3.5f) + 0.05f * sin(time * 5.2f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[4].position"), 1.096f, 1.8039f, -0.45079f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[4].ambient"), 0.03f * flicker4, 0.02f * flicker4, 0.01f * flicker4);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[4].diffuse"), 0.52f * flicker4, 0.35f * flicker4, 0.14f * flicker4);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[4].specular"), 0.25f, 0.20f, 0.10f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[4].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[4].linear"), 0.18f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[4].quadratic"), 0.10f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[5].position"), 1.096f, 1.8039f, -2.5193f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[5].ambient"), 0.05f * flicker5, 0.03f * flicker5, 0.015f * flicker5);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[5].diffuse"), 0.60f * flicker5, 0.40f * flicker5, 0.16f * flicker5);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[5].specular"), 0.25f, 0.20f, 0.10f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[5].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[5].linear"), 0.18f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[5].quadratic"), 0.10f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[6].position"), 6.1152f, 1.8039f, -1.3698f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[6].ambient"), 0.03f * flicker6, 0.02f * flicker6, 0.01f * flicker6);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[6].diffuse"), 0.48f * flicker6, 0.32f * flicker6, 0.12f * flicker6);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[6].specular"), 0.25f, 0.20f, 0.10f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[6].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[6].linear"), 0.20f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[6].quadratic"), 0.12f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[7].position"), 6.1348f, 1.8039f, -3.5318f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[7].ambient"), 0.03f * flicker7, 0.02f * flicker7, 0.01f * flicker7);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[7].diffuse"), 0.46f * flicker7, 0.30f * flicker7, 0.12f * flicker7);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[7].specular"), 0.25f, 0.20f, 0.10f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[7].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[7].linear"), 0.20f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[7].quadratic"), 0.12f);

		// ---------------------------------------------------------
		// Luces del camino hacia el panteon
		// ---------------------------------------------------------

		float flicker8 = 0.72f + 0.22f * sin(time * 2.7f + 0.8f) + 0.10f * sin(time * 6.0f);
		float flicker9 = 0.74f + 0.20f * sin(time * 3.0f + 1.7f) + 0.10f * sin(time * 6.5f);
		float flicker10 = 0.71f + 0.23f * sin(time * 2.5f + 2.6f) + 0.10f * sin(time * 6.2f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[8].position"), 0.081311f, 1.7833f, -0.46429f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[8].ambient"), 0.08f * flicker8, 0.045f * flicker8, 0.015f * flicker8);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[8].diffuse"), 0.85f * flicker8, 0.50f * flicker8, 0.16f * flicker8);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[8].specular"), 0.35f, 0.20f, 0.08f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[8].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[8].linear"), 0.16f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[8].quadratic"), 0.08f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[9].position"), 1.2133f, 1.6803f, -0.46429f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[9].ambient"), 0.08f * flicker9, 0.045f * flicker9, 0.015f * flicker9);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[9].diffuse"), 0.85f * flicker9, 0.50f * flicker9, 0.16f * flicker9);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[9].specular"), 0.35f, 0.20f, 0.08f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[9].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[9].linear"), 0.16f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[9].quadratic"), 0.08f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[10].position"), 2.3211f, 1.6319f, -0.46429f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[10].ambient"), 0.08f * flicker10, 0.045f * flicker10, 0.015f * flicker10);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[10].diffuse"), 0.85f * flicker10, 0.50f * flicker10, 0.16f * flicker10);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[10].specular"), 0.35f, 0.20f, 0.08f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[10].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[10].linear"), 0.16f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[10].quadratic"), 0.08f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.position"), curPos.x, curPos.y, curPos.z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.direction"), curFront.x, curFront.y, curFront.z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.ambient"), 0.2f, 0.2f, 0.8f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.diffuse"), 0.2f, 0.2f, 0.8f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.linear"), 0.3f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.quadratic"), 0.7f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.cutOff"), glm::cos(glm::radians(12.0f)));
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.outerCutOff"), glm::cos(glm::radians(18.0f)));

		glUniform1f(glGetUniformLocation(lightingShader.Program, "material.shininess"), 5.0f);

		GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");
		GLint viewLoc = glGetUniformLocation(lightingShader.Program, "view");
		GLint projLoc = glGetUniformLocation(lightingShader.Program, "projection");
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

		glm::mat4 model(1);

		// Entorno
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Isla.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Casas.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Cementerio.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Panteon.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Iglesia.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Muelles.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); SK.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Velas.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Velas2.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Monumento.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Fuente.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Pasto.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Camino.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Altar1.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Entrada.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Entrada2.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Entrada3.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Entrada4.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); FlorCamino1.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); FlorCamino2.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); FlorTumba.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); FlorTumba2.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Pasillo1.Draw(lightingShader); 

		model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(rotacionPeces + 30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		Peces.Draw(lightingShader);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -0.7f, 0.0f));
		model = glm::rotate(model, glm::radians(-rotacionPeces + 30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		Peces1.Draw(lightingShader);

		model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(-rotacionPeces - 30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		Peces2.Draw(lightingShader);

		// ---------------------------------------------------------
		// Barca principal con remos animados
		// ---------------------------------------------------------

		glm::vec3 boatPosition = glm::mix(glm::vec3(19.0f, 0.5f, -3.8f), glm::vec3(9.5f, 0.5f, -1.9f), boatT);
		glm::vec3 boatDirection = glm::normalize(glm::vec3(9.5f, 0.5f, -1.9f) - glm::vec3(19.0f, 0.5f, -3.8f));

		boatPosition += glm::vec3(-boatDirection.z, 0.0f, boatDirection.x) * (sin(boatT * glm::pi<float>() * 4.0f) * 0.30f);

		// Matriz base: conserva exactamente el recorrido, giro y escala originales
		glm::mat4 boatBase = glm::mat4(1.0f);
		boatBase = glm::translate(boatBase, boatPosition);
		boatBase = glm::rotate(boatBase, glm::radians(225.0f + sin(boatT * glm::pi<float>() * 4.0f) * 4.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		boatBase = glm::scale(boatBase, glm::vec3(0.25f));

		// Cuerpo de la barca
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(boatBase));
		BodyBoat.Draw(lightingShader);

		// Movimiento de los remos
		// 4 ciclos durante todo el recorrido y vuelve a 0 al terminar
		float paddleAngle = sin(boatT * glm::pi<float>() * 10.0f) * 20.0f;

		// ---------------------------------------------------------
		// Remo izquierdo
		// Pivote calculado directamente desde boat.obj original
		// ---------------------------------------------------------

		glm::vec3 pivotLeft(0.68804f, 0.69021f, -0.97946f);

		glm::mat4 leftPaddleMatrix = boatBase;
		leftPaddleMatrix = glm::translate(leftPaddleMatrix, pivotLeft);
		leftPaddleMatrix = glm::rotate(leftPaddleMatrix, glm::radians(paddleAngle), glm::vec3(1.0f, 0.0f, 0.0f));
		leftPaddleMatrix = glm::translate(leftPaddleMatrix, -pivotLeft);

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(leftPaddleMatrix));
		PaddleLeft.Draw(lightingShader);

		// ---------------------------------------------------------
		// Remo derecho
		// Pivote calculado directamente desde boat.obj original
		// ---------------------------------------------------------

		glm::vec3 pivotRight(-0.85048f, 0.27549f, 0.55167f);

		glm::mat4 rightPaddleMatrix = boatBase;
		rightPaddleMatrix = glm::translate(rightPaddleMatrix, pivotRight);
		rightPaddleMatrix = glm::rotate(rightPaddleMatrix, glm::radians(-paddleAngle), glm::vec3(1.0f, 0.0f, 0.0f));
		rightPaddleMatrix = glm::translate(rightPaddleMatrix, -pivotRight);

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rightPaddleMatrix));
		PaddleRight.Draw(lightingShader);

		// ---------------------------------------------------------
		// Barca secundaria 1
		// ---------------------------------------------------------

		float bgAngleRad = glm::radians(-bgBoat1Angle);

		glm::mat4 bgBoatBase1 = glm::mat4(1.0f);
		bgBoatBase1 = glm::translate(bgBoatBase1, glm::vec3(
			cos(bgAngleRad) * 18.0f,
			0.5f,
			sin(bgAngleRad) * 12.0f
		));

		bgBoatBase1 = glm::rotate(
			bgBoatBase1,
			glm::radians(
				glm::degrees(
					atan2(
						-sin(bgAngleRad) * 18.0f,
						cos(bgAngleRad) * 12.0f
					)
				) - 56.3f + 180.0f
			),
			glm::vec3(0.0f, 1.0f, 0.0f)
		);

		bgBoatBase1 = glm::scale(bgBoatBase1, glm::vec3(0.25f));

		// Cuerpo
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bgBoatBase1));
		BodyBoat.Draw(lightingShader);

		// Movimiento de remos
		float bgPaddleAngle1 =
			sin(glm::radians(bgBoat1Angle * 5.0f)) * 20.0f;

		// Remo izquierdo
		glm::vec3 bgPivotLeft1(0.68804f, 0.69021f, -0.97946f);

		glm::mat4 bgLeftPaddle1 = bgBoatBase1;
		bgLeftPaddle1 = glm::translate(bgLeftPaddle1, bgPivotLeft1);
		bgLeftPaddle1 = glm::rotate(
			bgLeftPaddle1,
			glm::radians(bgPaddleAngle1),
			glm::vec3(1.0f, 0.0f, 0.0f)
		);
		bgLeftPaddle1 = glm::translate(bgLeftPaddle1, -bgPivotLeft1);

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bgLeftPaddle1));
		PaddleLeft.Draw(lightingShader);

		// Remo derecho
		glm::vec3 bgPivotRight1(-0.85048f, 0.27549f, 0.55167f);

		glm::mat4 bgRightPaddle1 = bgBoatBase1;
		bgRightPaddle1 = glm::translate(bgRightPaddle1, bgPivotRight1);
		bgRightPaddle1 = glm::rotate(
			bgRightPaddle1,
			glm::radians(-bgPaddleAngle1),
			glm::vec3(1.0f, 0.0f, 0.0f)
		);
		bgRightPaddle1 = glm::translate(bgRightPaddle1, -bgPivotRight1);

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bgRightPaddle1));
		PaddleRight.Draw(lightingShader);


		// ---------------------------------------------------------
		// Barca secundaria 2
		// ---------------------------------------------------------

		float bgAngleRad2 = glm::radians(bgBoat2Angle);

		glm::mat4 bgBoatBase2 = glm::mat4(1.0f);
		bgBoatBase2 = glm::translate(bgBoatBase2, glm::vec3(
			cos(bgAngleRad2) * 15.0f,
			0.5f,
			sin(bgAngleRad2) * 10.0f
		));

		bgBoatBase2 = glm::rotate(
			bgBoatBase2,
			glm::radians(
				glm::degrees(
					atan2(
						sin(bgAngleRad2) * 15.0f,
						-cos(bgAngleRad2) * 10.0f
					)
				) - 56.3f + 180.0f
			),
			glm::vec3(0.0f, 1.0f, 0.0f)
		);

		bgBoatBase2 = glm::scale(bgBoatBase2, glm::vec3(0.25f));

		// Cuerpo
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bgBoatBase2));
		BodyBoat.Draw(lightingShader);

		// Movimiento de remos
		float bgPaddleAngle2 =
			sin(glm::radians(bgBoat2Angle * 5.0f)) * 20.0f;

		// Remo izquierdo
		glm::vec3 bgPivotLeft2(0.68804f, 0.69021f, -0.97946f);

		glm::mat4 bgLeftPaddle2 = bgBoatBase2;
		bgLeftPaddle2 = glm::translate(bgLeftPaddle2, bgPivotLeft2);
		bgLeftPaddle2 = glm::rotate(
			bgLeftPaddle2,
			glm::radians(bgPaddleAngle2),
			glm::vec3(1.0f, 0.0f, 0.0f)
		);
		bgLeftPaddle2 = glm::translate(bgLeftPaddle2, -bgPivotLeft2);

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bgLeftPaddle2));
		PaddleLeft.Draw(lightingShader);

		// Remo derecho
		glm::vec3 bgPivotRight2(-0.85048f, 0.27549f, 0.55167f);

		glm::mat4 bgRightPaddle2 = bgBoatBase2;
		bgRightPaddle2 = glm::translate(bgRightPaddle2, bgPivotRight2);
		bgRightPaddle2 = glm::rotate(
			bgRightPaddle2,
			glm::radians(-bgPaddleAngle2),
			glm::vec3(1.0f, 0.0f, 0.0f)
		);
		bgRightPaddle2 = glm::translate(bgRightPaddle2, -bgPivotRight2);

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bgRightPaddle2));
		PaddleRight.Draw(lightingShader);

		glm::mat4 baseMatrix = glm::mat4(1.0f);
		baseMatrix = glm::translate(baseMatrix, glm::vec3(charPosX, charPosY, charPosZ));
		baseMatrix = glm::rotate(baseMatrix, glm::radians(charRotY), glm::vec3(0.0f, 1.0f, 0.0f));

		// TORSO: pivota en la cadera
		glm::mat4 torsoMatrix = rotAround(baseMatrix, pivotHip, torsoPitch);
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(torsoMatrix));
		Torso.Draw(lightingShader);

		// CABEZA: hija del torso, pivota en el cuello
		glm::mat4 headMatrix = rotAround(torsoMatrix, pivotNeck, headRot);
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(headMatrix));
		Head.Draw(lightingShader);

		// BRAZO IZQUIERDO: hijo del torso, pivota en el hombro izquierdo
		glm::mat4 lArmMatrix = rotAround(torsoMatrix, pivotLShoulder, leftArmRot);
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lArmMatrix));
		LeftArm.Draw(lightingShader);

		// BRAZO DERECHO: hijo del torso, pivota en el hombro derecho
		glm::mat4 rArmMatrix = rotAround(torsoMatrix, pivotRShoulder, rightArmRot);
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rArmMatrix));
		RightArm.Draw(lightingShader);

		// PIERNAS: no heredan la inclinacion del torso, pivotan en la cadera
		glm::mat4 lLegMatrix = rotAround(baseMatrix, pivotLHip, leftLegRot);
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lLegMatrix));
		LeftLeg.Draw(lightingShader);

		glm::mat4 rLegMatrix = rotAround(baseMatrix, pivotRHip, rightLegRot);
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rLegMatrix));
		RightLeg.Draw(lightingShader);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		overlayShader.Use();
		glActiveTexture(GL_TEXTURE0);

		if (useFreeCamera) {
			glBindTexture(GL_TEXTURE_2D, hintFreeTexture);
			glBindVertexArray(hintFreeVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(0);
		}
		else if (!showInfoPanel) {

			// Aviso de informacion segun la camara
			if (correntCamera == 0)
				glBindTexture(GL_TEXTURE_2D, hintInfoTexture);
			else if (correntCamera == 1)
				glBindTexture(GL_TEXTURE_2D, hintInfoTexture2);
			else if (correntCamera == 2)
				glBindTexture(GL_TEXTURE_2D, hintInfoTexture3);
			else if (correntCamera == 3)
				glBindTexture(GL_TEXTURE_2D, hintInfoTexture4);

			glBindVertexArray(hintInfoVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(0);

			// Camara 1: animacion de la barca
			if (correntCamera == 0) {
				glBindTexture(GL_TEXTURE_2D, hintAnimationTexture);
				glBindVertexArray(hintAnimationVAO);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				glBindVertexArray(0);
			}

			// Camara 4: animacion del personaje
			else if (correntCamera == 3) {
				glBindTexture(GL_TEXTURE_2D, hintAnimationTexture4);
				glBindVertexArray(hintAnimationVAO);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				glBindVertexArray(0);
			}
		}
		else {

			// Ficha cultural segun la camara
			if (correntCamera == 0)
				glBindTexture(GL_TEXTURE_2D, infoTexture);
			else if (correntCamera == 1)
				glBindTexture(GL_TEXTURE_2D, infoTexture2);
			else if (correntCamera == 2)
				glBindTexture(GL_TEXTURE_2D, infoTexture3);
			else if (correntCamera == 3)
				glBindTexture(GL_TEXTURE_2D, infoTexture4);

			glBindVertexArray(overlayVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(0);
		}

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glfwSwapBuffers(window);
	}
	glfwTerminate();
	return 0;
}

void DoMovement()
{
	// WASD solo mueve la camara original (libre)
	if (useFreeCamera) {
		if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP]) camera.ProcessKeyboard(FORWARD, deltaTime);
		if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN]) camera.ProcessKeyboard(BACKWARD, deltaTime);
		if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT]) camera.ProcessKeyboard(LEFT, deltaTime);
		if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) camera.ProcessKeyboard(RIGHT, deltaTime);
	}

	// Ajuste manual de la pose (grados por frame)
	if (keys[GLFW_KEY_2]) leftArmRot -= 0.5f;
	if (keys[GLFW_KEY_3]) rightArmRot -= 0.5f;
	if (keys[GLFW_KEY_4]) rightArmRot += 0.5f;
	if (keys[GLFW_KEY_5]) torsoPitch += 0.5f;
	if (keys[GLFW_KEY_6]) torsoPitch -= 0.5f;
	if (keys[GLFW_KEY_I]) charPosZ -= 0.005f;
	if (keys[GLFW_KEY_K]) charPosZ += 0.005f;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) glfwSetWindowShouldClose(window, GL_TRUE);
	if (key >= 0 && key < 1024) { if (action == GLFW_PRESS) keys[key] = true; else if (action == GLFW_RELEASE) keys[key] = false; }
	if (key == GLFW_KEY_M && action == GLFW_PRESS) { if (!boatMoving) { boatT = 0.0f; boatMoving = true; } }

	// Tecla F: guardar keyframe con la pose actual
	if (key == GLFW_KEY_F && action == GLFW_PRESS) saveFrame();

	// Tecla L: reproducir la reverencia
	if ((key == GLFW_KEY_L) && action == GLFW_PRESS) {
		if (play == false && (FrameIndex > 1)) {
			resetElements();
			interpolation();
			play = true;
			playIndex = 0;
			i_curr_steps = 0;
		}
		else play = false;
	}

	// Teclado numerico: 0 = camara libre, 1 a 4 = camaras fijas
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_0) { useFreeCamera = true; showInfoPanel = false; }
		if (key == GLFW_KEY_1) { useFreeCamera = false; correntCamera = 0; showInfoPanel = false; }
		if (key == GLFW_KEY_2) { useFreeCamera = false; correntCamera = 1; showInfoPanel = false; }
		if (key == GLFW_KEY_3) { useFreeCamera = false; correntCamera = 2; showInfoPanel = false; }
		if (key == GLFW_KEY_4) { useFreeCamera = false; correntCamera = 3; showInfoPanel = false; }
	}

	// Tecla E: abrir/cerrar informacion cultural de las Camaras 1, 2, 3 y 4
	if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		if (!useFreeCamera && (correntCamera == 0 || correntCamera == 1 || correntCamera == 2 || correntCamera == 3))
			showInfoPanel = !showInfoPanel;
	}
}

void Animation() {
	rotacionPeces += 120.0f * deltaTime; if (rotacionPeces >= 360.0f) rotacionPeces -= 360.0f;

	if (boatMoving) { boatT += 0.08f * deltaTime; if (boatT >= 1.0f) { boatT = 1.0f; boatMoving = false; } }
	bgBoat1Angle += 12.0f * deltaTime; if (bgBoat1Angle >= 360.0f) bgBoat1Angle -= 360.0f;
	bgBoat2Angle += 10.0f * deltaTime;
	if (bgBoat2Angle >= 360.0f) bgBoat2Angle -= 360.0f;

	// Motor de interpolacion por incrementos
	if (play) {
		if (i_curr_steps >= i_max_steps) {
			playIndex++;
			if (playIndex > FrameIndex - 2) {
				playIndex = 0;
				play = false;
				printf("Animacion terminada.\n");
			}
			else {
				i_curr_steps = 0;
				interpolation();
			}
		}
		else {
			charPosX += KeyFrame[playIndex].incX;
			charPosY += KeyFrame[playIndex].incY;
			charPosZ += KeyFrame[playIndex].incZ;
			charRotY += KeyFrame[playIndex].charRotYInc;
			torsoPitch += KeyFrame[playIndex].torsoPitchInc;
			headRot += KeyFrame[playIndex].headRotInc;
			leftArmRot += KeyFrame[playIndex].leftArmRotInc;
			rightArmRot += KeyFrame[playIndex].rightArmRotInc;
			leftLegRot += KeyFrame[playIndex].leftLegRotInc;
			rightLegRot += KeyFrame[playIndex].rightLegRotInc;

			i_curr_steps++;
		}
	}
}

void MouseCallback(GLFWwindow* window, double xPos, double yPos) {
	if (firstMouse) { lastX = (GLfloat)xPos; lastY = (GLfloat)yPos; firstMouse = false; }
	GLfloat xOffset = (GLfloat)(xPos - lastX);
	GLfloat yOffset = (GLfloat)(lastY - yPos);
	lastX = (GLfloat)xPos;
	lastY = (GLfloat)yPos;

	if (useFreeCamera) {
		// Camara libre: el mouse usa su propia logica
		camera.ProcessMouseMovement(xOffset, yOffset);
	}
	else {
		// Camaras fijas: el mouse solo cambia el angulo de mira de la camara activa
		cameraYaw[correntCamera] += xOffset * cameraSensitivity;
		cameraPitch[correntCamera] += yOffset * cameraSensitivity;

		// Evita dar la vuelta completa por arriba o por abajo
		if (cameraPitch[correntCamera] > 89.0f) cameraPitch[correntCamera] = 89.0f;
		if (cameraPitch[correntCamera] < -89.0f) cameraPitch[correntCamera] = -89.0f;
	}
}
