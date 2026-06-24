#define STB_IMAGE_IMPLEMENTATION
#include "library/stb_image.h" 
#include <GL/freeglut.h>
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

// ==============================================================================
// TEKSTUR (Hanya framework, siapkan file foto jika dosen meminta)
// ==============================================================================
// Jika kamu ingin menggunakan tekstur lantai, kamu bisa menggunakan library
// stb_image.h. Di sini saya sediakan ID tekstur sebagai persiapan.
GLuint floorTexture;
bool useTexture = true; // Ubah ke true jika stb_image sudah diimplementasikan

GLuint loadTexture(const char* filename) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    } else {
        std::cout << "Gagal memuat tekstur: " << filename << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}

// ==============================================================================
// FUNGSI UTILITAS GEOMETRI & TEKS
// ==============================================================================
void updateCameraLook() {
    lookX = camX + sin(angleH) * cos(angleV);
    lookY = camY + sin(angleV);
    lookZ = camZ - cos(angleH) * cos(angleV);
}

// Fungsi untuk merender teks 3D (judul buku dll)
void drawText3D(const char* text, float x, float y, float z, float scale, float rotY) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);
    glColor3f(1.0f, 1.0f, 1.0f); // Teks putih
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
    // Lantai
    glColor3f(0.8f, 0.7f, 0.6f); // Warna kayu minimalis
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-10.0f, 0.0f, 10.0f); glVertex3f(10.0f, 0.0f, 10.0f);
    glVertex3f(10.0f, 0.0f, -10.0f); glVertex3f(-10.0f, 0.0f, -10.0f);
    glEnd();

    // Atap
    glColor3f(0.9f, 0.9f, 0.9f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-10.0f, 10.0f, -10.0f); glVertex3f(10.0f, 10.0f, -10.0f);
    glVertex3f(10.0f, 10.0f, 10.0f); glVertex3f(-10.0f, 10.0f, 10.0f);
    glEnd();

    // Tembok Belakang & Kanan
    glColor3f(0.95f, 0.95f, 0.95f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-10.0f, 0.0f, -10.0f); glVertex3f(10.0f, 0.0f, -10.0f);
    glVertex3f(10.0f, 10.0f, -10.0f); glVertex3f(-10.0f, 10.0f, -10.0f);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(10.0f, 0.0f, -10.0f); glVertex3f(10.0f, 0.0f, 10.0f);
    glVertex3f(10.0f, 10.0f, 10.0f); glVertex3f(10.0f, 10.0f, -10.0f);
    glEnd();

    // Tembok Kiri (Dengan Ventilasi & Jendela)
    glColor3f(0.92f, 0.92f, 0.92f);
    glBegin(GL_QUADS);
    glNormal3f(1.0f, 0.0f, 0.0f);
    // Bawah Jendela
    glVertex3f(-10.0f, 0.0f, 10.0f); glVertex3f(-10.0f, 0.0f, -10.0f);
    glVertex3f(-10.0f, 3.0f, -10.0f); glVertex3f(-10.0f, 3.0f, 10.0f);
    // Atas Jendela
    glVertex3f(-10.0f, 7.0f, 10.0f); glVertex3f(-10.0f, 7.0f, -10.0f);
    glVertex3f(-10.0f, 10.0f, -10.0f); glVertex3f(-10.0f, 10.0f, 10.0f);
    // Kiri Jendela
    glVertex3f(-10.0f, 3.0f, 10.0f); glVertex3f(-10.0f, 3.0f, 2.0f);
    glVertex3f(-10.0f, 7.0f, 2.0f); glVertex3f(-10.0f, 7.0f, 10.0f);
    // Kanan Jendela
    glVertex3f(-10.0f, 3.0f, -4.0f); glVertex3f(-10.0f, 3.0f, -10.0f);
    glVertex3f(-10.0f, 7.0f, -10.0f); glVertex3f(-10.0f, 7.0f, -4.0f);
    glEnd();

    // Tembok Depan (Dengan Pintu)
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

// ==============================================================================
// GORDEN JENDELA (Native Vertex Melengkung)
// ==============================================================================
void drawCurtains() {
    glPushMatrix();
    glTranslatef(-9.8f, 7.0f, -4.0f); // Posisi gorden di sisi jendela
    glColor3f(0.4f, 0.4f, 0.5f); // Warna navy elegan minimalis

    // Membuat lipatan gorden dengan kurva sinus secara native
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 60; i++) {
        float zPos = i * 0.1f; // Lebar gorden ke arah sumbu Z
        float xWave = sin(i * 0.5f) * 0.2f; // Gelombang lipatan
        glNormal3f(1.0f, 0.0f, 0.0f);
        glVertex3f(xWave, 0.0f, zPos);      // Titik atas
        glVertex3f(xWave, -4.5f, zPos);     // Titik bawah menjuntai
    }
    glEnd();
    glPopMatrix();
}

void drawDoor() {
    glPushMatrix();
    glTranslatef(-2.0f, 0.0f, 10.0f); 
    glRotatef(doorAngle, 0.0f, 1.0f, 0.0f); 
    
    // Daun Pintu
    glColor3f(0.5f, 0.3f, 0.2f); 
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, -1.0f); 
    glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(4.0f, 0.0f, 0.0f);
    glVertex3f(4.0f, 6.0f, 0.0f); glVertex3f(0.0f, 6.0f, 0.0f);
    glEnd();

    // Gagang Pintu (Detail)
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

// ==============================================================================
// FURNITUR KAMAR
// ==============================================================================
void drawBed() {
    glPushMatrix();
    glTranslatef(-6.0f, 0.0f, -6.0f);
    
    // Rangka Kasur
    glColor3f(0.3f, 0.2f, 0.1f);
    glPushMatrix();
    glTranslatef(0.0f, 0.5f, 0.0f);
    glScalef(5.0f, 1.0f, 7.0f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Matras
    glColor3f(0.9f, 0.9f, 0.9f); // Putih bersih
    glPushMatrix();
    glTranslatef(0.0f, 1.2f, 0.0f);
    glScalef(4.8f, 0.6f, 6.8f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Bantal 1
    glColor3f(0.8f, 0.8f, 0.85f);
    glPushMatrix();
    glTranslatef(-1.2f, 1.6f, -2.5f);
    glScalef(1.8f, 0.3f, 1.2f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Bantal 2
    glPushMatrix();
    glTranslatef(1.2f, 1.6f, -2.5f);
    glScalef(1.8f, 0.3f, 1.2f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Selimut (Detail native dengan lipatan kecil di ujung)
    glColor3f(0.3f, 0.5f, 0.6f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-2.45f, 1.55f, -0.5f); glVertex3f(2.45f, 1.55f, -0.5f);
    glVertex3f(2.45f, 1.55f, 3.45f);  glVertex3f(-2.45f, 1.55f, 3.45f);
    // Juntaian selimut
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-2.45f, 1.55f, 3.45f); glVertex3f(2.45f, 1.55f, 3.45f);
    glVertex3f(2.45f, 0.5f, 3.45f);   glVertex3f(-2.45f, 0.5f, 3.45f);
    glEnd();

    glPopMatrix();
}

void drawDeskAndLaptop() {
    glPushMatrix();
    glTranslatef(6.0f, 0.0f, -7.0f);
    
    // Papan Meja
    glColor3f(0.2f, 0.2f, 0.2f); // Meja hitam elegan
    glPushMatrix();
    glTranslatef(0.0f, 2.5f, 0.0f);
    glScalef(5.0f, 0.2f, 3.0f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Kaki Meja
    glColor3f(0.6f, 0.6f, 0.6f);
    float legPos[4][2] = {{-2.3f, -1.3f}, {2.3f, -1.3f}, {-2.3f, 1.3f}, {2.3f, 1.3f}};
    for (int i=0; i<4; i++) {
        glPushMatrix();
        glTranslatef(legPos[i][0], 1.25f, legPos[i][1]);
        glScalef(0.2f, 2.5f, 0.2f);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    // --- LAPTOP (Silver M4 Style) ---
    glPushMatrix();
    glTranslatef(0.0f, 2.65f, 0.0f);
    glRotatef(-15.0f, 0.0f, 1.0f, 0.0f); 

    // Base Laptop
    glColor3f(0.75f, 0.75f, 0.75f); // Silver
    glPushMatrix();
    glScalef(1.8f, 0.08f, 1.2f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Keyboard Hitam (Detail Hardcode)
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix();
    glTranslatef(0.0f, 0.05f, 0.1f);
    glScalef(1.6f, 0.05f, 0.8f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Layar Laptop Terbuka
    glTranslatef(0.0f, 0.5f, -0.6f);
    glRotatef(-20.0f, 1.0f, 0.0f, 0.0f); 
    
    // Casing Layar
    glColor3f(0.75f, 0.75f, 0.75f);
    glPushMatrix();
    glScalef(1.8f, 1.2f, 0.08f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Panel Layar (Menyala)
    glColor3f(0.1f, 0.8f, 0.9f); // Warna layar cyan/menyala
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.05f);
    glScalef(1.7f, 1.1f, 0.02f);
    glutSolidCube(1.0);
    glPopMatrix();

    glPopMatrix(); // End Laptop
    glPopMatrix(); // End Desk
}

// ==============================================================================
// BUKU DAN RAK BUKU (Detail Manual)
// ==============================================================================
void drawSingleBook(float w, float h, float d, float r, float g, float b, const char* title) {
    // Cover Buku
    glColor3f(r, g, b);
    glPushMatrix();
    glScalef(w, h, d);
    glutSolidCube(1.0);
    glPopMatrix();

    // Halaman Buku
    glColor3f(0.9f, 0.9f, 0.9f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-w/2.05f, -h/2.05f, d/2.05f); glVertex3f(w/2.05f, -h/2.05f, d/2.05f);
    glVertex3f(w/2.05f, h/2.05f, d/2.05f);   glVertex3f(-w/2.05f, h/2.05f, d/2.05f);
    glEnd();

    // Teks di punggung buku
    glPushMatrix();
    // Digeser lebih ke ujung bawah (-h/2.2f) agar tulisan panjang bisa muat
    glTranslatef(-w/2.5f, -h/2.2f, d/2.0f + 0.01f); 
    // Skala diperkecil drastis (dari 0.0015f menjadi 0.0007f)
    glScalef(0.0007f, 0.0007f, 0.0007f); 
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(1.5f);
    for (int i = 0; i < strlen(title); i++) {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, title[i]);
    }
    glPopMatrix();
}

void drawBookshelf() {
    glPushMatrix();
    glTranslatef(8.0f, 0.0f, 3.0f); // Posisi Rak
    
    // Struktur Rak (Kayu gelap)
    glColor3f(0.4f, 0.2f, 0.1f);
    
    // Sisi Kiri & Kanan
    glPushMatrix(); glTranslatef(-1.5f, 4.0f, 0.0f); glScalef(0.2f, 8.0f, 2.0f); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix(); glTranslatef(1.5f, 4.0f, 0.0f);  glScalef(0.2f, 8.0f, 2.0f); glutSolidCube(1.0); glPopMatrix();
    
    // Hambalan (Rak 1, 2, 3, 4)
    for(float y = 0.5f; y <= 7.0f; y += 2.0f) {
        glPushMatrix();
        glTranslatef(0.0f, y, 0.0f);
        glScalef(3.0f, 0.2f, 1.8f);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    // --- MENGISI RAK DENGAN BUKU (Hardcoded untuk memenuhi permintaan baris & detail) ---
    // Rak 1 (Bawah)
    glPushMatrix(); glTranslatef(-1.0f, 1.1f, 0.0f); drawSingleBook(0.4f, 1.0f, 1.2f, 0.8f, 0.1f, 0.1f, "Panduan Lab FTI"); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.5f, 1.1f, 0.0f); drawSingleBook(0.3f, 0.9f, 1.2f, 0.1f, 0.3f, 0.8f, "C++ Basic"); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.1f, 1.1f, 0.0f); drawSingleBook(0.5f, 1.1f, 1.3f, 0.2f, 0.8f, 0.2f, "Algoritma"); glPopMatrix();

    // Rak 2
    glPushMatrix(); glTranslatef(1.0f, 3.1f, 0.0f); drawSingleBook(0.3f, 1.0f, 1.1f, 0.9f, 0.6f, 0.1f, "KreatorHub Master"); glPopMatrix();
    glPushMatrix(); glTranslatef(0.6f, 3.1f, 0.0f); drawSingleBook(0.4f, 0.8f, 1.1f, 0.4f, 0.4f, 0.4f, "UI/UX Design"); glPopMatrix();

    // Rak 3 (Miring sedikit untuk realisme)
    glPushMatrix(); 
    glTranslatef(-1.0f, 5.1f, 0.0f); 
    glRotatef(15.0f, 0.0f, 0.0f, 1.0f); // Buku miring
    drawSingleBook(0.3f, 1.0f, 1.2f, 0.9f, 0.3f, 0.6f, "Memori Anggita"); 
    glPopMatrix();

    glPopMatrix(); // End Bookshelf
}

// ==============================================================================
// LEMARI & GANTUNGAN BAJU (Geometri Kompleks)
// ==============================================================================
void drawShirt(float x, float y, float z, float r, float g, float b) {
    glPushMatrix();
    glTranslatef(x, y, z);
    
    // Gantungan (Hanger) kawat
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

    // Kain Baju (Native Quads)
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f); // Depan
    glVertex3f(-0.7f, 0.0f, 0.1f);  glVertex3f(0.7f, 0.0f, 0.1f);
    glVertex3f(0.6f, -1.5f, 0.1f);  glVertex3f(-0.6f, -1.5f, 0.1f);
    glNormal3f(0.0f, 0.0f, -1.0f); // Belakang
    glVertex3f(-0.7f, 0.0f, -0.1f); glVertex3f(0.7f, 0.0f, -0.1f);
    glVertex3f(0.6f, -1.5f, -0.1f); glVertex3f(-0.6f, -1.5f, -0.1f);
    glEnd();

    glPopMatrix();
}

void drawWardrobeAndClothes() {
    glPushMatrix();
    glTranslatef(-7.0f, 0.0f, 5.0f); 
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    
    // Lemari Terbuka (Tanpa pintu agar baju terlihat)
    glColor3f(0.5f, 0.35f, 0.25f);
    // Sisi Kiri
    glPushMatrix(); glTranslatef(-2.0f, 4.0f, 0.0f); glScalef(0.2f, 8.0f, 3.0f); glutSolidCube(1.0); glPopMatrix();
    // Sisi Kanan
    glPushMatrix(); glTranslatef(2.0f, 4.0f, 0.0f);  glScalef(0.2f, 8.0f, 3.0f); glutSolidCube(1.0); glPopMatrix();
    // Belakang
    glPushMatrix(); glTranslatef(0.0f, 4.0f, -1.4f); glScalef(4.0f, 8.0f, 0.2f); glutSolidCube(1.0); glPopMatrix();
    // Atap Lemari
    glPushMatrix(); glTranslatef(0.0f, 7.9f, 0.0f);  glScalef(4.0f, 0.2f, 3.0f); glutSolidCube(1.0); glPopMatrix();
    
    // Pipa Besi Gantungan
    glColor3f(0.8f, 0.8f, 0.8f);
    glPushMatrix();
    glTranslatef(0.0f, 6.5f, 0.0f);
    glScalef(4.0f, 0.1f, 0.1f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Baju yang digantung (Memanggil fungsi baju manual)
    drawShirt(-1.0f, 5.8f, 0.0f,  0.8f, 0.2f, 0.2f); // Baju Merah
    drawShirt(-0.2f, 5.8f, 0.0f,  0.2f, 0.3f, 0.8f); // Baju Biru
    drawShirt(0.6f, 5.8f, 0.0f,  0.9f, 0.9f, 0.9f); // Baju Putih
    drawShirt(1.4f, 5.8f, 0.0f,  0.2f, 0.8f, 0.3f); // Baju Hijau

    glPopMatrix(); // End Wardrobe
}

// ==============================================================================
// SISTEM PENCAHAYAAN
// ==============================================================================
void initLighting() {
    GLfloat lightAmbient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    GLfloat lightDiffuse[] = { 0.9f, 0.9f, 0.9f, 1.0f };
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
void handleMovement() {
    float moveSpeed = 0.15f; 
    
    // Vektor arah maju (Forward)
    float dirX = sin(angleH) * cos(angleV);
    float dirY = sin(angleV);
    float dirZ = -cos(angleH) * cos(angleV);
    
    // PERBAIKAN: Vektor arah samping (True Right Vector)
    float rightX = cos(angleH);
    float rightZ = sin(angleH);

    // W = Maju
    if (keys['w'] || keys['W']) { 
        camX += dirX * moveSpeed; camY += dirY * moveSpeed; camZ += dirZ * moveSpeed; 
    }
    // A = Geser Kiri (Mengikuti Kamera)
    if (keys['a'] || keys['A']) { 
        camX -= rightX * moveSpeed; camZ -= rightZ * moveSpeed; 
    }
    // S = Geser Kanan (Sesuai permintaan)
    if (keys['d'] || keys['D']) { 
        camX += rightX * moveSpeed; camZ += rightZ * moveSpeed; 
    }
    // D = Mundur (Dialokasikan ke D karena S dipakai untuk geser kanan)
    if (keys['s'] || keys['S']) { 
        camX -= dirX * moveSpeed; camY -= dirY * moveSpeed; camZ -= dirZ * moveSpeed; 
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

    if (isLightOn) {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
    } else {
        glDisable(GL_LIGHTING);
    }

    drawEnvironment(); 
    drawRoom();        
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
    glutFullScreen(); 

    for(int i = 0; i < 256; i++) keys[i] = false;

    glEnable(GL_DEPTH_TEST);
    initLighting();
    floorTexture = loadTexture("C:/Users/Bravo/OneDrive/Dokumen/Arpidddd/SEMESTER 6_25-26 (2)/Grafika Komputer/Utility for Visual Studio/TR/lantai.jpg");
    glutDisplayFunc(display);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp); 
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutMouseWheelFunc(mouseWheel); 
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}