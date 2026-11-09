#include <iostream>
#include <cmath>

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
// Funciones Keyframes
void saveFrame(void);
void resetElements(void);
void interpolation(void);

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Camera
Camera camera(glm::vec3(21.292f, 1.498f, -2.335f));
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;

float rotacionPeces = 0.0f;
float boatT = 0.0f;
bool boatMoving = false;
float bgBoat1Angle = 0.0f;

// -----------------------------------------------------
// --- VARIABLES KEYFRAMES DEL PERSONAJE ---
// -----------------------------------------------------
float charPosX = 0.0f, charPosY = 0.0f, charPosZ = 0.0f;
float charRotY = 0.0f;

float torsoPitch = 0.0f;
float headRot = 0.0f;
float leftArmRot = 0.0f, rightArmRot = 0.0f;
float leftLegRot = 0.0f, rightLegRot = 0.0f;

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

	Shader lightingShader("assets/shader/lighting.vs", "assets/shader/lighting.frag");

	Model Isla((char*)"assets/modelos/Isla.obj");
	Model Casas((char*)"assets/modelos/casas.obj");
	Model Cementerio((char*)"assets/modelos/cementerio.obj");
	Model Panteon((char*)"assets/modelos/panteonM.obj");
	Model EntradaPanteon((char*)"assets/modelos/EntradaPanteon.obj");
	Model Iglesia((char*)"assets/modelos/iglesia.obj");
	Model Muelles((char*)"assets/modelos/muelles.obj");
	Model SK((char*)"assets/modelos/SK.obj");
	Model Boat((char*)"assets/modelos/boat.obj");
	Model Velas((char*)"assets/modelos/velas.obj");
	Model Fuente((char*)"assets/modelos/fuente.obj");
	Model Pasto((char*)"assets/modelos/pasto.obj");
	Model Peces((char*)"assets/modelos/peces.obj");
	Model Monumento((char*)"assets/modelos/monumento.obj");
	Model Head((char*)"assets/modelos/head.obj");
	Model Torso((char*)"assets/modelos/torso.obj");
	Model LeftArm((char*)"assets/modelos/left_Arm.obj");
	Model RightArm((char*)"assets/modelos/right_Arm.obj");
	Model LeftLeg((char*)"assets/modelos/left_Leg.obj");
	Model RightLeg((char*)"assets/modelos/right_Leg.obj");

	lightingShader.Use();
	glUniform1i(glGetUniformLocation(lightingShader.Program, "Material.difuse"), 0);
	glUniform1i(glGetUniformLocation(lightingShader.Program, "Material.specular"), 1);
	glm::mat4 projection = glm::perspective(camera.GetZoom(), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 100.0f);

	// -------------------------------------------------------------------------
	// --- ANIMACIÓN PREDEFINIDA: REVERENCIA / ORACIÓN ---
	// -------------------------------------------------------------------------

	// 1. Postura inicial (De pie, brazos a los lados)
	KeyFrame[0] = { charPosX, charPosY, charPosZ, charRotY, 0,0,0,0,
					0.0f, 0,   // torsoPitch
					0.0f, 0,   // headRot
					0.0f, 0,   // leftArmRot
					0.0f, 0,   // rightArmRot
					0.0f, 0, 0.0f, 0 }; // Piernas

	// 2. Manos juntas (Levantar brazos ligeramente y agachar un poco la cabeza)
	KeyFrame[1] = KeyFrame[0];
	KeyFrame[1].leftArmRot = -0.05f;  // Rota brazo izquierdo hacia adelante
	KeyFrame[1].rightArmRot = -0.05f; // Rota brazo derecho hacia adelante
	KeyFrame[1].headRot = 0.5f;      // Inclina la cabeza hacia abajo

	// 3. Inclinación (El torso se inclina hacia adelante, los brazos mantienen su ángulo relativo)
	KeyFrame[2] = KeyFrame[1];
	KeyFrame[2].torsoPitch = 0.35f;   // Inclina el torso hacia el frente
	KeyFrame[2].headRot = 0.25f;      // Baja un poco más la cabeza

	// 4. Vuelve a la postura (El torso se levanta, las manos siguen juntas)
	KeyFrame[3] = KeyFrame[1];        // Copiamos el frame 2 exacto

	// 5. Regreso total al reposo (Opcional para que quede listo para otra animación)
	KeyFrame[4] = KeyFrame[0];

	// Le indicamos al sistema que ya hay 5 frames grabados en memoria
	FrameIndex = 5;

	while (!glfwWindowShouldClose(window))
	{
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame; lastFrame = currentFrame;

		glfwPollEvents();
		DoMovement();
		Animation();

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		lightingShader.Use();
		glUniform1i(glGetUniformLocation(lightingShader.Program, "diffuse"), 0);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "viewPos"), camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.ambient"),0.08f, 0.10f, 0.16f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.diffuse"),0.18f, 0.22f, 0.32f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.specular"),0.20f, 0.24f, 0.35f);

		float time = (float)glfwGetTime();
		float flicker0 = 0.72f + 0.20f * sin(time * 1.7f) + 0.10f * sin(time * 4.3f);
		float flicker1 = 0.74f + 0.18f * sin(time * 1.9f + 1.0f) + 0.10f * sin(time * 5.1f);
		float flicker2 = 0.71f + 0.21f * sin(time * 1.5f + 2.0f) + 0.10f * sin(time * 4.7f);
		float flicker3 = 0.73f + 0.19f * sin(time * 1.8f + 3.0f) + 0.09f * sin(time * 5.4f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].position"), -2.4264f, 1.6475f, 0.11014f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].ambient"), 0.05f, 0.02f, 0.005f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].diffuse"), 0.80f * flicker0, 0.30f * flicker0, 0.05f * flicker0);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].specular"), 0.90f, 0.40f, 0.10f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].linear"), 0.50f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].quadratic"), 0.75f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].position"), -2.4322f, 1.6560f, -0.8175f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].ambient"), 0.05f, 0.02f, 0.005f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].diffuse"), 0.80f * flicker1, 0.30f * flicker1, 0.05f * flicker1);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].specular"), 0.90f, 0.40f, 0.10f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].linear"), 0.50f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].quadratic"), 0.75f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].position"), -1.6344f, 1.5845f, -0.21061f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].ambient"), 0.05f, 0.02f, 0.005f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].diffuse"), 0.80f * flicker2, 0.30f * flicker2, 0.05f * flicker2);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].specular"), 0.90f, 0.40f, 0.10f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].linear"), 0.50f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].quadratic"), 0.75f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].position"), -1.0226f, 1.5204f, -1.0543f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].ambient"), 0.05f, 0.02f, 0.005f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].diffuse"), 0.80f * flicker3, 0.30f * flicker3, 0.05f * flicker3);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].specular"), 0.90f, 0.40f, 0.10f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].linear"), 0.50f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].quadratic"), 0.75f);

		float flicker4 = 0.96f + 0.04f * sin(time * 1.3f + 0.5f);
		float flicker5 = 0.95f + 0.05f * sin(time * 1.1f + 1.5f);
		float flicker6 = 0.97f + 0.03f * sin(time * 1.4f + 2.5f);
		float flicker7 = 0.96f + 0.04f * sin(time * 1.2f + 3.5f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[4].position"), 1.096f, 1.8039f, -0.45079f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[4].ambient"), 0.03f, 0.02f, 0.01f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[4].diffuse"), 0.45f * flicker4, 0.30f * flicker4, 0.12f * flicker4);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[4].specular"), 0.25f, 0.20f, 0.10f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[4].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[4].linear"), 0.18f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[4].quadratic"), 0.10f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[5].position"), 3.8f, 1.8039f, -4.2f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[5].ambient"), 0.05f, 0.03f, 0.015f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[5].diffuse"), 0.60f * flicker5, 0.40f * flicker5, 0.16f * flicker5);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[5].specular"), 0.25f, 0.20f, 0.10f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[5].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[5].linear"), 0.18f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[5].quadratic"), 0.10f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[6].position"), 6.1152f, 1.8039f, -1.3698f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[6].ambient"), 0.03f, 0.02f, 0.01f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[6].diffuse"), 0.42f * flicker6, 0.28f * flicker6, 0.10f * flicker6);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[6].specular"), 0.25f, 0.20f, 0.10f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[6].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[6].linear"), 0.20f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[6].quadratic"), 0.12f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[7].position"), 6.1348f, 1.8039f, -3.5318f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[7].ambient"), 0.03f, 0.02f, 0.01f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[7].diffuse"), 0.40f * flicker7, 0.26f * flicker7, 0.10f * flicker7);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[7].specular"), 0.25f, 0.20f, 0.10f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[7].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[7].linear"), 0.20f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[7].quadratic"), 0.12f);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.position"), camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.direction"), camera.GetFront().x, camera.GetFront().y, camera.GetFront().z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.ambient"), 0.2f, 0.2f, 0.8f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.diffuse"), 0.2f, 0.2f, 0.8f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.linear"), 0.3f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.quadratic"), 0.7f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.cutOff"), glm::cos(glm::radians(12.0f)));
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.outerCutOff"), glm::cos(glm::radians(18.0f)));

		glUniform1f(glGetUniformLocation(lightingShader.Program, "material.shininess"), 5.0f);

		glm::mat4 view = camera.GetViewMatrix();
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
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); EntradaPanteon.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Iglesia.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Muelles.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); SK.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Velas.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Monumento.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Fuente.Draw(lightingShader);
		model = glm::mat4(1.0f); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Pasto.Draw(lightingShader);

		model = glm::mat4(1.0f); model = glm::rotate(model, glm::radians(rotacionPeces), glm::vec3(1.0f, 0.0f, 0.0f)); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Peces.Draw(lightingShader);

		glm::vec3 boatPosition = glm::mix(glm::vec3(19.0f, 0.5f, -3.8f), glm::vec3(9.5f, 0.5f, -1.9f), boatT);
		glm::vec3 boatDirection = glm::normalize(glm::vec3(9.5f, 0.5f, -1.9f) - glm::vec3(19.0f, 0.5f, -3.8f));
		boatPosition += glm::vec3(-boatDirection.z, 0.0f, boatDirection.x) * (sin(boatT * glm::pi<float>() * 4.0f) * 0.30f);
		model = glm::mat4(1.0f); model = glm::translate(model, boatPosition); model = glm::rotate(model, glm::radians(225.0f + sin(boatT * glm::pi<float>() * 4.0f) * 4.0f), glm::vec3(0.0f, 1.0f, 0.0f)); model = glm::scale(model, glm::vec3(0.25f)); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Boat.Draw(lightingShader);

		float bgAngleRad = glm::radians(-bgBoat1Angle);
		model = glm::mat4(1.0f); model = glm::translate(model, glm::vec3(cos(bgAngleRad) * 18.0f, 0.5f, sin(bgAngleRad) * 12.0f)); model = glm::rotate(model, glm::radians(glm::degrees(atan2(-sin(bgAngleRad) * 18.0f, cos(bgAngleRad) * 12.0f)) - 56.3f + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); model = glm::scale(model, glm::vec3(0.25f)); glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); Boat.Draw(lightingShader);

		// -------------------------------------------------------------------------
		glm::vec3 pivotTorso(0.0f, 1.0f, 0.0f);
		glm::vec3 pivotHead(0.0f, 1.6f, 0.0f);
		glm::vec3 pivotLArm(0.3f, 1.4f, 0.0f);
		glm::vec3 pivotRArm(-0.3f, 1.4f, 0.0f);
		glm::vec3 pivotLLeg(0.15f, 1.0f, 0.0f);
		glm::vec3 pivotRLeg(-0.15f, 1.0f, 0.0f);

		// -------------------------------------------------------------------------
		// --- RENDERIZADO DEL PERSONAJE (POSICIÓN EXACTA DESDE BLENDER) ---
		// -------------------------------------------------------------------------

		// 1. Asignamos las coordenadas exactas de Blender a las variables globales de posición
		charPosX = -0.756f;
		charPosY = 0.08f; // Altura Z de Blender pasa a Y en OpenGL
		charPosZ = 0.0f; // Profundidad Y de Blender pasa a Z en OpenGL

		// 2. Matriz base del personaje en el mundo
		glm::mat4 baseMatrix = glm::mat4(1.0f);
		baseMatrix = glm::translate(baseMatrix, glm::vec3(charPosX, charPosY, charPosZ));
		baseMatrix = glm::rotate(baseMatrix, glm::radians(charRotY), glm::vec3(0.0f, 1.0f, 0.0f));

		// 3. TORSO (Incluye la inclinación de 60° para la reverencia)
		glm::mat4 torsoMatrix = baseMatrix;
		torsoMatrix = glm::rotate(torsoMatrix, glm::radians(torsoPitch), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(torsoMatrix));
		Torso.Draw(lightingShader);

		// 4. CABEZA (Hereda los movimientos del torso)
		glm::mat4 headMatrix = torsoMatrix;
		headMatrix = glm::rotate(headMatrix, glm::radians(headRot), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(headMatrix));
		Head.Draw(lightingShader);

		// 5. BRAZO IZQUIERDO
		glm::mat4 lArmMatrix = torsoMatrix;
		lArmMatrix = glm::rotate(lArmMatrix, glm::radians(leftArmRot), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lArmMatrix));
		LeftArm.Draw(lightingShader);

		// 6. BRAZO DERECHO
		glm::mat4 rArmMatrix = torsoMatrix;
		rArmMatrix = glm::rotate(rArmMatrix, glm::radians(rightArmRot), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rArmMatrix));
		RightArm.Draw(lightingShader);

		// 7. PIERNA IZQUIERDA
		glm::mat4 lLegMatrix = baseMatrix; // Las piernas rotan desde la base, no se inclinan con el torso
		lLegMatrix = glm::rotate(lLegMatrix, glm::radians(leftLegRot), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lLegMatrix));
		LeftLeg.Draw(lightingShader);

		// 8. PIERNA DERECHA
		glm::mat4 rLegMatrix = baseMatrix;
		rLegMatrix = glm::rotate(rLegMatrix, glm::radians(rightLegRot), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rLegMatrix));
		RightLeg.Draw(lightingShader);

		glfwSwapBuffers(window);
	}
	glfwTerminate(); return 0;
}

void DoMovement()
{
	if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP]) camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN]) camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT]) camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) camera.ProcessKeyboard(RIGHT, deltaTime);

	// Controles manuales restantes (Teclas 2, 3, 4, etc.)
	if (keys[GLFW_KEY_2]) leftArmRot -= 0.001f;
	if (keys[GLFW_KEY_3]) rightArmRot += 0.001f;
	if (keys[GLFW_KEY_4]) rightArmRot -= 0.001f;
	if (keys[GLFW_KEY_5]) torsoPitch += 0.001f;
	if (keys[GLFW_KEY_6]) torsoPitch -= 0.001f;
	if (keys[GLFW_KEY_I]) charPosZ -= 0.005f;
	if (keys[GLFW_KEY_K]) charPosZ += 0.005f;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) glfwSetWindowShouldClose(window, GL_TRUE);
	if (key >= 0 && key < 1024) { if (action == GLFW_PRESS) keys[key] = true; else if (action == GLFW_RELEASE) keys[key] = false; }
	if (key == GLFW_KEY_M && action == GLFW_PRESS) { if (!boatMoving) { boatT = 0.0f; boatMoving = true; } }

	// Tecla F: Guardar Keyframe (Opcional, si quieres crear nuevas animaciones)
	if (key == GLFW_KEY_F && action == GLFW_PRESS) saveFrame();

	// Tecla 1 o L: Reproducir la animación de Reverencia
	if ((key == GLFW_KEY_1 || key == GLFW_KEY_L) && action == GLFW_PRESS) {
		if (play == false && (FrameIndex > 1)) {
			resetElements();
			interpolation();
			play = true;
			playIndex = 0;
			i_curr_steps = 0;
		}
		else play = false;
	}
}

void Animation() {
	rotacionPeces += 90.0f * deltaTime; if (rotacionPeces >= 360.0f) rotacionPeces -= 360.0f;
	if (boatMoving) { boatT += 0.08f * deltaTime; if (boatT >= 1.0f) { boatT = 1.0f; boatMoving = false; } }
	bgBoat1Angle += 12.0f * deltaTime; if (bgBoat1Angle >= 360.0f) bgBoat1Angle -= 360.0f;

	// Motor de interpolación matemática para la animación del personaje
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
	if (firstMouse) { lastX = xPos; lastY = yPos; firstMouse = false; }
	GLfloat xOffset = xPos - lastX; GLfloat yOffset = lastY - yPos;
	lastX = xPos; lastY = yPos;
	camera.ProcessMouseMovement(xOffset, yOffset);
}