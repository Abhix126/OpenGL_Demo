#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <cmath>

// Shader sources
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 transform;
void main() { gl_Position = transform * vec4(aPos,1.0); }
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 color;
void main() { FragColor = vec4(color,1.0); }
)";

// Indexed sphere mesh
void generateIndexedSphere(float radius,int sectors,int stacks,
                           std::vector<float>& vertices,
                           std::vector<unsigned int>& indices){
    for(int i=0;i<=stacks;++i){
        float phi=M_PI*i/stacks;
        for(int j=0;j<=sectors;++j){
            float theta=2.0f*M_PI*j/sectors;
            float x=radius*sin(phi)*cos(theta);
            float y=radius*cos(phi);
            float z=radius*sin(phi)*sin(theta);
            vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
        }
    }
    for(int i=0;i<stacks;++i){
        for(int j=0;j<sectors;++j){
            int first=i*(sectors+1)+j;
            int second=first+sectors+1;
            indices.push_back(first); indices.push_back(second); indices.push_back(first+1);
            indices.push_back(second); indices.push_back(second+1); indices.push_back(first+1);
        }
    }
}

// Callback for window resizing
void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0,0,width,height);
}

int main(){
    // GLFW Init
    if(!glfwInit()) return -1;
    GLFWwindow* window=glfwCreateWindow(800,600,"Responsive Bouncing Sphere",NULL,NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cerr<<"GLAD initialization failed\n"; return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // Compile shaders
    unsigned int vShader=glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader,1,&vertexShaderSource,NULL); glCompileShader(vShader);
    unsigned int fShader=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader,1,&fragmentShaderSource,NULL); glCompileShader(fShader);
    unsigned int shaderProgram=glCreateProgram();
    glAttachShader(shaderProgram,vShader); glAttachShader(shaderProgram,fShader);
    glLinkProgram(shaderProgram);

    // Plane
    float planeVertices[]={-5,0,-5, 5,0,-5, 5,0,5, 5,0,5, -5,0,5, -5,0,-5};
    unsigned int planeVAO,planeVBO;
    glGenVertexArrays(1,&planeVAO); glGenBuffers(1,&planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER,planeVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(planeVertices),planeVertices,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0); glEnableVertexAttribArray(0);

    // Sphere
    std::vector<float> sphereVertices; std::vector<unsigned int> sphereIndices;
    generateIndexedSphere(0.5f,30,30,sphereVertices,sphereIndices);
    unsigned int sphereVAO,sphereVBO,sphereEBO;
    glGenVertexArrays(1,&sphereVAO); glGenBuffers(1,&sphereVBO); glGenBuffers(1,&sphereEBO);
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER,sphereVBO);
    glBufferData(GL_ARRAY_BUFFER,sphereVertices.size()*sizeof(float),&sphereVertices[0],GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sphereIndices.size()*sizeof(unsigned int),&sphereIndices[0],GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0); glEnableVertexAttribArray(0);

    // Physics control
    float groundY = 0.5f;          // ground plane
    float maxHeight = 3.0f;        // bounce height
    float gravity = -9.8f;         // gravity
    float speedFactor = 1.0f;      // animation speed
    float ballY = groundY;
    float velocity = std::sqrt(-2.0f * gravity * maxHeight);

    float lastFrame=0.0f, deltaTime=0.0f;

    while(!glfwWindowShouldClose(window)){
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Physics update
        velocity += gravity * deltaTime * speedFactor;
        ballY += velocity * deltaTime * speedFactor;
        if(ballY < groundY){
            ballY = groundY;
            velocity = std::sqrt(-2.0f * gravity * maxHeight);
        }

        // Get current window size for responsive projection
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                                (float)width/(float)height,
                                                0.1f, 100.0f);
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0,-1,-6));

        // Draw plane
        glUseProgram(shaderProgram);
        glm::mat4 planeTransform = projection*view*glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"transform"),1,GL_FALSE,glm::value_ptr(planeTransform));
        glUniform3f(glGetUniformLocation(shaderProgram,"color"),0.3f,0.8f,0.3f);
        glBindVertexArray(planeVAO); glDrawArrays(GL_TRIANGLES,0,6);

        // Draw sphere
        glm::mat4 sphereModel = glm::translate(glm::mat4(1.0f), glm::vec3(0,ballY,0));
        glm::mat4 sphereTransform = projection*view*sphereModel;
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"transform"),1,GL_FALSE,glm::value_ptr(sphereTransform));
        glUniform3f(glGetUniformLocation(shaderProgram,"color"),1.0f,0.2f,0.2f);
        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}