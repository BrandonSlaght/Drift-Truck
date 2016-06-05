#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
// Needed on MsWindows
#include <windows.h>
#endif // Win32 platform

#include <GL/gl.h>
#include <GL/glu.h>
// Download glut from: http://www.opengl.org/resources/libraries/glut/
#include <GL/glut.h>

#include "float2.h"
#include "float3.h"
#include "Mesh.h"
#include "stb_image.h"
#include <vector>
#include <map>
#include <algorithm>

float3 GRAVITY(0, -9.81, 0);
int window_id;
std::vector<bool> keysPressed;

void addParticles(float3);

class LightSource
{
public:
	virtual float3 getRadianceAt(float3 x) = 0;
	virtual float3 getLightDirAt(float3 x) = 0;
	virtual float  getDistanceFrom(float3 x) = 0;
	virtual void   apply(GLenum openglLightName) = 0;
};

class DirectionalLight : public LightSource
{
	float3 dir;
	float3 radiance;
public:
	DirectionalLight(float3 dir, float3 radiance)
		:dir(dir), radiance(radiance) {}
	float3 getRadianceAt(float3 x) { return radiance; }
	float3 getLightDirAt(float3 x) { return dir; }
	float  getDistanceFrom(float3 x) { return 900000000; }
	void   apply(GLenum openglLightName)
	{
		float aglPos[] = { dir.x, dir.y, dir.z, 0.0f };
		glLightfv(openglLightName, GL_POSITION, aglPos);
		float aglZero[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glLightfv(openglLightName, GL_AMBIENT, aglZero);
		float aglIntensity[] = { radiance.x, radiance.y, radiance.z, 1.0f };
		glLightfv(openglLightName, GL_DIFFUSE, aglIntensity);
		glLightfv(openglLightName, GL_SPECULAR, aglIntensity);
		glLightf(openglLightName, GL_CONSTANT_ATTENUATION, 1.0f);
		glLightf(openglLightName, GL_LINEAR_ATTENUATION, 0.0f);
		glLightf(openglLightName, GL_QUADRATIC_ATTENUATION, 0.0f);
	}
};

class PointLight : public LightSource
{
	float3 pos;
	float3 power;
public:
	PointLight(float3 pos, float3 power)
		:pos(pos), power(power) {}
	float3 getRadianceAt(float3 x) { return power*(1 / (x - pos).norm2() * 4 * 3.14); }
	float3 getLightDirAt(float3 x) { return (pos - x).normalize(); }
	float  getDistanceFrom(float3 x) { return (pos - x).norm(); }
	void   apply(GLenum openglLightName)
	{
		float aglPos[] = { pos.x, pos.y, pos.z, 1.0f };
		glLightfv(openglLightName, GL_POSITION, aglPos);
		float aglZero[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glLightfv(openglLightName, GL_AMBIENT, aglZero);
		float aglIntensity[] = { power.x, power.y, power.z, 1.0f };
		glLightfv(openglLightName, GL_DIFFUSE, aglIntensity);
		glLightfv(openglLightName, GL_SPECULAR, aglIntensity);
		glLightf(openglLightName, GL_CONSTANT_ATTENUATION, 0.0f);
		glLightf(openglLightName, GL_LINEAR_ATTENUATION, 0.0f);
		glLightf(openglLightName, GL_QUADRATIC_ATTENUATION, 0.25f / 3.14f);
	}
};

class Material
{
public:
	float3 kd;			// diffuse reflection coefficient
	float3 ks;			// specular reflection coefficient
	float shininess;	// specular exponent
	Material()
	{
		kd = float3(0.5, 0.5, 0.5) + float3::random() * 0.5;
		ks = float3(1, 1, 1);
		shininess = 15;
	}
	virtual void apply()
	{
		glDisable(GL_TEXTURE_2D);
		float aglDiffuse[] = { kd.x, kd.y, kd.z, 1.0f };
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, aglDiffuse);
		float aglSpecular[] = { kd.x, kd.y, kd.z, 1.0f };
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, aglSpecular);
		if (shininess <= 128)
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
		else
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 128.0f);
	}
};

class TexturedMaterial : public Material
{
public:
	GLuint id;
	GLint filtering;
	TexturedMaterial(const char* filename, GLint filtering = GL_LINEAR_MIPMAP_LINEAR) {
		unsigned char* data;
		int width;
		int height;
		int nComponents = 4;

		data = stbi_load(filename, &width, &height, &nComponents, 0);

		if (data == NULL) return;

		// opengl texture creation comes here
		glGenTextures(1, &id);  // id generation
		glBindTexture(GL_TEXTURE_2D, id);      // binding

		if (nComponents == 4)
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
		else if (nComponents == 3)
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);

		delete data;
	}

	void apply() {
		Material::apply();
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, id);
		//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	}

};

class Camera
{
	friend class Billboard;
	friend class MovableBillboard;

	float3 eye;

	float3 ahead;
	float3 lookAt;
	float3 right;
	float3 up;

	float fov;
	float aspect;

	float2 lastMousePos;
	float2 mouseDelta;

public:
	float3 getEye()
	{
		return eye;
	}
	float3 getAhead() 
	{
		return ahead;
	}
	Camera()
	{
		eye = float3(0, 100, 0);
		lookAt = float3(0, -1, 0);
		right = float3(1, 0, 0);
		up = float3(0, 1, 0);

		fov = 1.1;
		aspect = 1;
	}

	void apply()
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(fov / 3.14 * 180, aspect, 0.1, 500);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(0, 170, 0, 0, 0, 0, 0.0, 0.0, 1.0);
		//gluLookAt(eye.x, eye.y, eye.z, lookAt.x, lookAt.y, lookAt.z, 0.0, 1.0, 0.0);
		//gluLookAt(eye.x, eye.y, eye.z, -49, 99, -50, 5.0, -5.0, 0.0);

	}

	void setAspectRatio(float ar) { aspect = ar; }

	void move(float dt, std::vector<bool>& keysPressed)
	{
		if (keysPressed.at('w'))
			eye += ahead * dt * 20;
		if (keysPressed.at('s'))
			eye -= ahead * dt * 20;
		if (keysPressed.at('a'))
			eye -= right * dt * 20;
		if (keysPressed.at('d'))
			eye += right * dt * 20;
		if (keysPressed.at('q'))
			eye -= float3(0, 1, 0) * dt * 20;
		if (keysPressed.at('e'))
			eye += float3(0, 1, 0) * dt * 20;
		if (keysPressed.at(27)) {
			glutDestroyWindow(window_id);
			exit(0);
		}

		/*float yaw = atan2f(ahead.x, ahead.z);
		float pitch = -atan2f(ahead.y, sqrtf(ahead.x * ahead.x + ahead.z * ahead.z));

		yaw -= mouseDelta.x * 0.02f;
		pitch += mouseDelta.y * 0.02f;
		if (pitch > 3.14 / 2) pitch = 3.14 / 2;
		if (pitch < -3.14 / 2) pitch = -3.14 / 2;

		mouseDelta = float2(0, 0);

		ahead = float3(0, -1, 0);
		ahead = float3(sin(yaw)*cos(pitch), -sin(pitch), cos(yaw)*cos(pitch));
		right = ahead.cross(float3(0, 1, 0)).normalize();
		up = right.cross(ahead);

		lookAt = eye + ahead;*/
	}

	void startDrag(int x, int y)
	{
		lastMousePos = float2(x, y);
	}
	void drag(int x, int y)
	{
		float2 mousePos(x, y);
		mouseDelta = mousePos - lastMousePos;
		lastMousePos = mousePos;
	}
	void endDrag()
	{
		mouseDelta = float2(0, 0);
	}

};

class Object
{
protected:
	Material* material;
	Material* shadow;
	float3 scaleFactor;
	float3 position;
	float3 orientationAxis;
	float3 velocity;
	float orientationAngle;
	bool onGround;
public:
	Object(Material* material) :material(material), shadow(new TexturedMaterial("dark.png")), orientationAngle(0.0f), scaleFactor(1.0, 1.0, 1.0), orientationAxis(0.0, 1.0, 0.0), position(0, 0, 0), onGround(false) {}
	virtual ~Object() {}
	float3 getPosition() {
		return position;
	}
	float3 getVelocity() {
		return velocity;
	}
	void setVelocity(float3 v) {
		velocity = v;
	}

	bool onground() {
		return onGround;
	}
	Object* translate(float3 offset) {
		position += offset; return this;
	}
	Object* scale(float3 factor) {
		scaleFactor *= factor; return this;
	}
	Object* rotate(float angle) {
		orientationAngle += angle; return this;
	}
	virtual void draw()
	{
		material->apply();
		// apply scaling, translation and orientation
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(position.x, position.y, position.z);
		glRotatef(orientationAngle, orientationAxis.x, orientationAxis.y, orientationAxis.z);
		glScalef(scaleFactor.x, scaleFactor.y, scaleFactor.z);
		drawModel();
		glPopMatrix();
	}
	virtual void drawModel() = 0;
	virtual void drawShadow(float3 lightDir) 
	{
		if (position.y > -1) {
			// apply scaling, translation and orientation
			shadow->apply();
			glColor3f(0, 0, 0);
			glMatrixMode(GL_MODELVIEW);
			glColor3f(0, 0, 0);
			glPushMatrix();
			glTranslatef(position.x, 0.1, position.z);
			glRotatef(orientationAngle, orientationAxis.x, orientationAxis.y, orientationAxis.z);
			glScalef(scaleFactor.x-(position.y/200), 0, scaleFactor.z-(position.y/200));
			glColor3f(0, 0, 0);
			drawModel();
			glColor3f(0, 0, 0);
			glPopMatrix();
		}
	}
	virtual void move(double t, double dt) {}
	virtual bool control(std::vector<bool>& keysPressed, std::vector<Object*>& spawn, std::vector<Object*>& objects) { return false; }
};

class Ground : public Object
{
public:
	Ground(Material* m) : Object(m) {}

	void drawModel() {
		glBegin(GL_QUADS);
		glTexCoord2f(1, 1);
		glVertex3d(100, 0, 100);
		glTexCoord2f(0, 1);
		glVertex3d(-100, 0, 100);
		glTexCoord2f(0, 0);
		glVertex3d(-100, 0, -100);
		glTexCoord2f(1, 0);
		glVertex3d(100, 0, -100);
		glEnd();
	}

	void drawShadow(float3 lightDirection) {}
};

class MeshInstance : public Object
{
	Mesh* mesh;
public:
	MeshInstance(Mesh* mesh, Material* material) : mesh(mesh), Object(material) {}
	void drawModel()
	{
		mesh->draw();
	}
};

class Movable : public MeshInstance 
{
public:
	float3 acceleration;
	float angularVelocity;

	Movable(float3 velovity, float angularVelocity, MeshInstance* mesh) : angularVelocity(angularVelocity), MeshInstance(*mesh) {
		Object::setVelocity(velocity);
	}

	virtual void move(double t, double dt) {
		velocity += GRAVITY*dt;
		velocity += acceleration*dt;
		velocity *= pow(0.8, dt);
		if (position.y < 0 && (position.x<100 && position.x>-100 && position.z<100 && position.z>-100)) {
			if (!onGround) {
				addParticles(position);
				onGround = true;
			}
			velocity.y *= 0;
		}
		position += velocity*dt;
		//position.y *= angularVelocity;
		if (angularVelocity == 1.0f && (velocity.x != 0.0f || velocity.y != 0.0f)) {
			orientationAngle += 100*dt;
		}
		if (angularVelocity == -1.0f && (velocity.x != 0.0f || velocity.y != 0.0f)) {
			orientationAngle -= 100*dt;
		}
	}
};

class Bouncer : public Movable
{
public:
	float restitution;

	Bouncer(float restitution, Movable* movable) : restitution(restitution), Movable(*movable) {}

	virtual void move(double t, double dt) {
		Movable::move(t, dt);
		if (position.y < 0) velocity.y *= -restitution;
	}
};

class Controllable : public Movable 
{
public:
	Controllable(Movable* movable) : Movable(*movable) {}

	virtual bool control(std::vector<bool>& keysPressed, std::vector<Object*>& spawn, std::vector<Object*>& objects) {
		if (keysPressed.at('h') || keysPressed.at('k')) {
			if (keysPressed.at('h')) {
				angularVelocity = 1;
			}
			if (keysPressed.at('k')) {
				angularVelocity = -1;
			}
		}
		else 
		{
			angularVelocity = 0;
		}
		if (keysPressed.at('u') || keysPressed.at('j')) {
			int rads = 2 * M_PI * (orientationAngle / 360);
			if (keysPressed.at('u')) {
				acceleration = float3(-cos(rads) * 10, -10, sin(rads) * 10);
			}
			if (keysPressed.at('j')) {
				acceleration = float3(cos(rads) * 10, -10, -sin(rads) * 10);
			}
		}
		else
		{
			acceleration = float3(0, 0, 0);
		}
		return false; 
	}
};

class Billboard
{
public:
	float3 position;
	Material* material;
	float size;
	float opacity;
	int age;

	Billboard(Material* material, float3 position) : position(position), material(material), age(0), size(7), opacity(1)
	{
	}

	virtual void draw(Camera& camera)
	{
		//glDisable(GL_DEPTH_TEST);
		glDepthMask(false);
		glDisable(GL_LIGHTING);
		//glColor3f(1, 1, 1);
		material->apply();
		glEnable(GL_LIGHTING);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPushMatrix();
		glTranslatef(position.x, position.y, position.z);
		float camRotation[] = {
			camera.right.x, camera.up.x, camera.ahead.x, 0,
			camera.right.y, camera.up.y, camera.ahead.y, 0,
			camera.right.z, camera.up.z, camera.ahead.z, 0,
			0, 0, 0, 1
		};
		glMultMatrixf(camRotation);

		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex3f(-size, -size, 0);
		glTexCoord2f(1, 0);
		glVertex3f(size, -size, 0.0);
		glTexCoord2f(1, 1);
		glVertex3f(size, size, 0.0);
		glTexCoord2f(0, 1);
		glVertex3f(-size, size, 0.0);
		glEnd();

		glPopMatrix();
		glDisable(GL_BLEND);
		glDepthMask(true);
		//glEnable(GL_DEPTH_TEST);
	}

	virtual void move(double t, double dt) {}
};

class MovableBillboard : public Billboard
{
public:
	float3 velocity;
	MovableBillboard(float3 velocity, Billboard* billboard) : Billboard(*billboard), velocity(velocity) {}

	void move(double t, double dt) {
		position += velocity*dt;
		age++;
		size += .01;
		opacity -= .1;
		velocity.y -= .01;
	}

	virtual void draw(Camera& camera)
	{
		//glDisable(GL_DEPTH_TEST);
		glDepthMask(false);
		glDisable(GL_LIGHTING);
		//glColor3f(1, 1, 1);
		material->apply();
		glEnable(GL_LIGHTING);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPushMatrix();
		glTranslatef(position.x, position.y, position.z);
		float camRotation[] = {
			camera.right.x, camera.up.x, camera.ahead.x, 0,
			camera.right.y, 1, 1, 0,
			camera.right.z, camera.up.z, camera.ahead.z, 0,
			0, 0, 0, 1
		};
		glMultMatrixf(camRotation);

		//glClearColor(0, 0, 0, 0);

		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glColor4f(0, 0, opacity, opacity);
		glVertex3f(-size, -size, 0);
		glTexCoord2f(1, 0);
		glColor4f(0, 0, opacity, opacity);
		glVertex3f(size, -size, 0.0);
		glTexCoord2f(1, 1);
		glColor4f(0, 0, opacity, opacity);
		glVertex3f(size, size, 0.0);
		glTexCoord2f(0, 1);
		glColor4f(0, 0, opacity, opacity);
		glVertex3f(-size, size, 0.0);
		glEnd();

		glPopMatrix();
		glDisable(GL_BLEND);
		glDepthMask(true);
		//glEnable(GL_DEPTH_TEST);
	}
};

class Teapot : public Object
{
public:
	Teapot(Material* material) :Object(material) {}
	void drawModel()
	{
		glutSolidTeapot(1.0f);
	}
};

class Scene
{
	Camera camera;
	std::vector<LightSource*> lightSources;
	std::vector<Object*> objects;
	std::vector<Material*> materials;
	std::vector<Mesh*> meshes;
	std::vector<Billboard*> billboards;

	struct CameraDepthComparator {
		float3 ahead;
		bool operator() (
			Billboard* a, Billboard* b) {
			return
				((a->position.dot(ahead)) >
					(b->position.dot(ahead) + 0.01));
		}
	} comp = { camera.getAhead() };

public:
	void initialize()
	{
		// BUILD YOUR SCENE HERE
		lightSources.push_back(
			new DirectionalLight(
				float3(0, 1, 0),
				float3(1, 0.5, 1)));
		lightSources.push_back(
			new PointLight(
				float3(-1, -1, 1),
				float3(0.2, 0.1, 0.1)));
		Material* yellowDiffuseMaterial = new Material();
		materials.push_back(yellowDiffuseMaterial);
		yellowDiffuseMaterial->kd = float3(1, 1, 0);
		materials.push_back(new Material());
		materials.push_back(new Material());
		materials.push_back(new Material());
		materials.push_back(new Material());
		materials.push_back(new Material());
		materials.push_back(new Material());

		objects.push_back(new Ground(new TexturedMaterial("ground.jpg")));
		//objects.push_back((new Teapot(yellowDiffuseMaterial))->translate(float3(0, -1, 0)));
		//objects.push_back((new Teapot(materials.at(2)))->translate(float3(0, 1.2, 0.5))->scale(float3(1.3, 1.3, 1.3)));
		//objects.push_back((new Teapot(materials.at(1)))->translate(float3(0, -1, -2))->scale(float3(0.5, 0.5, 0.5)));
		objects.push_back((new Controllable(new Movable(float3(.1, .1, .1), .1, new MeshInstance(new Mesh("truck1.obj"), new TexturedMaterial("humvee.jpg")))))->translate(float3(0,100,0))->scale(float3(2,2,2)));
		for (int i = 0; i < 100; i++)
			billboards.push_back(new Billboard(new TexturedMaterial("grass.png"), float3((rand() % 190) - 95, .1, (rand() % 190) - 95)));
		//meshes.push_back(new Mesh("tigger.obj"));

	}
	~Scene()
	{
		for (std::vector<LightSource*>::iterator iLightSource = lightSources.begin(); iLightSource != lightSources.end(); ++iLightSource)
			delete *iLightSource;
		for (std::vector<Material*>::iterator iMaterial = materials.begin(); iMaterial != materials.end(); ++iMaterial)
			delete *iMaterial;
		for (std::vector<Object*>::iterator iObject = objects.begin(); iObject != objects.end(); ++iObject)
			delete *iObject;
		for (std::vector<Mesh*>::iterator iMesh = meshes.begin(); iMesh != meshes.end(); ++iMesh)
			delete *iMesh;
		for (std::vector<Billboard*>::iterator iBillboard = billboards.begin(); iBillboard != billboards.end(); ++iBillboard)
			delete *iBillboard;
	}

public:
	Camera& getCamera()
	{
		return camera;
	}

	void draw()
	{
		camera.apply();
		unsigned int iLightSource = 0;
		for (; iLightSource < lightSources.size(); iLightSource++)
		{
			glEnable(GL_LIGHT0 + iLightSource);
			lightSources.at(iLightSource)->apply(GL_LIGHT0 + iLightSource);
		}
		for (; iLightSource < GL_MAX_LIGHTS; iLightSource++)
			glDisable(GL_LIGHT0 + iLightSource);

		for (unsigned int iObject = 0; iObject < objects.size(); iObject++) {
			objects.at(iObject)->draw();
			if (iObject == 1) {
				for (unsigned int iObject1 = 2; iObject1 < objects.size(); iObject1++) {
					//printf("checking");
					if ((objects.at(iObject)->getPosition().x - objects.at(iObject1)->getPosition().x) < 10 && (objects.at(iObject)->getPosition().x - objects.at(iObject1)->getPosition().x) > -10 && (objects.at(iObject)->getPosition().z - objects.at(iObject1)->getPosition().z) < 10 && (objects.at(iObject)->getPosition().z - objects.at(iObject1)->getPosition().z) > -10 ){
						if (objects.at(iObject1)->onground()) {
							objects.at(iObject1)->setVelocity(objects.at(iObject)->getVelocity()*2);
						}
						else if ((objects.at(iObject)->getPosition().y - objects.at(iObject1)->getPosition().y) < 5) {
							printf("YOU DIED");
							glutDestroyWindow(window_id);
							exit(0);
						}
					}
				}
			}
			glDisable(GL_LIGHTING);
			objects.at(iObject)->drawShadow(lightSources.at(0)->getLightDirAt(float3(0, 0, 0)));
			glEnable(GL_LIGHTING);

		}

		for (unsigned int iMesh = 0; iMesh < meshes.size(); iMesh++)
			meshes.at(iMesh)->draw();

		for (unsigned int iBillboard = 0; iBillboard < billboards.size(); iBillboard++)
			billboards.at(iBillboard)->draw(camera);

		objects.erase(std::remove_if(objects.begin(), objects.end(), eraseO), objects.end());
		billboards.erase(std::remove_if(billboards.begin(), billboards.end(), eraseb), billboards.end());

	}	

	static bool eraseO(Object* iObject) {
		if (iObject->getPosition().y < -10)
			return true;
		else
			return false;
	}

	static bool eraseb(Billboard* iObject) {
		if (iObject->position.y < -10)
			return true;
		else
			return false;
	}

	void move(double t, double dt)
	{
		if (fmod(t, 2) == 0) {
			//printf("hi");
			objects.push_back((new Movable(float3(.1, .1, .1), .1, new MeshInstance(new Mesh("tree.obj"), new TexturedMaterial("tree.png"))))->translate(float3((rand()%190)-95, 200, (rand()%190) - 95)));
		}

		std::vector<Object*> spawn;

		for (unsigned int iObject = 0; iObject < objects.size(); iObject++) {
			objects.at(iObject)->control(keysPressed, spawn, objects);
			objects.at(iObject)->move(t, dt);
		}

		for (unsigned int iBillboard = 0; iBillboard < billboards.size(); iBillboard++) {
			billboards.at(iBillboard)->move(t, dt);
		}

		std::sort(billboards.begin(), billboards.end(), comp);
	}

	void addParticles(float3 position) {
		for (int i = 0; i < 50; i++) {
			//printf("hi");
			//billboards.push_back(new Billboard(new TexturedMaterial("grass.png"), float3(rand() % (10) - .5, rand() % (10) - .5, rand() % (10) - .5)));
			billboards.push_back(new MovableBillboard(float3(rand() % (10) - 5, rand() % (10) - 5, rand() % (10) - 5), new Billboard(new TexturedMaterial("dust.png"), float3(position.x + rand() % (1) - .5, position.y + rand() % (1) - .5, position.z + rand() % (1) - .5))));
		}
	}
};

Scene scene;

void addParticles(float3 position) {
	scene.addParticles(position);
}

void onDisplay() {
	glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear screen

	scene.draw();

	glutSwapBuffers(); // drawing finished
}

void onIdle()
{
	double t = glutGet(GLUT_ELAPSED_TIME) * 0.001;        	// time elapsed since starting this program in msec 
	static double lastTime = 0.0;
	double dt = t - lastTime;
	lastTime = t;

	scene.getCamera().move(dt, keysPressed);
	scene.move(t, dt);

	glutPostRedisplay();
}

void onKeyboard(unsigned char key, int x, int y)
{
	keysPressed.at(key) = true;
}

void onKeyboardUp(unsigned char key, int x, int y)
{
	keysPressed.at(key) = false;
}

void onMouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON)
		if (state == GLUT_DOWN)
			scene.getCamera().startDrag(x, y);
		else
			scene.getCamera().endDrag();
}

void onMouseMotion(int x, int y)
{
	scene.getCamera().drag(x, y);
}

void onReshape(int winWidth, int winHeight)
{
	glViewport(0, 0, winWidth, winHeight);
	scene.getCamera().setAspectRatio((float)winWidth / winHeight);
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);						// initialize GLUT
	glutInitWindowSize(600, 600);				// startup window size 
	glutInitWindowPosition(100, 100);           // where to put window on screen
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);    // 8 bit R,G,B,A + double buffer + depth buffer

	window_id = glutCreateWindow("OpenGL teapots");				// application window is created and displayed

	//glViewport(0, 0, 600, 600);
	glutFullScreen();

	glutDisplayFunc(onDisplay);					// register callback
	glutIdleFunc(onIdle);						// register callback
	glutReshapeFunc(onReshape);
	glutKeyboardFunc(onKeyboard);
	glutKeyboardUpFunc(onKeyboardUp);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMouseMotion);

	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);

	scene.initialize();
	for (int i = 0; i<256; i++)
		keysPressed.push_back(false);

	glutMainLoop();								// launch event handling loop

	return 0;
}