/*
 *  Lorenz Attractor Visualization
 */
#include "CSCIx229.h"
#ifdef USEGLEW
#include <GL/glew.h>
#endif
#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
//  Default resolution
//  For Retina displays compile with -DRES=2
#ifndef RES
#define RES 1
#endif

//-----------------------------------------------------------
// Struct declarations
//-----------------------------------------------------------
// Define struct for RGB color value
typedef struct Color
{
   float r;
   float g;
   float b;
} Color;

typedef struct Point
{
   double x;
   double y;
   double z;
} Point;

typedef struct Angle
{
   double psi; // Rotation about the X axis in degrees
   double ph;  // Rotation about the Y axis in degrees
   double th;  // Rotation about the Z axis in degrees
} Angle;

// Define struct for view parameters
typedef struct ViewParams
{
   Point pos;
   Angle angle;
} ViewParams;

typedef struct Cylinder
{
   Point base;  // Base position of cylinder
   Angle angle; // Rotation angles
   double r;    // Radius of cylinder
   double h;    // Height of cylinder
} Cylinder;

typedef struct Torus
{
   Point center;  // Center position of torus
   Point axis;    // Axis vector of torus
   double rMajor; // Major radius (distance from center to tube center)
   double rMinor; // Minor radius (radius of the tube)
} Torus;

typedef struct EllipseStruct
{
   Point center;  // Center position of ellipse
   Point axis;    // Axis vector of ellipse
   double rMajor; // Major radius
   double rMinor; // Minor radius
} EllipseStruct;

double Tan(double theta)
{
   return tan(theta * 3.1415926535 / 180.0);
}

//-----------------------------------------------------------
// Global variables
//-----------------------------------------------------------

double asp = 16.0 / 9.0; // Aspect ratio
int fov = 110;           // Field of view for perspective
int m = 0;               // perspective mode switcher
double dim = 5.0;        // Size of the world
int ph = 20;             // Elevation of view angle
int th = 0;              // Azimuth of view angle
double Ex = 0.0;         // First person camera x position
double Ey = 1.0;         // first person camera y position
double Ez = -1.0;        // first person camera z position

// Flags
int axes = 1;  // Display axes or not
int light = 1; // Lighting on or off
int moveLight = 1; // Move light in idle or not

// Light values
int one = 1;       // Unit value
int distance = 5;  // Light distance
int inc = 10;      // Ball increment
int smooth = 1;    // Smooth/Flat shading
int local = 0;     // Local Viewer Model
int emission = 0;  // Emission intensity (%)
int ambient = 20;  // Ambient intensity (%)
int diffuse = 50;  // Diffuse intensity (%)
int specular = 50;  // Specular intensity (%)
int zh = 90;       // Light azimuth
float ylight = 0;  // Elevation of light

/*
 *  Check for OpenGL errors
 */
void ErrCheck(const char *where)
{
   int err = glGetError();
   if (err)
      fprintf(stderr, "ERROR: %s [%s]\n", gluErrorString(err), where);
}

/*
 *  Print message to stderr and exit
 */
void Fatal(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   vfprintf(stderr, format, args);
   va_end(args);
   exit(1);
}

/*
 *  Convenience routine to output raster text
 *  Use VARARGS to make this more flexible
 */
#define LEN 8192 //  Maximum length of text string
void Print(const char *format, ...)
{
   char buf[LEN];
   char *ch = buf;
   va_list args;
   //  Turn the parameters into a character string
   va_start(args, format);
   vsnprintf(buf, LEN, format, args);
   va_end(args);
   //  Display the characters one at a time at the current raster position
   while (*ch)
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *ch++);
}

// Compute angles for aligning the khat vector with a given direction vector
Angle computeAngles(Point dir)
{
   Angle angles;
   // Compute the angles
   angles.ph = acos(dir.z / sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z)) * 180.0 / 3.1415926535;
   angles.th = atan2(dir.y, dir.x) * 180.0 / 3.1415926535;
   angles.psi = 0.0; // No roll angle needed for this application
   return angles;
}

// Lets you specify the center of the two end points of the cylinder and draws it with the associated radius
// Enhanced version with global coordinate coloring
void drawCylinder(Point p1, Point p2, double r)
{
   // Compute the direction vector from p1 to p2
   Point dir = {
       p2.x - p1.x,
       p2.y - p1.y,
       p2.z - p1.z,
   };
   // Compute the length of the cylinder
   double length = sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);

   // Compute the angles for aligning the cylinder with the direction vector
   Angle angles = computeAngles(dir);

   // Save current transformation matrix
   glPushMatrix();

   // Set the origin to be the base of the cylinder
   glTranslated(p1.x, p1.y, p1.z);

   // Apply rotations
   glRotated(angles.th, 0.0, 0.0, 1.0);  // Rotate about Z axis
   glRotated(angles.ph, 0.0, 1.0, 0.0);  // Rotate about Y axis
   glRotated(angles.psi, 1.0, 0.0, 0.0); // Rotate about X axis

   // Body of the cylinder
   const int deltaDegree = 15; // degrees per segment
   glBegin(GL_QUAD_STRIP);
   for (int degree = 0; degree <= 360; degree += deltaDegree)
   {
      double x = r * Cos(degree);
      double y = r * Sin(degree);

      // Bottom vertex - compute global position
      glNormal3d(Cos(degree), Sin(degree), 0.0); // Normal points outwards
      glVertex3d(x, y, 0.0);

      // Top vertex - compute global position
      glNormal3d(Cos(degree), Sin(degree), 0.0); // Normal points outwards
      glVertex3d(x, y, length);
   }
   glEnd();

   // Top circle
   glBegin(GL_TRIANGLE_FAN);
   // Center vertex
   glNormal3d(0.0, 0.0, 1.0); // Normal points forward
   glVertex3d(0.0, 0.0, length);

   for (int degree = 0; degree <= 360; degree += deltaDegree)
   {
      double x = r * Cos(degree);
      double y = r * Sin(degree);
      glNormal3d(0.0, 0.0, 1.0); // Normal points forward
      glVertex3d(x, y, length);
   }
   glEnd();

   // Bottom circle
   glBegin(GL_TRIANGLE_FAN);
   // Center vertex
   glNormal3d(0.0, 0.0, -1.0); // Normal points backward
   glVertex3d(0.0, 0.0, 0.0);

   for (int degree = 0; degree <= 360; degree += deltaDegree)
   {
      double x = r * Cos(degree);
      double y = r * Sin(degree);
      glNormal3d(0.0, 0.0, -1.0); // Normal points backward
      glVertex3d(x, y, 0.0);
   }
   glEnd();

   // Restore transformation matrix
   glPopMatrix();
}

void drawTorus(Torus t)
{
   // Save current transformation matrix
   glPushMatrix();

   // Set the origin to be the center of the torus
   glTranslated(t.center.x, t.center.y, t.center.z);

   // Compute the angles for aligning the torus with the axis vector
   Angle angles = computeAngles(t.axis);

   // Apply rotations
   glRotated(angles.th, 0.0, 0.0, 1.0);  // Rotate about Z axis
   glRotated(angles.ph, 0.0, 1.0, 0.0);  // Rotate about Y axis
   glRotated(angles.psi, 1.0, 0.0, 0.0); // Rotate about X axis

   // Draw torus using quad strips
   double deltaDegree = 15; // degrees per segment
   for (double theta = 0; theta <= 360; theta += deltaDegree)
   {
      glBegin(GL_QUAD_STRIP);

      for (double phi = 0; phi <= 360; phi += deltaDegree)
      {
         // Equation for torus
         double x1 = (t.rMajor + t.rMinor * Cos(theta)) * Cos(phi);
         double y1 = (t.rMajor + t.rMinor * Cos(theta)) * Sin(phi);
         double z1 = t.rMinor * Sin(theta);
         glNormal3d(Cos(theta) * Cos(phi), Cos(theta) * Sin(phi), Sin(theta));
         glVertex3d(x1, y1, z1);

         double x2 = (t.rMajor + t.rMinor * Cos(theta + deltaDegree)) * Cos(phi + deltaDegree);
         double y2 = (t.rMajor + t.rMinor * Cos(theta + deltaDegree)) * Sin(phi + deltaDegree);
         double z2 = t.rMinor * Sin(theta + deltaDegree);
         glNormal3d(Cos(theta + deltaDegree) * Cos(phi + deltaDegree),
                    Cos(theta + deltaDegree) * Sin(phi + deltaDegree),
                    Sin(theta + deltaDegree));
         glVertex3d(x2, y2, z2);
      }
      glEnd();
   }

   // Restore transformation matrix to whatever it was before we drew the torus
   glPopMatrix();
}

void drawEllipse(EllipseStruct e)
{
   // Save current transformation matrix
   glPushMatrix();

   // Set the origin to be the center of the ellipse
   glTranslated(e.center.x, e.center.y, e.center.z);

   // Compute the angles for aligning the ellipse with the axis vector
   Angle angles = computeAngles(e.axis);

   // Apply rotations
   glRotated(angles.th, 0.0, 0.0, 1.0);  // Rotate about Z axis
   glRotated(angles.ph, 0.0, 1.0, 0.0);  // Rotate about Y axis
   glRotated(angles.psi, 1.0, 0.0, 0.0); // Rotate about X axis

   // Scale by the major and minor axes
   glScaled(e.rMinor, 1.0 * e.rMajor, e.rMajor);

   //  Latitude bands
   double deltaDegree = 15; // degrees per segment
   for (int ph = -90; ph < 90; ph += deltaDegree)
   {
      glBegin(GL_QUAD_STRIP);
      for (int th = 0; th <= 360; th += deltaDegree)
      {
         double x1 = Sin(th) * Cos(ph);
         double y1 = Sin(ph);
         double z1 = Cos(th) * Cos(ph);

         double x2 = Sin(th) * Cos(ph + deltaDegree);
         double y2 = Sin(ph + deltaDegree);
         double z2 = Cos(th) * Cos(ph + deltaDegree);

         glNormal3d(x1, y1, z1);
         glVertex3d(x1, y1, z1);

         glNormal3d(x2, y2, z2);
         glVertex3d(x2, y2, z2);
      }
      glEnd();
   }

   // Restore transformation matrix to whatever it was before we drew the ellipse
   glPopMatrix();
}

void drawBicycle(Point origin, Point direction, Point scale)
{
   // Reference angle for forward direction
   Angle forward = computeAngles(direction);

   // Bike parameters for Specialized S-Works Diverge
   // Sourced: https://geometrygeeks.bike/compare/specialized-diverge-s-works-2021-54,cannondale-topstone-carbon-2020-md,3t-cycling-exploro-2020-m/
   // All dimensions in m and degrees
   double seatAngle = 74.0;
   double headAngle = 70.0;
   double r = 0.0254;
   // double topTubeLength = 0.358 / Sin(90.0 - seatAngle);
   double topTubeEff = 0.529;
   double headTubeLength = 0.116;
   double headTubeTopLength = 0.086; // Not specified in source, just a guess
   double chainStayLength = 0.425;
   double handleBarLength = 0.580; // Not specified in source, just a guess
   double seatTubeCC = 0.460;
   double seatTubeLength = 0.120; // Not specified in source, just a guess
   double wheelBase = 1.019;
   double axleWidth = 0.300;
   double wheelRadius = 0.311;

   // Build the bicycle geometry relative to (0,0,0), with seatpost at origin
   Point seatPost = {0.0, 0.0, 0.0};
   Point midHeadTube = {seatPost.x,
                        seatPost.y + topTubeEff * Tan(90.0 - headAngle),
                        seatPost.z + topTubeEff};

   Point headTubeBottom = {midHeadTube.x,
                           midHeadTube.y + headTubeLength * Cos(90.0 + headAngle),
                           midHeadTube.z + headTubeLength * Sin(90.0 + headAngle)};

   Point headTubeTop = {midHeadTube.x,
                        midHeadTube.y - headTubeTopLength * Cos(90.0 + headAngle),
                        midHeadTube.z - headTubeTopLength * Sin(90.0 + headAngle)};

   Point seatTubeBottom = {seatPost.x,
                           seatPost.y + seatTubeCC * Cos(90.0 + seatAngle),
                           seatPost.z + seatTubeCC * Sin(90.0 + seatAngle)};

   Point seatTubeTop = {seatPost.x,
                        seatPost.y - seatTubeLength * Cos(90.0 + seatAngle),
                        seatPost.z - seatTubeLength * Sin(90.0 + seatAngle)};

   Point rearAxle = {seatTubeBottom.x,
                     seatTubeBottom.y - chainStayLength * Cos(104.0),
                     seatTubeBottom.z - chainStayLength * Sin(104.0)};

   Point frontAxle = {rearAxle.x, rearAxle.y, rearAxle.z + wheelBase};
   Point frontAxleLeft = {frontAxle.x - axleWidth / 2.0, frontAxle.y, frontAxle.z};
   Point frontAxleRight = {frontAxle.x + axleWidth / 2.0, frontAxle.y, frontAxle.z};
   Point rearAxleLeft = {rearAxle.x - axleWidth / 2.0, rearAxle.y, rearAxle.z};
   Point rearAxleRight = {rearAxle.x + axleWidth / 2.0, rearAxle.y, rearAxle.z};

   Point handlebarLeft = {headTubeTop.x - handleBarLength / 2.0, headTubeTop.y, headTubeTop.z};
   Point handlebarRight = {headTubeTop.x + handleBarLength / 2.0, headTubeTop.y, headTubeTop.z};
   Point gripLeft = {handlebarLeft.x + 0.4*(handleBarLength / 2.0), handlebarLeft.y, handlebarLeft.z};
   Point gripRight = {handlebarRight.x - 0.4*(handleBarLength / 2.0), handlebarRight.y, handlebarRight.z};
   Point handleBarEndLeft = handlebarLeft;
   handleBarEndLeft.x -= 0.1;
   Point handleBarEndRight = handlebarRight;
   handleBarEndRight.x += 0.1;

   //  Colors for materials and light properties
   float white[] = {1.0, 1.0, 1.0, 1.0};
   float red[] = {1.0, 0.0, 0.0, 1.0};
   float lightGrey[] = {0.7882352941176471, 0.7882352941176471, 0.7882352941176471, 1.0};
   float darkGrey[] = {0.392156862745098, 0.392156862745098, 0.392156862745098, 1.0};
   float silver[] = {0.8196078431372549, 0.8196078431372549, 0.8196078431372549, 1.0};
   float black[] = {0.0, 0.0, 0.0, 1.0};

   // Apply rotation
   glPushMatrix();
   glTranslated(origin.x, origin.y, origin.z);
   glRotated(forward.th, 0.0, 0.0, 1.0);  // Rotate about Z axis
   glRotated(forward.ph, 0.0, 1.0, 0.0);  // Rotate about Y axis
   glRotated(forward.psi, 1.0, 0.0, 0.0); // Rotate about X axis
   glScaled(scale.x, scale.y, scale.z);   // Scale to desired size

   // Grey color
   glColor4f(lightGrey[0], lightGrey[1], lightGrey[2], lightGrey[3]);
   glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 64.0);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, lightGrey);
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, lightGrey);
   drawCylinder(seatPost, seatTubeTop, r);         // Actual seat post

   // Chrome silver
   glColor4f(silver[0], silver[1], silver[2], silver[3]);
   glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 128.0);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, black);
   drawCylinder(rearAxleLeft, rearAxleRight, r);   // Rear axle
   drawCylinder(frontAxleLeft, frontAxleRight, r); // Front axle

   // Chrome red (for speed)
   glColor4f(red[0], red[1], red[2], red[3]);
   glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 128.0);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, red);
   drawCylinder(headTubeBottom, headTubeTop, r);    // Head tube
   drawCylinder(seatPost, midHeadTube, r);          // Top tube
   drawCylinder(seatTubeBottom, headTubeBottom, r); // Down tube? No name on the diagram
   drawCylinder(seatPost, rearAxleRight, r);        // Chain stay right
   drawCylinder(seatPost, seatTubeBottom, r);       // Seat tube
   drawCylinder(seatTubeBottom, rearAxleRight, r);  // Seat stay right
   drawCylinder(seatPost, rearAxleLeft, r);         // Chain stay left
   drawCylinder(seatTubeBottom, rearAxleLeft, r);   // Seat stay left

   drawCylinder(headTubeBottom, frontAxleRight, r); // Right fork
   drawCylinder(headTubeBottom, frontAxleLeft, r);  // Left fork

   // Darker grey - not as shiny
   glColor4f(darkGrey[0], darkGrey[1], darkGrey[2], darkGrey[3]);
   glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 1.0);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, darkGrey);
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, darkGrey);
   drawCylinder(handlebarLeft, handlebarRight, r); // Handlebars


   // Draw seat
   EllipseStruct seat = {seatTubeTop, midHeadTube, 0.1, 0.05};
   glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 4.0);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, lightGrey);
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, lightGrey);
   drawEllipse(seat);

   // Draw wheels - black rubber
   glColor4f(black[0], black[1], black[2], black[3]);
   glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 0.0);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, darkGrey);
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, black);
   Torus frontWheel = {(Point){frontAxle.x, frontAxle.y, frontAxle.z}, (Point){1.0, 0.0, 0.0}, wheelRadius, 0.0254};
   drawTorus(frontWheel);
   Torus rearWheel = {(Point){rearAxle.x, rearAxle.y, rearAxle.z}, (Point){1.0, 0.0, 0.0}, wheelRadius, 0.0254};
   drawTorus(rearWheel);

   // Draw handlebar grips - black rubber
   drawCylinder(gripLeft, handleBarEndLeft, 1.1*r);
   drawCylinder(gripRight, handleBarEndRight, 1.1*r);

   glPopMatrix();
}

void display()
{
   // Set background color to light blue
   glClearColor(32.0 / 255.0, 72.0 / 255.0, 87.0 / 255.0, 1.0);

   // Clear the image
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   // Reset transformations
   glLoadIdentity();

   // Set the eye position
   switch (m)
   {
   case 0:
      // Orthogonal
      glRotated(ph, 1.0, 0.0, 0.0);
      glRotated(th, 0.0, 1.0, 0.0);
      break;
   case 1:
      // Perspective
      gluLookAt(Ex, Ey, Ez, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
      glRotated(ph, 1.0, 0.0, 0.0);
      glRotated(th, 0.0, 1.0, 0.0);
      break;
   default:
      Fatal("Invalid mode %d\n", m);
   }

   //  Flat or smooth shading
   glShadeModel(smooth ? GL_SMOOTH : GL_FLAT);

   //  Light switch
   if (light)
   {
      //  Translate intensity to color vectors
      float Ambient[] = {0.01 * ambient, 0.01 * ambient, 0.01 * ambient, 1.0};
      float Diffuse[] = {0.01 * diffuse, 0.01 * diffuse, 0.01 * diffuse, 1.0};
      float Specular[] = {0.01 * specular, 0.01 * specular, 0.01 * specular, 1.0};
      //  Light position
      float Position[] = {distance * Cos(zh), ylight, distance * Sin(zh), 1.0};
      //  Draw light position as sphere (still no lighting here)
      glColor3f(1, 1, 1);
      EllipseStruct lightSphere = {(Point){Position[0], Position[1], Position[2]}, (Point){0.0, 1.0, 0.0}, 0.1, 0.1};
      drawEllipse(lightSphere);
      //  OpenGL should normalize normal vectors
      glEnable(GL_NORMALIZE);
      //  Enable lighting
      glEnable(GL_LIGHTING);
      //  Location of viewer for specular calculations
      // glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,local);
      //  glColor sets ambient and diffuse color materials
      glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
      glEnable(GL_COLOR_MATERIAL);
      //  Enable light 0
      glEnable(GL_LIGHT0);
      //  Set ambient, diffuse, specular components and position of light 0
      glLightfv(GL_LIGHT0, GL_AMBIENT, Ambient);
      glLightfv(GL_LIGHT0, GL_DIFFUSE, Diffuse);
      glLightfv(GL_LIGHT0, GL_SPECULAR, Specular);
      glLightfv(GL_LIGHT0, GL_POSITION, Position);
   }
   else
      glDisable(GL_LIGHTING);

   // Set color to red for bike
   glColor3f(1.0, 0.0, 0.0);

   drawBicycle((Point){0.0, 0.0, 0.0}, (Point){0.0, 0.0, 1.0}, (Point){1.0, 1.0, 1.0});

   glDisable(GL_LIGHTING); // No lighting for axes and text
   glColor3f(1, 1, 1);     // white
   if (axes)
   {
      //  Draw axes in white
      glBegin(GL_LINES);
      glVertex3d(0, 0, 0);
      glVertex3d(1, 0, 0);
      glVertex3d(0, 0, 0);
      glVertex3d(0, 1, 0);
      glVertex3d(0, 0, 0);
      glVertex3d(0, 0, 1);
      glEnd();
      //  Label axes
      glRasterPos3d(1, 0, 0);
      Print("X");
      glRasterPos3d(0, 1, 0);
      Print("Y");
      glRasterPos3d(0, 0, 1);
      Print("Z");
   }

   //  Display parameters

   glWindowPos2i(5, 5);
   Print("Angle=%d,%d  Dim=%.1f FOV=%d Projection=%s Light=%s",
         th, ph, dim, fov, m == 1 ? "Perspective" : "Orthogonal", light ? "On" : "Off");
   if (light)
   {
      glWindowPos2i(5, 45);
      Print("Model=%s LocalViewer=%s Distance=%d Elevation=%.1f", smooth ? "Smooth" : "Flat", local ? "On" : "Off", distance, ylight);
      glWindowPos2i(5, 25);
      Print("Ambient=%d  Diffuse=%d Specular=%d Emission=%d", ambient, diffuse, specular, emission);
   }

   // Error check
   ErrCheck("display");

   // Flush and swap buffer
   glFlush();
   glutSwapBuffers();
}

/*
 * This function is called by GLUT when the window is resized
 */
void reshape(int width, int height)
{
   // Avoid divide by zero
   asp = (height > 0) ? (double)width / height : 1;

   glViewport(0, 0, width, height);

   // Apply projection based on the current mode
   if (m == 0)
      Project(0, asp, dim);
   else
      Project(fov, asp, dim);
}

void key(unsigned char ch, int x, int y)
{
   if (ch == 27) // Escape key
   {
      exit(0);
   }
   else if (ch == 'l' || ch == 'L')
   {
      light = 1 - light;
   }
   else if (ch == 'x' || ch == 'X')
   {
      axes = 1 - axes;
   }
   else if( ch == 'n' || ch == 'N')
   {
      moveLight = 1 - moveLight;
   }
   else if( ch == 'm' || ch == 'M')
   {
      m = 1 - m;
   }
   else if( !moveLight && (ch == 'W' || ch == 'w'))
   {
      ylight += 0.1;
   }
   else if( !moveLight && (ch == 'S' || ch == 's'))
   {
      ylight -= 0.1;
   }
   else if( !moveLight && (ch == 'a' || ch == 'A'))
   {
      zh = fmod(zh + 5, 360.0);
   }
   else if( !moveLight && (ch == 'D' || ch == 'd'))
   {
      zh = fmod(zh - 5, 360.0);
   }

   if (m == 0)
   {
      Project(0, asp, dim);
   }
   else
   {
      Project(fov, asp, dim);
   }
   //  Request display update
   glutPostRedisplay();
}

/*
 *  Functionality to move the camera position
 */
void special(int key, int x, int y)
{
   // These seem backwards from the code perspective but make more sense when controlling the camera
   if (key == GLUT_KEY_RIGHT)
   {
      th -= 5;
   }
   else if (key == GLUT_KEY_LEFT) // Look left (yaw left)
   {
      th += 5;
   }
   else if (key == GLUT_KEY_UP) // Look up (pitch up)
   {
      ph += 5;
   }
   else if (key == GLUT_KEY_DOWN) // Look down (pitch down)
   {
      ph -= 5;
   }
   //  Smooth color model
   else if (key == GLUT_KEY_F1)
   {
      smooth = 1 - smooth;
   }

   if (m == 0)
      Project(0, asp, dim);
   else
      Project(fov, asp, dim);

   //  Request display update
   glutPostRedisplay();
}

// Function for basic animations
void idle()
{
   if( moveLight )
   {   
   // Rotate light around the origin
   zh = fmod(zh + 1, 360.0);

   // Oscillate the light height
   ylight = 2.0 * Sin((double)2*zh);

   glutPostRedisplay();
   }
}

// Main
int main(int argc, char *argv[])
{
   //  Initialize GLUT
   glutInit(&argc, argv);
   //  Request double buffered true color window without Z-buffer
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   //  Create window
   glutCreateWindow("Brendan Chong - Bicycle with lighting");
#ifdef USEGLEW
   //  Initialize GLEW
   if (glewInit() != GLEW_OK)
      Fatal("Error initializing GLEW\n");
#endif
   //  Register display, reshape, idle and key callbacks
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutKeyboardFunc(key);
   glutSpecialFunc(special);
   glutIdleFunc(idle);
   //  Enable Z-buffer depth test
   glEnable(GL_DEPTH_TEST);
   //  Pass control to GLUT for events
   glutMainLoop();
   return 0;
}