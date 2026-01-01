#include <GL/glut.h>
#include <GL/freeglut.h>
#include <cmath>
#include <GL/glu.h>
#include <string>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Window dimensions
int windowWidth = 1200;
int windowHeight = 800;
// Global variable to control floodlight state
bool nightMode = false; // Default to day (lights off)
// --- GAME ANIMATION VARIABLES ---
bool isPlaying = false;
int animStage = 0; // 0=Wait, 1=Run Up, 2=Ball Flying

// Positions
float ballX = 0.0f, ballZ = 0.0f, ballRot = 0.0f;
float strikerX = -6.0f, strikerZ = 0.0f; // Start behind the ball
float goalieZ = 0.0f;

// Velocities
float ballVelX = 0.0f, ballVelZ = 0.0f;

// **********************************************
// ************ CAMERA VARIABLES ****************
// **********************************************
float angleY = 0.0f;
float angleX = 20.0f;
float camDist = 140.0f; 
float cameraHeight = 25.0f;
float lookAtHeight = 0.0f;

float cameraX = 0.0f, cameraY = cameraHeight, cameraZ = camDist;
float targetX = 0.0f, targetY = lookAtHeight, targetZ = 0.0f;

float deltaAngleY = 0.0f, deltaAngleX = 0.0f, deltaMove = 0.0f;

// **********************************************
// ************ DIMENSIONS (FIXED) **************
// **********************************************

// 1. INCREASE TRACK SIZE to accommodate the field corners
const float TRACK_INNER_X_RADIUS = 55.0f; 
const float TRACK_INNER_Z_RADIUS = 38.0f; // Made wider to prevent corner overlap

const float TRACK_WIDTH = 10.0f; 
const float TRACK_OUTER_X_RADIUS = TRACK_INNER_X_RADIUS + TRACK_WIDTH;
const float TRACK_OUTER_Z_RADIUS = TRACK_INNER_Z_RADIUS + TRACK_WIDTH;

// 2. SHRINK FIELD SIZE to ensure corners don't touch the track
// The field is now significantly smaller than the track inner radius
const float FIELD_X_RADIUS = 40.0f; // Leaves ~15 units of "D" area length
const float FIELD_Z_RADIUS = 24.0f; // Leaves ~14 units of side area width

// Corner Check: (40/55)^2 + (24/38)^2 = 0.53 + 0.40 = 0.93 < 1.0 (Safe inside)

const float SEATING_BASE_X_RADIUS = TRACK_OUTER_X_RADIUS;
const float SEATING_BASE_Z_RADIUS = TRACK_OUTER_Z_RADIUS;

const int NUM_TIERS = 8;       
const float TIER_HEIGHT = 1.2f; 
const float TIER_DEPTH_INCREASE_X = 1.2f;
const float TIER_DEPTH_INCREASE_Z = 1.2f;

const float MAX_SEATING_X_RADIUS = SEATING_BASE_X_RADIUS + (NUM_TIERS - 1) * TIER_DEPTH_INCREASE_X;
const float MAX_SEATING_Z_RADIUS = SEATING_BASE_Z_RADIUS + (NUM_TIERS - 1) * TIER_DEPTH_INCREASE_Z;

const float STADIUM_TOTAL_HEIGHT = (NUM_TIERS - 1) * TIER_HEIGHT;
const float MAIN_GRANDSTAND_WIDTH = 120.0f; 

GLUquadricObj *quadric;

// **********************************************
// ************ DRAWING FUNCTIONS ***************
// **********************************************

void drawSeat() {
    glColor3f(0.1f, 0.1f, 0.9f);
    glPushMatrix();
    glScalef(0.8f, 0.4f, 0.6f); 
    glutSolidCube(1.0);
    glPopMatrix();
}

void drawOvalSeatRow(float baseX, float baseZ, float y, float tierIncreaseX, float tierIncreaseZ, float angleOffset) {
    float currentRadiusX = baseX + tierIncreaseX;
    float currentRadiusZ = baseZ + tierIncreaseZ;
    
    // Calculate seats based on circumference
    float h_val = pow(currentRadiusX - currentRadiusZ, 2) / pow(currentRadiusX + currentRadiusZ, 2);
    float circumference = M_PI * (currentRadiusX + currentRadiusZ) * (1 + (3 * h_val) / (10 + sqrt(4 - 3 * h_val)));
    
    int numSeats = (int)(circumference * 1.1f); 
    float angleStep = 360.0f / (float)numSeats;
    
    // GAP SETTING: How wide is the opening in degrees?
    float gapWidth = 14.0f; 

    for (int i = 0; i < numSeats; ++i) {
        float angleDeg = angleOffset + i * angleStep;
        
        // Normalize angle to 0-360 range
        while(angleDeg >= 360.0f) angleDeg -= 360.0f;
        while(angleDeg < 0.0f) angleDeg += 360.0f;

        // --- CUT THE TIERS ---
        // Gate A is at 0 degrees (360). Skip seats near 0.
        if (angleDeg < gapWidth || angleDeg > 360.0f - gapWidth) continue;

        // Gate B is at 180 degrees. Skip seats near 180.
        if (angleDeg > 180.0f - gapWidth && angleDeg < 180.0f + gapWidth) continue;
        // ---------------------

        float angle = angleDeg * M_PI / 180.0f;
        float currentX = currentRadiusX * cos(angle);
        float currentZ = currentRadiusZ * sin(angle);

        glPushMatrix();
        glTranslatef(currentX, y, currentZ);
        glRotatef(90.0f - angleDeg, 0.0f, 1.0f, 0.0f);
        drawSeat();
        glPopMatrix();
    }
}

void drawStadiumSeatingBowl() {
    for (int i = 0; i < NUM_TIERS; ++i) {
        float currentY = i * TIER_HEIGHT;
        float stagger = (i % 2 == 0) ? 0.0f : 0.5f;
        drawOvalSeatRow(SEATING_BASE_X_RADIUS, SEATING_BASE_Z_RADIUS, currentY,
                        i * TIER_DEPTH_INCREASE_X, i * TIER_DEPTH_INCREASE_Z, stagger);
    }
}

void drawMainGrandstandRoof() {
    glColor3f(0.8f, 0.8f, 0.9f); 
    float topTierZ = SEATING_BASE_Z_RADIUS + (NUM_TIERS - 1) * TIER_DEPTH_INCREASE_Z;

    glPushMatrix();
    glTranslatef(0.0f, STADIUM_TOTAL_HEIGHT + 10.0f, -topTierZ - 15.0f); 
    glPushMatrix();
    glScalef(MAIN_GRANDSTAND_WIDTH, 2.0f, 65.0f);
    glutSolidCube(1.0);
    glPopMatrix();
    
    glColor3f(0.3f, 0.3f, 0.3f);
    glPushMatrix();
    glTranslatef(0.0f, -2.0f, 0.0f);
    glScalef(MAIN_GRANDSTAND_WIDTH - 2.0f, 1.0f, 60.0f);
    glutWireCube(1.0);
    glPopMatrix();
    glPopMatrix();
}

void drawMainGrandstandColumns() {
    glColor3f(0.6f, 0.6f, 0.65f); 
    float topTierZ = SEATING_BASE_Z_RADIUS + (NUM_TIERS - 1) * TIER_DEPTH_INCREASE_Z;
    float roofHeight = STADIUM_TOTAL_HEIGHT + 10.0f;
    float columnZ = -topTierZ - 15.0f; 

    int numColumns = 8; 
    float columnSpacing = (MAIN_GRANDSTAND_WIDTH - 10.0f) / (numColumns - 1);
    float startX = -(MAIN_GRANDSTAND_WIDTH - 10.0f) / 2.0f;

    for (int i = 0; i < numColumns; ++i) {
        glPushMatrix();
        glTranslatef(startX + i * columnSpacing, roofHeight / 2.0f, columnZ);
        glScalef(2.0f, roofHeight, 2.0f); 
        glutSolidCube(1.0);
        glPopMatrix();
    }
}

void drawGrandstandFacade() {
    glColor3f(0.7f, 0.7f, 0.7f);
    float topTierZ = SEATING_BASE_Z_RADIUS + (NUM_TIERS - 1) * TIER_DEPTH_INCREASE_Z;
    float facadeZ = -topTierZ - 12.0f;

    glPushMatrix();
    glTranslatef(0.0f, STADIUM_TOTAL_HEIGHT / 2.0f, facadeZ);
    glScalef(MAIN_GRANDSTAND_WIDTH, STADIUM_TOTAL_HEIGHT, 1.0f);
    glutSolidCube(1.0);
    glPopMatrix();
}

void drawVIPSeating() {
    glColor3f(0.8f, 0.0f, 0.0f); 
    float topTierZ = SEATING_BASE_Z_RADIUS + (NUM_TIERS - 1) * TIER_DEPTH_INCREASE_Z;
    float vipY = STADIUM_TOTAL_HEIGHT + 2.0f;
    float vipZ = -topTierZ - 5.0f;

    glPushMatrix();
    glTranslatef(0.0f, vipY, vipZ);
    glPushMatrix();
    glScalef(MAIN_GRANDSTAND_WIDTH * 0.6f, 1.0f, 10.0f);
    glColor3f(0.2f, 0.2f, 0.2f);
    glutSolidCube(1.0);
    glPopMatrix();

    glColor3f(0.8f, 0.1f, 0.1f);
    for(float x = -MAIN_GRANDSTAND_WIDTH * 0.25f; x < MAIN_GRANDSTAND_WIDTH * 0.25f; x+=2.0f) {
        glPushMatrix();
        glTranslatef(x, 1.0f, -2.0f);
        glutSolidCube(1.0);
        glPopMatrix();
    }
    glPopMatrix();
}

void drawStadiumName() {
    std::string text = "ASTU STADIUM";
    glColor3f(1.0f, 1.0f, 0.0f); 

    float topTierZ = SEATING_BASE_Z_RADIUS + (NUM_TIERS - 1) * TIER_DEPTH_INCREASE_Z;
    float textY = STADIUM_TOTAL_HEIGHT + 8.0f; 
    float textZ = -topTierZ - 14.0f; 

    glPushMatrix();
    glTranslatef(0.0f, textY, textZ);
    float textWidth = 0.0f;
    for (std::string::size_type i = 0; i < text.size(); ++i) textWidth += glutStrokeWidth(GLUT_STROKE_ROMAN, (int)text[i]);
    glScalef(0.02f, 0.02f, 0.02f); 
    glTranslatef(-textWidth / 2.0f, 0.0f, 0.0f);
    glLineWidth(3.0f);
    for (std::string::size_type i = 0; i < text.size(); ++i) glutStrokeCharacter(GLUT_STROKE_ROMAN, (int)text[i]);
    glLineWidth(1.0f);
    glPopMatrix();
}

void drawStoneFacade() {
    glColor3f(0.5f, 0.5f, 0.55f); 
    float facadeHeight = STADIUM_TOTAL_HEIGHT;
    
    // We draw two separate strips to leave holes at 0 and 180 degrees
    float gap = 14.0f; // Must match the seat gap roughly
    int segments = 60; // Resolution per side

    // ARC 1: Back side (approx 15 to 165 degrees)
    float start1 = gap;
    float end1 = 180.0f - gap;

    glPushMatrix();
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i) {
        float t = (float)i / (float)segments;
        float angDeg = start1 * (1.0f - t) + end1 * t; // Interpolate angle
        float angRad = angDeg * M_PI / 180.0f;

        float x = MAX_SEATING_X_RADIUS * cos(angRad);
        float z = MAX_SEATING_Z_RADIUS * sin(angRad);
        glVertex3f(x, facadeHeight, z);
        glVertex3f(x, -5.0f, z); 
    }
    glEnd();
    glPopMatrix();

    // ARC 2: Front side (approx 195 to 345 degrees)
    float start2 = 180.0f + gap;
    float end2 = 360.0f - gap;

    glPushMatrix();
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i) {
        float t = (float)i / (float)segments;
        float angDeg = start2 * (1.0f - t) + end2 * t; 
        float angRad = angDeg * M_PI / 180.0f;

        float x = MAX_SEATING_X_RADIUS * cos(angRad);
        float z = MAX_SEATING_Z_RADIUS * sin(angRad);
        glVertex3f(x, facadeHeight, z);
        glVertex3f(x, -5.0f, z); 
    }
    glEnd();
    glPopMatrix();
}
void drawSafetyRailing() {
    glColor3f(0.9f, 0.9f, 0.9f); 
    float railingHeight = 2.0f;
    float rX = MAX_SEATING_X_RADIUS + 0.5f;
    float rZ = MAX_SEATING_Z_RADIUS + 0.5f;
    int segments = 100;

    glPushMatrix();
    glTranslatef(0.0f, STADIUM_TOTAL_HEIGHT + railingHeight, 0.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex3f(rX * cos(angle), 0.0f, rZ * sin(angle));
    }
    glEnd();
    glPopMatrix();
}

// NEW FUNCTION: Fills the entire inside of the track with grass
// This prevents empty space between the rectangular field and the oval track
void drawInnerGrass() {
    glColor3f(0.0f, 0.5f, 0.0f); // Grass Green
    glPushMatrix();
    glTranslatef(0.0f, 0.01f, 0.0f); // Just above ground

    int segments = 100;
    glBegin(GL_POLYGON); // Fill the oval
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex3f(TRACK_INNER_X_RADIUS * cos(angle), 0.0f, TRACK_INNER_Z_RADIUS * sin(angle));
    }
    glEnd();
    glPopMatrix();
}

void drawFootballPitch() {
    // We don't draw the green quad for the field anymore, because drawInnerGrass covers it.
    // We only draw the LINES on top.
    
    glPushMatrix();
    glTranslatef(0.0f, 0.04f, 0.0f); // Slightly above the grass

    // Lines
    glColor3f(1.0f, 1.0f, 1.0f); 
    glLineWidth(2.0f);
    
    // Outer boundary
    glBegin(GL_LINE_LOOP);
    glVertex3f(-FIELD_X_RADIUS, 0.0f, -FIELD_Z_RADIUS);
    glVertex3f( FIELD_X_RADIUS, 0.0f, -FIELD_Z_RADIUS);
    glVertex3f( FIELD_X_RADIUS, 0.0f,  FIELD_Z_RADIUS);
    glVertex3f(-FIELD_X_RADIUS, 0.0f,  FIELD_Z_RADIUS);
    glEnd();

    // Halfway line
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, FIELD_Z_RADIUS);
    glVertex3f(0.0f, 0.0f, -FIELD_Z_RADIUS);
    glEnd();

    // Center circle
    glBegin(GL_LINE_LOOP);
    float centerCircleRadius = 9.0f; 
    for (int i = 0; i < 50; ++i) {
        float angle = 2.0f * M_PI * i / 50.0f;
        glVertex3f(centerCircleRadius * cos(angle), 0.0f, centerCircleRadius * sin(angle));
    }
    glEnd();

    // Penalty Areas
    float boxDepth = 16.0f;
    float boxWidth = 30.0f; 

    // +X Box
    glBegin(GL_LINE_LOOP);
    glVertex3f(FIELD_X_RADIUS, 0.0f, -boxWidth/2);
    glVertex3f(FIELD_X_RADIUS - boxDepth, 0.0f, -boxWidth/2);
    glVertex3f(FIELD_X_RADIUS - boxDepth, 0.0f, boxWidth/2);
    glVertex3f(FIELD_X_RADIUS, 0.0f, boxWidth/2);
    glEnd();

    // -X Box
    glBegin(GL_LINE_LOOP);
    glVertex3f(-FIELD_X_RADIUS, 0.0f, -boxWidth/2);
    glVertex3f(-FIELD_X_RADIUS + boxDepth, 0.0f, -boxWidth/2);
    glVertex3f(-FIELD_X_RADIUS + boxDepth, 0.0f, boxWidth/2);
    glVertex3f(-FIELD_X_RADIUS, 0.0f, boxWidth/2);
    glEnd();

    glPopMatrix();
}

void drawAthleticsTrack() {
    glColor3f(0.8f, 0.2f, 0.1f); // Burnt orange track
    glPushMatrix();
    glTranslatef(0.0f, 0.02f, 0.0f); 

    int segments = 120;
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex3f(TRACK_OUTER_X_RADIUS * cos(angle), 0.0f, TRACK_OUTER_Z_RADIUS * sin(angle));
        glVertex3f(TRACK_INNER_X_RADIUS * cos(angle), 0.0f, TRACK_INNER_Z_RADIUS * sin(angle));
    }
    glEnd();
    
    // Track lines
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(1.0f);
    for(int lane=1; lane<4; lane++) {
        float rX = TRACK_INNER_X_RADIUS + (lane * (TRACK_WIDTH/4.0f));
        float rZ = TRACK_INNER_Z_RADIUS + (lane * (TRACK_WIDTH/4.0f));
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i <= segments; ++i) {
             float angle = 2.0f * M_PI * i / segments;
             glVertex3f(rX * cos(angle), 0.05f, rZ * sin(angle));
        }
        glEnd();
    }
    glPopMatrix();
}

void drawGoalNet(float width, float height, float depth) {
    glColor4f(0.9f, 0.9f, 0.9f, 0.3f); 
    glDisable(GL_LIGHTING); 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.0f);

    glBegin(GL_LINES);
    for (float x = -width / 2; x <= width / 2; x += 0.5f) {
        glVertex3f(x, 0.0f, depth); glVertex3f(x, height, depth);
        glVertex3f(x, height, 0.0f); glVertex3f(x, height, depth);
    }
    for (float y = 0.0f; y <= height; y += 0.5f) {
        glVertex3f(-width / 2, y, depth); glVertex3f(width / 2, y, depth);
        glVertex3f(-width / 2, y, 0.0f); glVertex3f(-width / 2, y, depth);
        glVertex3f(width / 2, y, 0.0f); glVertex3f(width / 2, y, depth);
    }
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING); 
}

void drawGoalposts() {
    glColor3f(1.0f, 1.0f, 1.0f); 
    float postR = 0.15f; 
    float postH = 2.44f; 
    float crossW = 7.32f; 
    
    // CHANGED: Make net depth negative to extend backwards
    float netD = -2.0f; 

    // Goal +X (Right side of screen, facing Left)
    glPushMatrix();
    glTranslatef(FIELD_X_RADIUS, 0.0f, 0.0f);
    glRotatef(-90.0f, 0.0f, 1.0f, 0.0f); 

    // Draw Posts
    glPushMatrix(); glTranslatef(-crossW/2, postH/2, 0.0f); glScalef(postR, postH, postR); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix(); glTranslatef(crossW/2, postH/2, 0.0f); glScalef(postR, postH, postR); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, postH, 0.0f); glScalef(crossW, postR, postR); glutSolidCube(1.0); glPopMatrix();
    
    // Draw Net
    glPushMatrix(); 
    // CHANGED: Translate to -postR (back of the post) instead of +postR (front)
    glTranslatef(0.0f, 0.0f, -postR); 
    drawGoalNet(crossW, postH, netD); 
    glPopMatrix();
    
    glPopMatrix();

    // Goal -X (Left side of screen, facing Right)
    glPushMatrix();
    glTranslatef(-FIELD_X_RADIUS, 0.0f, 0.0f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f); 

    // Draw Posts
    glPushMatrix(); glTranslatef(-crossW/2, postH/2, 0.0f); glScalef(postR, postH, postR); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix(); glTranslatef(crossW/2, postH/2, 0.0f); glScalef(postR, postH, postR); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, postH, 0.0f); glScalef(crossW, postR, postR); glutSolidCube(1.0); glPopMatrix();
    
    // Draw Net
    glPushMatrix(); 
    // CHANGED: Translate to -postR (back of the post) instead of +postR (front)
    glTranslatef(0.0f, 0.0f, -postR); 
    drawGoalNet(crossW, postH, netD); 
    glPopMatrix();
    
    glPopMatrix();
}
void drawTeamBenches() {
    // ADJUSTMENT: Moved further out (8.0f offset instead of 3.5f)
    // Field boundary is at 24.0f, Track starts at 38.0f. 
    // This places the benches around 32.0f, safely in the grass area.
    float benchZ = -FIELD_Z_RADIUS - 8.0f; 
    
    float benchWidth = 8.0f;
    float benchHeight = 2.2f;
    float benchDepth = 1.5f;

    // Define positions (Left and Right of the halfway line)
    float positions[2] = { -15.0f, 15.0f }; // Moved slightly further apart along X as well
    
    // Define Team Colors (Red for one, Blue for the other)
    float colors[2][3] = { {0.9f, 0.2f, 0.2f}, {0.2f, 0.2f, 0.9f} };

    for(int i = 0; i < 2; i++) {
        glPushMatrix();
        // Translate to position
        glTranslatef(positions[i], 0.0f, benchZ);
        
        // No rotation needed on this side (Negative Z). 
        // The benches are drawn facing Positive Z (towards the field center) by default.
        
        // --- Draw Structure (Dugout Shell) ---
        glColor3f(0.3f, 0.3f, 0.35f); // Dark Grey/Metal color

        // Roof
        glPushMatrix();
        glTranslatef(0.0f, benchHeight, 0.0f);
        glScalef(benchWidth, 0.1f, benchDepth);
        glutSolidCube(1.0);
        glPopMatrix();

        // Back Wall
        glPushMatrix();
        glTranslatef(0.0f, benchHeight / 2.0f, -benchDepth / 2.0f);
        glScalef(benchWidth, benchHeight, 0.1f);
        glutSolidCube(1.0);
        glPopMatrix();

        // Left Wall
        glPushMatrix();
        glTranslatef(-benchWidth / 2.0f, benchHeight / 2.0f, 0.0f);
        glScalef(0.1f, benchHeight, benchDepth);
        glutSolidCube(1.0);
        glPopMatrix();

        // Right Wall
        glPushMatrix();
        glTranslatef(benchWidth / 2.0f, benchHeight / 2.0f, 0.0f);
        glScalef(0.1f, benchHeight, benchDepth);
        glutSolidCube(1.0);
        glPopMatrix();

        // --- Draw Seats ---
        glColor3fv(colors[i]); // Set team color
        
        int numSeats = 6;
        float seatSpacing = (benchWidth - 0.5f) / numSeats;
        float startX = -(benchWidth / 2.0f) + (seatSpacing / 2.0f) + 0.25f;

        for(int s = 0; s < numSeats; s++) {
            float seatX = startX + (s * seatSpacing);
            
            // Seat Base
            glPushMatrix();
            glTranslatef(seatX, 0.4f, 0.0f);
            glScalef(0.8f, 0.1f, 0.8f);
            glutSolidCube(1.0);
            glPopMatrix();

            // Seat Back
            glPushMatrix();
            glTranslatef(seatX, 0.7f, -0.35f);
            glScalef(0.8f, 0.6f, 0.1f);
            glutSolidCube(1.0);
            glPopMatrix();
        }

        glPopMatrix();
    }
}
// **********************************************
// ************ CAMERA & SETUP ******************
// **********************************************

void computeCameraPosition() {
    angleY += deltaAngleY;
    angleX += deltaAngleX;
    if (angleX > 89.0f) angleX = 89.0f;
    if (angleX < 5.0f) angleX = 5.0f; 

    camDist += deltaMove;
    if (camDist < 20.0f) camDist = 20.0f;
    if (camDist > 300.0f) camDist = 300.0f; 

    float radY = angleY * M_PI / 180.0f;
    float radX = angleX * M_PI / 180.0f;

    cameraY = lookAtHeight + camDist * sin(radX);
    float distXZ = camDist * cos(radX);
    cameraX = distXZ * sin(radY);
    cameraZ = distXZ * cos(radY);
}

void pressKey(int key, int xx, int yy) {
    switch (key) {
        case GLUT_KEY_LEFT: deltaAngleY = -1.0f; break; 
        case GLUT_KEY_RIGHT: deltaAngleY = 1.0f; break;
        case GLUT_KEY_UP: deltaAngleX = -1.0f; break;
        case GLUT_KEY_DOWN: deltaAngleX = 1.0f; break;
        case GLUT_KEY_PAGE_UP: deltaMove = -2.0f; break; 
        case GLUT_KEY_PAGE_DOWN: deltaMove = 2.0f; break;
    }
}

void releaseKey(int key, int xx, int yy) {
    switch (key) {
        case GLUT_KEY_LEFT:
        case GLUT_KEY_RIGHT: deltaAngleY = 0.0f; break;
        case GLUT_KEY_UP:
        case GLUT_KEY_DOWN: deltaAngleX = 0.0f; break;
        case GLUT_KEY_PAGE_UP:
        case GLUT_KEY_PAGE_DOWN: deltaMove = 0.0f; break;
    }
}

void updateCamera(void) {
    if (deltaAngleY || deltaAngleX || deltaMove) {
        computeCameraPosition();
        glutPostRedisplay();
    }
}
void drawEntranceGates() {
    float gateWidth = 16.0f;
    float gateHeight = 12.0f;
    float gateDepth = 4.0f;
    
    // Position at the outer seating radius
    float rX = MAX_SEATING_X_RADIUS; 
    float rZ = MAX_SEATING_Z_RADIUS;

    float angles[2] = { 0.0f, 180.0f };
    std::string labels[2] = { "GATE A", "GATE B" };

    for(int i=0; i<2; i++) {
        glPushMatrix();
        
        float angleRad = angles[i] * M_PI / 180.0f;
        // Move slightly inward so the columns sit inside the gap
        float x = (rX - 2.0f) * cos(angleRad); 
        float z = (rZ - 2.0f) * sin(angleRad);
        
        glTranslatef(x, 0.0f, z);
        glRotatef(angles[i] + 90.0f, 0.0f, 1.0f, 0.0f); 

        // --- Draw Gate Frame ---
        glColor3f(0.5f, 0.5f, 0.55f); 
        
        // Left Pillar
        glPushMatrix();
        glTranslatef(-gateWidth/2 + 1.5f, gateHeight/2, 0.0f);
        glScalef(3.0f, gateHeight, gateDepth);
        glutSolidCube(1.0);
        glPopMatrix();

        // Right Pillar
        glPushMatrix();
        glTranslatef(gateWidth/2 - 1.5f, gateHeight/2, 0.0f);
        glScalef(3.0f, gateHeight, gateDepth);
        glutSolidCube(1.0);
        glPopMatrix();

        // Top Arch/Beam
        glPushMatrix();
        glTranslatef(0.0f, gateHeight - 1.5f, 0.0f);
        glScalef(gateWidth, 3.0f, gateDepth);
        glutSolidCube(1.0);
        glPopMatrix();
        
        // --- Gate Sign Board ---
        glColor3f(0.1f, 0.1f, 0.4f); 
        glPushMatrix();
        glTranslatef(0.0f, gateHeight + 2.0f, 0.0f);
        glScalef(gateWidth * 0.8f, 3.0f, 0.5f);
        glutSolidCube(1.0);
        glPopMatrix();

        // --- Gate Text ---
        glColor3f(1.0f, 1.0f, 0.0f); 
        glPushMatrix();
        glTranslatef(0.0f, gateHeight + 1.5f, 0.5f); 
        float textWidth = 0.0f;
       
        glScalef(0.025f, 0.025f, 0.025f);
        glTranslatef(-textWidth / 2.0f, 0.0f, 0.0f);
        glLineWidth(3.0f);
        
        glLineWidth(1.0f);
        glPopMatrix();

        // --- REMOVED BLACKOUT BOX HERE ---
        // The gate is now open air.

        // --- Ground Walkway (Extends Inside) ---
        // Draws a concrete path from outside, through the gate, onto the track edge
        glColor3f(0.55f, 0.55f, 0.6f);
        glPushMatrix();
        glTranslatef(0.0f, 0.05f, 5.0f); // 5.0f offset to center it through the gate
        // Length 30.0f ensures it covers the gap from outside to the track
        glScalef(gateWidth - 4.0f, 0.1f, 30.0f); 
        glutSolidCube(1.0);
        glPopMatrix();

        glPopMatrix();
    }
}
// Helper function to draw a single tower
void drawFloodlightTower(float x, float z, float angleRotation, int lightIndex) {
    float poleHeight = 65.0f; 
    float poleWidth = 2.5f;
    
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(angleRotation, 0.0f, 1.0f, 0.0f);

    // --- 1. The Mast (Pole) ---
    glColor3f(0.6f, 0.65f, 0.7f); // Steel Grey
    glPushMatrix();
    glTranslatef(0.0f, poleHeight / 2.0f, 0.0f);
    glScalef(poleWidth, poleHeight, poleWidth);
    glutSolidCube(1.0);
    glPopMatrix();

    // --- 2. The Light Head (Panel) ---
    float headWidth = 14.0f;
    float headHeight = 8.0f;
    float headDepth = 2.0f;

    glPushMatrix();
    glTranslatef(0.0f, poleHeight, 0.0f); 
    glRotatef(25.0f, 1.0f, 0.0f, 0.0f);   

    // Panel Backing
    glColor3f(0.2f, 0.2f, 0.25f); 
    glPushMatrix();
    glScalef(headWidth, headHeight, headDepth);
    glutSolidCube(1.0);
    glPopMatrix();

    // --- 3. The Bulbs (Glowing White) ---
    if (nightMode) { // Only make bulbs glow if it's night
        glDisable(GL_LIGHTING); 
        glColor3f(1.0f, 1.0f, 0.2f); // Pure White
        
        float startX = -headWidth / 2.0f + 1.5f;
        float startY = -headHeight / 2.0f + 1.5f;
        
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 3; j++) {
                glPushMatrix();
                glTranslatef(startX + (i * 3.5f), startY + (j * 2.5f), 1.1f); 
                glScalef(2.5f, 1.5f, 0.5f);
                glutSolidCube(1.0);
                glPopMatrix();
            }
        }
        glEnable(GL_LIGHTING); // Re-enable lighting
    } else { // If it's day, draw bulbs as white plastic, not glowing
        glColor3f(0.9f, 0.9f, 0.9f); // Just white plastic
        float startX = -headWidth / 2.0f + 1.5f;
        float startY = -headHeight / 2.0f + 1.5f;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 3; j++) {
                glPushMatrix();
                glTranslatef(startX + (i * 3.5f), startY + (j * 2.5f), 1.1f); 
                glScalef(2.5f, 1.5f, 0.5f);
                glutSolidCube(1.0);
                glPopMatrix();
            }
        }
    }
    glPopMatrix(); // End Head transformation

    // --- 4. ACTUAL OPENGL LIGHT SOURCE ---
    // Only enable these lights if nightMode is true
    if (nightMode) {
        GLfloat lightColor[] = { 0.6f, 0.6f, 0.6f, 1.0f }; // Dimmer spot light
        GLfloat lightPos[] = { x, poleHeight, z, 1.0f }; 
        
        glEnable(lightIndex);
        glLightfv(lightIndex, GL_DIFFUSE, lightColor);
        glLightfv(lightIndex, GL_SPECULAR, lightColor);
        glLightfv(lightIndex, GL_POSITION, lightPos);
        
        glLightf(lightIndex, GL_CONSTANT_ATTENUATION, 0.5f);
        glLightf(lightIndex, GL_LINEAR_ATTENUATION, 0.005f);
        glLightf(lightIndex, GL_QUADRATIC_ATTENUATION, 0.0f);
    } else {
        // If not night mode, disable the specific floodlights
        glDisable(lightIndex); 
    }

    glPopMatrix();
}

// The drawAllFloodlights function itself doesn't need significant changes,
// it just calls drawFloodlightTower which now handles the conditional logic.
void drawAllFloodlights() {
    float distX = 85.0f;
    float distZ = 65.0f;

    // 1. Corner +X, +Z (Top Right) -> Faces Center (-135 deg roughly)
    drawFloodlightTower(distX, distZ, 225.0f, GL_LIGHT1);

    // 2. Corner -X, +Z (Top Left) -> Faces Center (-45 deg roughly)
    drawFloodlightTower(-distX, distZ, 135.0f, GL_LIGHT2);

    // 3. Corner -X, -Z (Bottom Left) -> Faces Center (45 deg roughly)
    drawFloodlightTower(-distX, -distZ, 45.0f, GL_LIGHT3);

    // 4. Corner +X, -Z (Bottom Right) -> Faces Center (135 deg roughly)
    drawFloodlightTower(distX, -distZ, 315.0f, GL_LIGHT4);
}
void drawTree(float x, float z) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);

    // --- Trunk ---
    glColor3f(0.4f, 0.26f, 0.13f); // Wood Brown
    glPushMatrix();
    glTranslatef(0.0f, 2.0f, 0.0f); // Lift trunk center
    glScalef(1.5f, 4.0f, 1.5f);     // Tall and thin
    glutSolidCube(1.0);
    glPopMatrix();

    // --- Leaves (3 Tiered Cones) ---
    glColor3f(0.05f, 0.4f, 0.05f); // Dark Green

    // Bottom Tier
    glPushMatrix();
    glTranslatef(0.0f, 4.0f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); // Point up
    glutSolidCone(5.0, 6.0, 10, 2);      // Base, Height, Slices, Stacks
    glPopMatrix();

    // Middle Tier
    glPushMatrix();
    glTranslatef(0.0f, 6.5f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidCone(4.0, 5.5, 10, 2);
    glPopMatrix();

    // Top Tier
    glPushMatrix();
    glTranslatef(0.0f, 9.0f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidCone(2.5, 5.0, 10, 2);
    glPopMatrix();

    glPopMatrix();
}

void drawSurroundingTrees() {
    float treeRadiusX = 115.0f; // Wider than the stadium
    float treeRadiusZ = 95.0f;
    int numTrees = 10;

    for (int i = 0; i < numTrees; i++) {
        float theta = 2.0f * M_PI * i / numTrees;
        float angleDeg = theta * 180.0f / M_PI;
        
        // Normalize angle to 0-360
        while(angleDeg >= 360.0f) angleDeg -= 360.0f;
        while(angleDeg < 0.0f) angleDeg += 360.0f;

        // --- GAP LOGIC ---
        // Don't draw trees in front of Gate A (0 degrees) or Gate B (180 degrees)
        float gapSize = 15.0f;
        
        // Check Gate A (Around 0/360)
        if (angleDeg < gapSize || angleDeg > 360.0f - gapSize) continue;
        
        // Check Gate B (Around 180)
        if (angleDeg > 180.0f - gapSize && angleDeg < 180.0f + gapSize) continue;

        float x = treeRadiusX * cos(theta);
        float z = treeRadiusZ * sin(theta);
        
        // Add a little randomness to the position so they aren't in a perfect robot line
        // (Optional: simple offset based on index)
        float offset = (i % 2 == 0) ? 3.0f : -3.0f; 
        
        drawTree(x + offset, z + offset);
    }
}
void drawPlayer(float x, float z, bool isTeamRed, float rotation) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(rotation, 0.0f, 1.0f, 0.0f);

    // Dimensions
    float bodyWidth = 0.6f;
    float bodyHeight = 0.7f;
    float legHeight = 0.9f;
    float headRadius = 0.25f;

    // --- 1. Legs (White Shorts/Socks) ---
    glColor3f(1.0f, 1.0f, 1.0f); 
    
    // Left Leg
    glPushMatrix();
    glTranslatef(-0.15f, legHeight / 2.0f, 0.0f);
    glScalef(0.2f, legHeight, 0.2f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Right Leg
    glPushMatrix();
    glTranslatef(0.15f, legHeight / 2.0f, 0.0f);
    glScalef(0.2f, legHeight, 0.2f);
    glutSolidCube(1.0);
    glPopMatrix();

    // --- 2. Torso (Team Color Shirt) ---
    if (isTeamRed) glColor3f(0.9f, 0.1f, 0.1f); // Red Team
    else           glColor3f(0.1f, 0.1f, 0.9f); // Blue Team

    glPushMatrix();
    glTranslatef(0.0f, legHeight + (bodyHeight / 2.0f), 0.0f);
    glScalef(bodyWidth, bodyHeight, 0.3f);
    glutSolidCube(1.0);
    glPopMatrix();

    // --- 3. Arms (Skin Tone) ---
    glColor3f(0.87f, 0.72f, 0.53f); // Beige/Skin

    // Left Arm
    glPushMatrix();
    glTranslatef(-bodyWidth/2.0f - 0.1f, legHeight + bodyHeight - 0.2f, 0.0f);
    glScalef(0.15f, 0.5f, 0.15f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Right Arm
    glPushMatrix();
    glTranslatef(bodyWidth/2.0f + 0.1f, legHeight + bodyHeight - 0.2f, 0.0f);
    glScalef(0.15f, 0.5f, 0.15f);
    glutSolidCube(1.0);
    glPopMatrix();

    // --- 4. Head (Skin Tone) ---
    glPushMatrix();
    glTranslatef(0.0f, legHeight + bodyHeight + headRadius, 0.0f);
    glutSolidSphere(headRadius, 10, 10);
    glPopMatrix();

    glPopMatrix();
}

void drawFootball() {
    float ballRadius = 0.25f;
    
    glPushMatrix();
    // CHANGED: Use variables
    glTranslatef(ballX, ballRadius, ballZ); 
    
    // Rotate ball as it moves (visual effect)
    glRotatef(ballRot, 0.0f, 0.0f, -1.0f); 

    // Main White Ball
    glColor3f(1.0f, 1.0f, 1.0f);
    glutSolidSphere(ballRadius, 12, 12);
    
    // Shadow
    glDisable(GL_LIGHTING);
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix();
    // Shadow doesn't rotate with ball, stays on ground relative to ball center
    glRotatef(-ballRot, 0.0f, 0.0f, -1.0f); 
    glTranslatef(0.0f, -ballRadius + 0.02f, 0.0f);
    glScalef(1.0f, 0.01f, 1.0f);
    glutSolidSphere(ballRadius, 8, 8);
    glPopMatrix();
    glEnable(GL_LIGHTING);

    glPopMatrix();
}
void drawTeams() {
    // --- RED TEAM (Left Side, Facing Right) ---
    float rotRed = 90.0f;
    
    drawPlayer(-38.0f, 0.0f, true, rotRed);   // Goalkeeper
    drawPlayer(-25.0f, -10.0f, true, rotRed); // Defender
    drawPlayer(-25.0f, 10.0f, true, rotRed);  // Defender
    drawPlayer(-10.0f, -5.0f, true, rotRed);  // Midfielder
    drawPlayer(-5.0f, 15.0f, true, rotRed);   // Midfielder
    
    // CHANGED: The Striker now uses dynamic variables
    drawPlayer(strikerX, strikerZ, true, rotRed); 

    // --- BLUE TEAM (Right Side, Facing Left) ---
    float rotBlue = -90.0f;

    // CHANGED: The Goalie now uses dynamic variables (moves Z to dive)
    drawPlayer(38.0f, goalieZ, false, rotBlue);   
    
    drawPlayer(25.0f, -8.0f, false, rotBlue);  // Defender
    drawPlayer(25.0f, 8.0f, false, rotBlue);   // Defender
    drawPlayer(15.0f, 0.0f, false, rotBlue);   // Midfielder
    drawPlayer(8.0f, -15.0f, false, rotBlue);  // Midfielder
    drawPlayer(5.0f, 5.0f, false, rotBlue);    // Striker
}
void updateGameLogic() {
    if (!isPlaying) return;

    // STAGE 1: Striker Runs to Ball
    if (animStage == 1) {
        if (strikerX < -0.8f) {
            strikerX += 0.15f; // Run speed
        } else {
            // Reached ball, KICK!
            animStage = 2;
            ballVelX = 0.8f;  // Fast shot X
            ballVelZ = 0.25f; // Slight curve Z
        }
    }

    // STAGE 2: Ball Flies & Goalie Dives
    if (animStage == 2) {
        // Move Ball
        ballX += ballVelX;
        ballZ += ballVelZ;
        ballRot += 20.0f; // Spin

        // Move Goalie (Simple AI: Move towards ball Z)
        // Only if ball is getting close
        if (ballX > 20.0f) {
            if (goalieZ < ballZ) goalieZ += 0.15f;
            if (goalieZ > ballZ) goalieZ -= 0.15f;
        }

        // Friction / Stop condition
        if (ballX > 45.0f) { // Ball passed goal line
            ballVelX *= 0.95f; // Slow down
            ballVelZ *= 0.95f;
            if (ballVelX < 0.01f) isPlaying = false; // Stop
        }
    }
}

// Rename/Create this central idle function
void idle() {
    updateGameLogic(); // Move players/ball
    updateCamera();    // Move camera (if keys pressed)
    glutPostRedisplay();
}
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    gluLookAt(cameraX, cameraY, cameraZ,
              targetX, targetY, targetZ,
              0.0f, 1.0f, 0.0f);

    // Draw all components
    drawInnerGrass();     // NEW: Fills the gap between track and field
    drawAthleticsTrack(); // Red track ring
    drawFootballPitch();  // White lines on top of grass
    drawGoalposts();
    drawTeamBenches();
    drawEntranceGates();
    drawAllFloodlights();
	drawSurroundingTrees();
	drawFootball();
    drawTeams(); 
    drawStadiumSeatingBowl();
    drawMainGrandstandRoof();
    drawMainGrandstandColumns();
    drawGrandstandFacade();
    drawVIPSeating();
    drawStadiumName();
    drawStoneFacade();
    drawSafetyRailing();

    glutSwapBuffers();
}

void reshape(int w, int h) {
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w / (double)h, 1.0, 500.0);
    glMatrixMode(GL_MODELVIEW);
}

void init() {
    // Set background based on night mode
    if (nightMode) {
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f); // Dark sky for night
    } else {
        glClearColor(0.6f, 0.8f, 1.0f, 1.0f); // Bright sky for day
    }
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0); // The "sun" light
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE); 

    GLfloat light_pos[] = {100.0f, 200.0f, 100.0f, 1.0f}; 
    GLfloat white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat ambient[] = {0.4f, 0.4f, 0.4f, 1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
    glLightfv(GL_LIGHT0, GL_SPECULAR, white);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

    quadric = gluNewQuadric();
    gluQuadricDrawStyle(quadric, GLU_FILL);
    gluQuadricNormals(quadric, GLU_SMOOTH);

    computeCameraPosition();
}
void toggleNightMode() {
    nightMode = !nightMode; // Flip the state

    // Update background color immediately
    if (nightMode) {
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f); 
    } else {
        glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
    }
    
    glutPostRedisplay(); // Force a redraw to show background change
}
void keyboardHandler(unsigned char key, int x, int y) {
    if (key == 'n' || key == 'N') {
        toggleNightMode();
    }
    
    // --- NEW: Press R to Start ---
    if (key == 'r' || key == 'R') {
        isPlaying = true;
        animStage = 1; // Start Running
        
        // Reset Positions
        ballX = 0.0f; ballZ = 0.0f; ballRot = 0.0f;
        strikerX = -6.0f; strikerZ = 0.0f;
        goalieZ = 0.0f;
        ballVelX = 0.0f; ballVelZ = 0.0f;
    }
}
// R
int main(int argc, char** argv) {
    // 1. Initialize GLUT (MUST BE FIRST)
    glutInit(&argc, argv);
    
    // 2. Configure Display Mode
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    
    // 3. Create the Window (This creates the OpenGL context)
    glutCreateWindow("Astu Stadium");

    // 4. Initialize your settings (Lighting, Materials, etc.)
    init();

    // 5. Register Callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(pressKey);
    glutSpecialUpFunc(releaseKey);
    glutIdleFunc(idle);            // Ensure 'idle' is defined (combines game+camera logic)
    glutKeyboardFunc(keyboardHandler);

    // 6. Enter Main Loop
    glutMainLoop();
    
    return 0;
}
