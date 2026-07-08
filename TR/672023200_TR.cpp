#define STB_IMAGE_IMPLEMENTATION
#include <GL/freeglut.h>
#include "stb_image.h"
#include <math.h>
#include <string.h>
#include <iostream>

// ==============================================================================
// KONFIGURASI KAMERA & STATE
// ==============================================================================

float camX = 0.0f, camY = 4.0f, camZ = 8.0f; 
float lookX = 0.0f, lookY = 4.0f, lookZ = 0.0f; 
float angleH = 0.0f, angleV = 0.0f; 
float fov = 60.0f; 

bool isMouseLeftDown = false;
int lastMouseX = 0, lastMouseY = 0;
bool keys[256]; 

bool isFanMoving = false;
float fanAngle = 0.0f;
bool isLightOn = false;
bool isDoorOpen = false;
float doorAngle = 0.0f; 

GLuint posterTexture;
bool isTextureLoaded = false;

void loadTexture() {
    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    
    // Gunakan nama file yang SAMA PERSIS dengan yang dites tadi (misal: "poster.jpg")
    unsigned char* data = stbi_load("test.jpg", &width, &height, &channels, 3);
    
    if (data) {
        isTextureLoaded = true;
        glEnable(GL_TEXTURE_2D);
        glGenTextures(1, &posterTexture);
        glBindTexture(GL_TEXTURE_2D, posterTexture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    } else {
        isTextureLoaded = false;
        std::cout << "Masih Gagal: " << stbi_failure_reason() << std::endl;
    }
}

void passiveMouseMotion(int x, int y) {
    int width = glutGet(GLUT_WINDOW_WIDTH);
    int height = glutGet(GLUT_WINDOW_HEIGHT);
    int centerX = width / 2;
    int centerY = height / 2;

    if (x == centerX && y == centerY) return;

    float deltaX = (float)(x - centerX);
    float deltaY = (float)(y - centerY);
    float sensitivity = 0.002f;

    angleH += deltaX * sensitivity; 
    angleV -= deltaY * sensitivity; 

    if (angleV > 1.48f) angleV = 1.48f;
    if (angleV < -1.48f) angleV = -1.48f;

    glutWarpPointer(centerX, centerY);
}

// ==============================================================================
// FUNGSI UTILITAS GEOMETRI & TEKS
// ==============================================================================
void updateCameraLook() {
    lookX = camX + sin(angleH) * cos(angleV);
    lookY = camY + sin(angleV);
    lookZ = camZ - cos(angleH) * cos(angleV);
}

void drawText3D(const char* text, float x, float y, float z, float scale, float rotY) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);
    glColor3f(1.0f, 1.0f, 1.0f); 
    glLineWidth(2.0f);
    for (int i = 0; i < strlen(text); i++) {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, text[i]);
    }
    glPopMatrix();
}

// ==============================================================================
// LINGKUNGAN & RUANGAN DASAR
// ==============================================================================
void drawEnvironment() {
    glColor3f(0.3f, 0.7f, 0.3f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-100.0f, -0.1f, 100.0f); 
    glVertex3f(100.0f, -0.1f, 100.0f);
    glVertex3f(100.0f, -0.1f, -100.0f);
    glVertex3f(-100.0f, -0.1f, -100.0f);
    glEnd();
}

void drawRoom() {
    // 1. LANTAI PROSEDURAL
    glDisable(GL_LIGHTING); 
    float tileSize = 2.0f; 
    for (float x = -10.0f; x < 10.0f; x += tileSize) {
        for (float z = -10.0f; z < 10.0f; z += tileSize) {
            if ((int)((x + 10.0f) / tileSize + (z + 10.0f) / tileSize) % 2 == 0) {
                glColor3f(0.45f, 0.3f, 0.2f); 
            } else {
                glColor3f(0.6f, 0.45f, 0.3f); 
            }
            glBegin(GL_QUADS);
            glNormal3f(0.0f, 1.0f, 0.0f);
            glVertex3f(x,            0.0f, z + tileSize);
            glVertex3f(x + tileSize, 0.0f, z + tileSize);
            glVertex3f(x + tileSize, 0.0f, z);
            glVertex3f(x,            0.0f, z);
            glEnd();
        }
    }
    if (isLightOn) glEnable(GL_LIGHTING); 

    // 2. ATAP
    glColor3f(0.9f, 0.9f, 0.9f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-10.0f, 10.0f, -10.0f); glVertex3f(10.0f, 10.0f, -10.0f);
    glVertex3f(10.0f, 10.0f, 10.0f); glVertex3f(-10.0f, 10.0f, 10.0f);
    glEnd();

    // 3. TEMBOK BELAKANG & KANAN
    glColor3f(0.95f, 0.95f, 0.95f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-10.0f, 0.0f, -10.0f); glVertex3f(10.0f, 0.0f, -10.0f);
    glVertex3f(10.0f, 10.0f, -10.0f); glVertex3f(-10.0f, 10.0f, -10.0f);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(10.0f, 0.0f, -10.0f); glVertex3f(10.0f, 0.0f, 10.0f);
    glVertex3f(10.0f, 10.0f, 10.0f); glVertex3f(10.0f, 10.0f, -10.0f);
    glEnd();

    // 4. TEMBOK KIRI (Dengan Jendela)
    glColor3f(0.92f, 0.92f, 0.92f);
    glBegin(GL_QUADS);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-10.0f, 0.0f, 10.0f); glVertex3f(-10.0f, 0.0f, -10.0f);
    glVertex3f(-10.0f, 3.0f, -10.0f); glVertex3f(-10.0f, 3.0f, 10.0f);
    glVertex3f(-10.0f, 7.0f, 10.0f); glVertex3f(-10.0f, 7.0f, -10.0f);
    glVertex3f(-10.0f, 10.0f, -10.0f); glVertex3f(-10.0f, 10.0f, 10.0f);
    glVertex3f(-10.0f, 3.0f, 10.0f); glVertex3f(-10.0f, 3.0f, 2.0f);
    glVertex3f(-10.0f, 7.0f, 2.0f); glVertex3f(-10.0f, 7.0f, 10.0f);
    glVertex3f(-10.0f, 3.0f, -4.0f); glVertex3f(-10.0f, 3.0f, -10.0f);
    glVertex3f(-10.0f, 7.0f, -10.0f); glVertex3f(-10.0f, 7.0f, -4.0f);
    glEnd();

    // 5. TEMBOK DEPAN (Dengan Pintu)
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-10.0f, 0.0f, 10.0f); glVertex3f(-2.0f, 0.0f, 10.0f);
    glVertex3f(-2.0f, 10.0f, 10.0f); glVertex3f(-10.0f, 10.0f, 10.0f);
    glVertex3f(2.0f, 0.0f, 10.0f); glVertex3f(10.0f, 0.0f, 10.0f);
    glVertex3f(10.0f, 10.0f, 10.0f); glVertex3f(2.0f, 10.0f, 10.0f);
    glVertex3f(-2.0f, 6.0f, 10.0f); glVertex3f(2.0f, 6.0f, 10.0f);
    glVertex3f(2.0f, 10.0f, 10.0f); glVertex3f(-2.0f, 10.0f, 10.0f);
    glEnd();
}

void drawPoster() {
    if (isTextureLoaded) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, posterTexture);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    } else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0f, 0.0f, 0.0f); 
    }
    
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f); 
    
    // PERBAIKAN KOORDINAT: Sumbu X dipersempit dari 2.5 menjadi 1.665 agar rasio sesuai foto
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.665f, 3.5f, -9.9f); // Kiri Bawah
    glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.665f, 3.5f, -9.9f); // Kanan Bawah
    glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.665f, 8.5f, -9.9f); // Kanan Atas
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.665f, 8.5f, -9.9f); // Kiri Atas
    glEnd();
    
    if (isTextureLoaded) {
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glDisable(GL_TEXTURE_2D);
    }
}

void drawCurtains() {
    glPushMatrix();
    glTranslatef(-9.8f, 7.0f, -4.0f); 
    glColor3f(0.4f, 0.4f, 0.5f); 
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 60; i++) {
        float zPos = i * 0.1f; 
        float xWave = sin(i * 0.5f) * 0.2f; 
        glNormal3f(1.0f, 0.0f, 0.0f);
        glVertex3f(xWave, 0.0f, zPos);      
        glVertex3f(xWave, -4.5f, zPos);     
    }
    glEnd();
    glPopMatrix();
}

void drawDoor() {
    glPushMatrix();
    glTranslatef(-2.0f, 0.0f, 10.0f); 
    glRotatef(doorAngle, 0.0f, 1.0f, 0.0f); 
    glColor3f(0.5f, 0.3f, 0.2f); 
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, -1.0f); 
    glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(4.0f, 0.0f, 0.0f);
    glVertex3f(4.0f, 6.0f, 0.0f); glVertex3f(0.0f, 6.0f, 0.0f);
    glEnd();
    glColor3f(0.8f, 0.8f, 0.8f);
    glPushMatrix();
    glTranslatef(3.5f, 3.0f, -0.1f);
    glutSolidSphere(0.15, 10, 10);
    glPopMatrix();
    glPopMatrix();
}

void drawFan() {
    glPushMatrix();
    glTranslatef(0.0f, 9.5f, 0.0f); 
    glColor3f(0.2f, 0.2f, 0.2f);
    glPushMatrix();
    glScalef(0.2f, 1.0f, 0.2f);
    glutSolidCube(1.0);
    glPopMatrix();
    glTranslatef(0.0f, -0.5f, 0.0f);
    glRotatef(fanAngle, 0.0f, 1.0f, 0.0f);
    glColor3f(0.4f, 0.4f, 0.4f);
    glutSolidSphere(0.4, 20, 20);
    glColor3f(0.9f, 0.9f, 0.9f);
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
        glRotatef(i * 90.0f, 0.0f, 1.0f, 0.0f);
        glTranslatef(1.5f, 0.0f, 0.0f);
        glScalef(2.5f, 0.1f, 0.5f);
        glutSolidCube(1.0);
        glPopMatrix();
    }
    glPopMatrix();
}

void drawBed() {
    glPushMatrix();
    glTranslatef(-6.0f, 0.0f, -6.0f);
    glColor3f(0.3f, 0.2f, 0.1f);
    glPushMatrix();
    glTranslatef(0.0f, 0.5f, 0.0f);
    glScalef(5.0f, 1.0f, 7.0f);
    glutSolidCube(1.0);
    glPopMatrix();
    glColor3f(0.9f, 0.9f, 0.9f); 
    glPushMatrix();
    glTranslatef(0.0f, 1.2f, 0.0f);
    glScalef(4.8f, 0.6f, 6.8f);
    glutSolidCube(1.0);
    glPopMatrix();
    glColor3f(0.8f, 0.8f, 0.85f);
    glPushMatrix();
    glTranslatef(-1.2f, 1.6f, -2.5f);
    glScalef(1.8f, 0.3f, 1.2f);
    glutSolidCube(1.0);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(1.2f, 1.6f, -2.5f);
    glScalef(1.8f, 0.3f, 1.2f);
    glutSolidCube(1.0);
    glPopMatrix();
    glColor3f(0.3f, 0.5f, 0.6f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-2.45f, 1.55f, -0.5f); glVertex3f(2.45f, 1.55f, -0.5f);
    glVertex3f(2.45f, 1.55f, 3.45f);  glVertex3f(-2.45f, 1.55f, 3.45f);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-2.45f, 1.55f, 3.45f); glVertex3f(2.45f, 1.55f, 3.45f);
    glVertex3f(2.45f, 0.5f, 3.45f);   glVertex3f(-2.45f, 0.5f, 3.45f);
    glEnd();
    glPopMatrix();
}

void drawDeskAndLaptop() {
    glPushMatrix();
    glTranslatef(6.0f, 0.0f, -7.0f);
    
    // 1. Meja Belajar
    glColor3f(0.2f, 0.2f, 0.2f); 
    glPushMatrix();
    glTranslatef(0.0f, 2.5f, 0.0f);
    glScalef(5.0f, 0.2f, 3.0f);
    glutSolidCube(1.0);
    glPopMatrix();
    
    glColor3f(0.6f, 0.6f, 0.6f);
    float legPos[4][2] = {{-2.3f, -1.3f}, {2.3f, -1.3f}, {-2.3f, 1.3f}, {2.3f, 1.3f}};
    for (int i=0; i<4; i++) {
        glPushMatrix();
        glTranslatef(legPos[i][0], 1.25f, legPos[i][1]);
        glScalef(0.2f, 2.5f, 0.2f);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    // 2. Base Laptop & Layar
    glPushMatrix();
    glTranslatef(0.0f, 2.65f, 0.0f);
    glRotatef(-15.0f, 0.0f, 1.0f, 0.0f); 
    
    // Body Bawah Laptop (Silver)
    glColor3f(0.75f, 0.75f, 0.75f); 
    glPushMatrix();
    glScalef(1.8f, 0.08f, 1.2f);
    glutSolidCube(1.0);
    glPopMatrix();
    
    // Area Tatakan Keyboard (Hitam)
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix();
    glTranslatef(0.0f, 0.05f, 0.1f);
    glScalef(1.6f, 0.05f, 0.8f);
    glutSolidCube(1.0);
    glPopMatrix();

    // -------------------------------------------------------------------------
    // TAMBAHAN: ORNAMEN GARIS-GARIS KEYBOARD (ANTI NO-CLIP TEXTURE)
    // -------------------------------------------------------------------------
    glDisable(GL_LIGHTING); // Matikan lighting sejenak agar garis tombol tajam konstan
    glColor3f(0.4f, 0.4f, 0.4f); // Warna garis abu-abu keyboard
    glLineWidth(1.5f);

    // Kita gambar grid di atas permukaan tatakan hitam (Y = 0.076f agar tidak z-fighting)
    // Area tatakan membentang dari X: -0.8 sampai 0.8, dan Z: -0.3 sampai 0.5
    glBegin(GL_LINES);
    
    // A. Garis-Garis Horizontal (Membuat Baris Tombol / Rows)
    // Membuat 5 baris tombol dari atas ke bawah
    for (float zRow = -0.25f; zRow <= 0.45f; zRow += 0.14f) {
        glVertex3f(-0.75f, 0.076f, zRow);
        glVertex3f( 0.75f, 0.076f, zRow);
    }

    // B. Garis-Garis Vertikal (Memotong Menjadi Kolom Tombol / Keys)
    // Membuat sekat tombol dari kiri ke kanan
    for (float xCol = -0.75f; xCol <= 0.75f; xCol += 0.12f) {
        glVertex3f(xCol, 0.076f, -0.25f);
        glVertex3f(xCol, 0.076f,  0.45f);
    }
    glEnd();
    if (isLightOn) glEnable(GL_LIGHTING);
    // -------------------------------------------------------------------------

    // 3. Monitor Laptop
    glTranslatef(0.0f, 0.5f, -0.6f);
    glRotatef(-20.0f, 1.0f, 0.0f, 0.0f); 
    
    // Frame Monitor (Silver)
    glColor3f(0.75f, 0.75f, 0.75f);
    glPushMatrix();
    glScalef(1.8f, 1.2f, 0.08f);
    glutSolidCube(1.0);
    glPopMatrix();
    
    // Layar Menyala (Cyan)
    glColor3f(0.1f, 0.8f, 0.9f); 
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.05f);
    glScalef(1.7f, 1.1f, 0.02f);
    glutSolidCube(1.0);
    glPopMatrix();
    
    glPopMatrix(); 
    glPopMatrix(); 
}

void drawSingleBook(float w, float h, float d, float r, float g, float b, const char* title) {
    float coverThickness = 0.02f; // Ketebalan kulit cover buku

    // -------------------------------------------------------------------------
    // A. GAMBAR ISI BUKU (KERTAS PUTIH MURNI - MENGHADAP DEPAN KAMERA)
    // -------------------------------------------------------------------------
    glColor3f(0.95f, 0.95f, 0.95f);
    glPushMatrix();
    // Ukuran kertas sedikit lebih kecil dari cover agar menjepit di dalam
    glScalef(w - (coverThickness * 2), h - 0.04f, d - coverThickness);
    glTranslatef(0.0f, 0.0f, coverThickness / 2.0f);
    glutSolidCube(1.0);
    glPopMatrix();

    // -------------------------------------------------------------------------
    // B. GAMBAR KULIT COVER BUKU (WARNA PROSEDURAL JEPERET KANAN-KIRI-BELAKANG)
    // -------------------------------------------------------------------------
    glColor3f(r, g, b); // Warna Cover Buku

    // 1. Cover Lembar Kiri
    glPushMatrix();
    glTranslatef(-(w / 2.0f) + (coverThickness / 2.0f), 0.0f, 0.0f);
    glScalef(coverThickness, h, d);
    glutSolidCube(1.0);
    glPopMatrix();

    // 2. Cover Lembar Kanan
    glPushMatrix();
    glTranslatef((w / 2.0f) - (coverThickness / 2.0f), 0.0f, 0.0f);
    glScalef(coverThickness, h, d);
    glutSolidCube(1.0);
    glPopMatrix();

    // 3. Punggung Belakang Buku (Menutup Bagian Belakang Rak)
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -(d / 2.0f) + (coverThickness / 2.0f));
    glScalef(w, h, coverThickness);
    glutSolidCube(1.0);
    glPopMatrix();

    // -------------------------------------------------------------------------
    // C. MENEMPELKAN JUDUL DI SAMPING COVER BUKU (SISI KANAN & SISI KIRI)
    // -------------------------------------------------------------------------
    glDisable(GL_LIGHTING);
    GLboolean isCullEnabled = glIsEnabled(GL_CULL_FACE);
    if (isCullEnabled) glDisable(GL_CULL_FACE);

    float textScale = 0.00045f;
    float totalTextLength = (float)glutStrokeLength(GLUT_STROKE_ROMAN, (const unsigned char*)title);
    float realTextLength3D = totalTextLength * textScale;
    float startX = -(realTextLength3D / 2.0f);
    float startY = -(textScale * 100.0f) / 2.0f;

    // === 1. TEKS DI COVER SEBELAH KANAN ===
    glPushMatrix();
    glTranslatef((w / 2.0f) + 0.005f, 0.0f, 0.0f); 
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f); // Menghadap ke kanan
    glPushMatrix();
    glTranslatef(startX, startY, 0.0f);
    glScalef(textScale, textScale, textScale);
    glColor3f(1.0f, 1.0f, 1.0f); // Warna putih murni
    glLineWidth(2.0f);
    for (int i = 0; i < strlen(title); i++) {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, title[i]);
    }
    glPopMatrix();
    glPopMatrix();

    // === 2. TAMBAHAN: TEKS DI COVER SEBELAH KIRI ===
    glPushMatrix();
    glTranslatef(-(w / 2.0f) - 0.005f, 0.0f, 0.0f); 
    glRotatef(-90.0f, 0.0f, 1.0f, 0.0f); // Menghadap ke kiri (kebalikan dari sisi kanan)
    glPushMatrix();
    glTranslatef(startX, startY, 0.0f);
    glScalef(textScale, textScale, textScale);
    glColor3f(1.0f, 1.0f, 1.0f); // Warna putih murni
    glLineWidth(2.0f);
    for (int i = 0; i < strlen(title); i++) {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, title[i]);
    }
    glPopMatrix();
    glPopMatrix();

    if (isCullEnabled) glEnable(GL_CULL_FACE);
    if (isLightOn) glEnable(GL_LIGHTING);
}

void drawBookshelf() {
    glPushMatrix();
    glTranslatef(8.0f, 0.0f, 3.0f); 
    glColor3f(0.4f, 0.2f, 0.1f);
    glPushMatrix(); glTranslatef(-1.5f, 4.0f, 0.0f); glScalef(0.2f, 8.0f, 2.0f); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix(); glTranslatef(1.5f, 4.0f, 0.0f);  glScalef(0.2f, 8.0f, 2.0f); glutSolidCube(1.0); glPopMatrix();
    
    for(float y = 0.5f; y <= 7.0f; y += 2.0f) {
        glPushMatrix();
        glTranslatef(0.0f, y, 0.0f);
        glScalef(3.0f, 0.2f, 1.8f);
        glutSolidCube(1.0);
        glPopMatrix();
    }
    
    // TINGKAT 1: Buku Berdiri Tegak Berjejer (Sisi Kertas di Depan Kamera)
    glPushMatrix(); glTranslatef(-0.8f, 1.1f, 0.2f); drawSingleBook(0.35f, 1.1f, 1.2f, 0.8f, 0.1f, 0.1f, "Panduan Lab FTI"); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.4f, 1.0f, 0.2f); drawSingleBook(0.32f, 0.95f, 1.2f, 0.1f, 0.3f, 0.8f, "C++ Basic"); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 1.15f, 0.2f); drawSingleBook(0.38f, 1.2f, 1.2f, 0.2f, 0.7f, 0.2f, "Algoritma"); glPopMatrix();
    
    // TINGKAT 2: Buku Desain
    glPushMatrix(); glTranslatef(0.4f, 3.1f, 0.2f); drawSingleBook(0.35f, 1.1f, 1.2f, 0.9f, 0.6f, 0.1f, "KreatorHub Master"); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 3.0f, 0.2f); drawSingleBook(0.35f, 0.9f, 1.2f, 0.4f, 0.4f, 0.4f, "UI/UX Design"); glPopMatrix();
    
    // TINGKAT 3: Buku Istimewa Dimiringkan Bersandar
    glPushMatrix(); 
    glTranslatef(-0.6f, 5.1f, 0.2f); 
    glRotatef(-16.0f, 0.0f, 0.0f, 1.0f); 
    drawSingleBook(0.32f, 1.1f, 1.2f, 0.9f, 0.3f, 0.6f, "Memori Anggita"); 
    glPopMatrix();
    
    glPopMatrix(); 
}

void drawShirt(float x, float y, float z, float r, float g, float b) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glColor3f(0.8f, 0.8f, 0.8f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(0.0f, 0.5f, 0.0f);
    glVertex3f(-0.8f, 0.0f, 0.0f);
    glVertex3f(0.8f, 0.0f, 0.0f);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.5f, 0.0f);
    glVertex3f(0.0f, 0.7f, 0.0f);
    glEnd();
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f); 
    glVertex3f(-0.7f, 0.0f, 0.1f);  glVertex3f(0.7f, 0.0f, 0.1f);
    glVertex3f(0.6f, -1.5f, 0.1f);  glVertex3f(-0.6f, -1.5f, 0.1f);
    glNormal3f(0.0f, 0.0f, -1.0f); 
    glVertex3f(-0.7f, 0.0f, -0.1f); glVertex3f(0.7f, 0.0f, -0.1f);
    glVertex3f(0.6f, -1.5f, -0.1f); glVertex3f(-0.6f, -1.5f, -0.1f);
    glEnd();
    glPopMatrix();
}

void drawWardrobeAndClothes() {
    glPushMatrix();
    glTranslatef(-7.0f, 0.0f, 5.0f); 
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glColor3f(0.5f, 0.35f, 0.25f);
    glPushMatrix(); glTranslatef(-2.0f, 4.0f, 0.0f); glScalef(0.2f, 8.0f, 3.0f); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix(); glTranslatef(2.0f, 4.0f, 0.0f);  glScalef(0.2f, 8.0f, 3.0f); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 4.0f, -1.4f); glScalef(4.0f, 8.0f, 0.2f); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 7.9f, 0.0f);  glScalef(4.0f, 0.2f, 3.0f); glutSolidCube(1.0); glPopMatrix();
    glColor3f(0.8f, 0.8f, 0.8f);
    glPushMatrix();
    glTranslatef(0.0f, 6.5f, 0.0f);
    glScalef(4.0f, 0.1f, 0.1f);
    glutSolidCube(1.0);
    glPopMatrix();
    drawShirt(-1.0f, 5.8f, 0.0f,  0.8f, 0.2f, 0.2f); 
    drawShirt(-0.2f, 5.8f, 0.0f,  0.2f, 0.3f, 0.8f); 
    drawShirt(0.6f, 5.8f, 0.0f,  0.9f, 0.9f, 0.9f); 
    drawShirt(1.4f, 5.8f, 0.0f,  0.2f, 0.8f, 0.3f); 
    glPopMatrix(); 
}

// ==============================================================================
// SISTEM PENCAHAYAAN
// ==============================================================================
void initLighting() {
    GLfloat lightAmbient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat lightDiffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat lightPosition[] = { 0.0f, 9.0f, 0.0f, 1.0f }; 

    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
}

// ==============================================================================
// LOGIKA GERAK & INPUT
// ==============================================================================

// Fungsi pembantu untuk mengecek apakah posisi (x, z) berada di dalam suatu kotak perabotan
bool checkCollision(float x, float z) {
    // 1. BATAS TEMBOK KAMAR (Kamar luasnya dari -10 sampai 10)
    // Berikan jarak aman (radius kamera) sebesar 0.5 agar kamera tidak terlalu mepet tembok
    if (x < -9.5f || x > 9.5f || z < -9.5f || z > 9.5f) {
        return true; 
    }

    // 2. KOTAK KASUR (drawBed) -> Posisi di glTranslatef(-6.0f, 0.0f, -6.0f) dengan skala kubus 5.0 x 7.0
    // Batas X: -6.0 - 2.5 = -8.5 sampai -6.0 + 2.5 = -3.5
    // Batas Z: -6.0 - 3.5 = -9.5 sampai -6.0 + 3.5 = -2.5
    if (x >= -8.7f && x <= -3.3f && z >= -9.7f && z <= -2.3f) {
        return true;
    }

    // 3. KOTAK MEJA BELAJAR (drawDeskAndLaptop) -> Posisi di glTranslatef(6.0f, 0.0f, -7.0f) skala 5.0 x 3.0
    // Batas X: 6.0 - 2.5 = 3.5 sampai 6.0 + 2.5 = 8.5
    // Batas Z: -7.0 - 1.5 = -8.5 sampai -7.0 + 1.5 = -5.5
    if (x >= 3.3f && x <= 8.7f && z >= -8.7f && z <= -5.3f) {
        return true;
    }

    // 4. KOTAK RAK BUKU (drawBookshelf) -> Posisi di glTranslatef(8.0f, 0.0f, 3.0f) skala 3.0 x 2.0
    // Batas X: 8.0 - 1.5 = 6.5 sampai 8.0 + 1.5 = 9.5
    // Batas Z: 3.0 - 1.0 = 2.0 sampai 3.0 + 1.0 = 4.0
    if (x >= 6.3f && x <= 9.7f && z >= 1.7f && z <= 4.3f) {
        return true;
    }

    // 5. KOTAK LEMARI BAJU (drawWardrobeAndClothes) -> Posisi di glTranslatef(-7.0f, 0.0f, 5.0f) 
    // Karena dirotasi 90 derajat, skalanya bertukar: X jadi tebal (3.0), Z jadi lebar (4.0)
    // Batas X: -7.0 - 1.5 = -8.5 sampai -7.0 + 1.5 = -5.5
    // Batas Z: 5.0 - 2.0 = 3.0 sampai 5.0 + 2.0 = 7.0
    if (x >= -8.7f && x <= -5.3f && z >= 2.7f && z <= 7.3f) {
        return true;
    }

    return false; // Jika tidak mengenai apa pun, aman (tidak tabrakan)
}

void handleMovement() {
    float moveSpeed = 0.15f; 
    float dirX = sin(angleH) * cos(angleV);
    float dirZ = -cos(angleH) * cos(angleV);
    float rightX = cos(angleH);
    float rightZ = sin(angleH);

    // Variabel sementara untuk menampung posisi baru sebelum dieksekusi
    float nextX = camX;
    float nextZ = camZ;

    // Kalkulasi pergerakan berdasarkan tombol keyboard yang ditekan
    if (keys['w'] || keys['W']) { 
        nextX += dirX * moveSpeed; 
        nextZ += dirZ * moveSpeed; 
    }
    if (keys['s'] || keys['S']) { 
        nextX -= dirX * moveSpeed; 
        nextZ -= dirZ * moveSpeed; 
    }
    if (keys['a'] || keys['A']) { 
        nextX -= rightX * moveSpeed; 
        nextZ -= rightZ * moveSpeed; 
    }
    if (keys['d'] || keys['D']) { 
        nextX += rightX * moveSpeed; 
        nextZ += rightZ * moveSpeed; 
    }

    // SELEKSI FISIK: Hanya update posisi asli jika jalur masa depan TIDAK Nabrak!
    if (!checkCollision(nextX, camZ)) {
        camX = nextX;
    }
    if (!checkCollision(camX, nextZ)) {
        camZ = nextZ;
    }
}

void timer(int value) {
    handleMovement(); 
    updateCameraLook();

    if (isFanMoving) {
        fanAngle += 15.0f; 
        if (fanAngle > 360.0f) fanAngle -= 360.0f;
    }
    if (isDoorOpen && doorAngle > -90.0f) doorAngle -= 4.0f;
    else if (!isDoorOpen && doorAngle < 0.0f) doorAngle += 4.0f;

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); 
}

void display() {
    glClearColor(0.5f, 0.8f, 0.9f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, (double)glutGet(GLUT_WINDOW_WIDTH) / (double)glutGet(GLUT_WINDOW_HEIGHT), 0.1, 200.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(camX, camY, camZ, lookX, lookY, lookZ, 0.0f, 1.0f, 0.0f);

    glDisable(GL_LIGHTING); 
    drawPoster();
    if (isLightOn) {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
    } else {
        glDisable(GL_LIGHTING);
    }

    drawEnvironment(); 
    drawRoom();        
    drawPoster();
    drawDoor();        
    drawCurtains();
    drawFan();          
    drawBed();
    drawDeskAndLaptop();
    drawBookshelf();
    drawWardrobeAndClothes();

    glutSwapBuffers();
}

void keyDown(unsigned char key, int x, int y) {
    keys[key] = true; 
    switch (key) {
        case 'k': case 'K': isFanMoving = !isFanMoving; break;
        case 'l': case 'L': isLightOn = !isLightOn; break;
        case 'f': case 'F': isDoorOpen = !isDoorOpen; break;
        case 27: exit(0); break;
    }
}

void keyUp(unsigned char key, int x, int y) { keys[key] = false; }

void mouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        isMouseLeftDown = (state == GLUT_DOWN);
        lastMouseX = x; lastMouseY = y;
    }
}

void mouseMotion(int x, int y) {
    if (isMouseLeftDown) {
        angleH += (x - lastMouseX) * 0.005f;
        angleV -= (y - lastMouseY) * 0.005f;
        if (angleV > 1.5f) angleV = 1.5f;
        if (angleV < -1.5f) angleV = -1.5f;
        lastMouseX = x; lastMouseY = y;
    }
}

void mouseWheel(int wheel, int direction, int x, int y) {
    if (direction > 0) { fov -= 3.0f; if (fov < 10.0f) fov = 10.0f; } 
    else { fov += 3.0f; if (fov > 120.0f) fov = 120.0f; }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1024, 768);
    glutCreateWindow("Tugas Akhir Grafika Komputer - Kamar Kos Interaktif");
    
    glutDisplayFunc(display);
    loadTexture();
    glutFullScreen(); 

    for(int i = 0; i < 256; i++) keys[i] = false;

    glEnable(GL_DEPTH_TEST);
    initLighting();

    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp); 
    glutMouseFunc(mouseButton);
    glutPassiveMotionFunc(passiveMouseMotion);
    glutSetCursor(GLUT_CURSOR_NONE);
    glutMouseWheelFunc(mouseWheel); 
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}