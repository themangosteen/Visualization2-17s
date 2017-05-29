#pragma once

#include <glm/glm.hpp>

#define M_PI 3.14159265358979323846


//! Camera class

//! Camera class supporting perspective and orthographic projection
//! Mouse interaction allows to rotate around target
class Camera
{
public:

	Camera();

	~Camera(void);

	void reset();

	void buildViewMatrix(void);

	void buildProjectionMatrix(void);

	void zoom(float t);

	void rotateAzimuth(float t);

	void rotatePolar(float t);

	void moveLeft(float t);

	void moveRight(float t);

	void moveForwards(float t);

	void moveBackwards(float t);

	void moveUp(float t);

	void moveDown(float t);

	void rotateUp(float t);

	void rotateDown(float t);

	void rotateLeft(float t);

	void rotateRight(float t);

	void setOrthogonalBorders(float left, float right, float top, float bottom);

	glm::vec3 getUp() { return mUp; };

	glm::vec3 getRight() { return glm::inverse(mViewMatrix)[0]; };

	glm::vec3 getTarget() { return mTarget; };

	glm::vec3 getPosition() { return mPos; };

	bool isOrthogonal() { return mOrthogonal; };

	float getFieldOfView() { return mFieldOfView; };

	void setOrthogonal(bool orthogonal) { mOrthogonal = orthogonal; };

	void setUp(glm::vec3 &up) { mUp = up; };

	void setTarget(glm::vec3 &target) { mTarget = target; };

	void setFieldOfView(float fieldOfView) { mFieldOfView = fieldOfView; };

	void setNearPlane(float nearPlane) { mNear = nearPlane; };

	void setFarPlane(float farPlane) { mFar = farPlane; buildProjectionMatrix(); };

	float getNearPlane() { return mNear; };

	float getFarPlane() { return mFar; };

	void setAspect(float aspect) { mAspect = aspect;  buildProjectionMatrix();};

	const glm::mat4 & getViewMatrix() { return mViewMatrix; };

	const glm::mat4 & getProjectionMatrix() { return mProjectionMatrix; };


private:
	glm::mat4 mViewMatrix;
	glm::mat4 mProjectionMatrix;

	float mRadius;
	float mAzimuth;
	float mPolar;

	glm::vec3 mUp;
	glm::vec3 mTarget;
	glm::vec3 mPos;
	float mFieldOfView;
	float mAspect;
	float mNear;
	float mFar;
	float mLeft;
	float mRight;
	float mTop;
	float mBottom;
	bool mOrthogonal;
};
