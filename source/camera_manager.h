#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <iostream>

class CameraManager
{
  public:
    CameraManager(
        glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f), 
        //                       pitch,  yaw, roll
        glm::vec3 ang = glm::vec3(0.0f, 0.0f, 0.0f)
    );

    // generate world_to_view matrix
    glm::mat4 get_lookAt();
    // generate projection matrix
    glm::mat4 get_projection();

    void set_position(glm::vec3 pos);
    glm::vec3 get_position();
    void set_direction(glm::vec3 dir);
    glm::vec3 get_direction();
    void set_target(glm::vec3 targ);
    void move_foreward(float displace);
    void move_vertical(float displace);
    void move_global_vertical(float displace);
    void move_horizontal(float displace);
    void turn_yaw(float deg);
    void turn_pitch(float deg);

    void set_view_height(unsigned int pix);
    unsigned int get_view_height();
    void set_view_width(unsigned int pix);
    unsigned int get_view_width();

    void set_view_fov(float degrees);
    float get_view_fov();
    void set_use_perspective(bool enable);
    bool get_use_perspective();

    void set_near_plane(float near);
    float get_near_plane();
    void set_far_plane(float far);
    float get_far_plane();


  private:
    const glm::vec3 GLOBAL_UP = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 position;

    // keep these representations in sync (minus euler.z/roll)
    glm::vec3 direction;
    glm::vec3 euler;

    float viewNear;
    float viewFar;
    unsigned int viewWidth;
    unsigned int viewHeight;
    float viewFov;
    bool usePerspective;

    // set direciton = newDirection, and set matching euler
    void _set_and_match_direction(glm::vec3 newDirection);
    // set euler = newEuler, and set matching direction
    void _set_and_match_euler(glm::vec3 newEuler);

};

CameraManager::CameraManager(
    glm::vec3 pos,
    glm::vec3 ang)
    : position(pos)
    , viewNear(0.1f)
    , viewFar(100.0f)
    , viewWidth(1290)    // 3440:1440 reduced is 43:18
    , viewHeight(540)
    , viewFov(45.0f)
    , usePerspective(true)
{
    _set_and_match_euler(ang);
}

glm::mat4 CameraManager::get_lookAt()
{
    return glm::lookAt(position, position + direction, GLOBAL_UP);
}

glm::mat4 CameraManager::get_projection()
{
    if (usePerspective)
    {
        return glm::perspective(
            glm::radians(viewFov), 
            (float)viewWidth/(float)viewHeight, 
            viewNear, viewFar
        );
    }
    else
    {
        // how to determine left, right, bottom, top?
        // view fov 45 -> orthoRatio ~100?
        float orthoRatio = (45.0f / viewFov) * 100;
        return glm::ortho(
            -1 * (float)(viewWidth)/orthoRatio, 
             1 * (float)(viewWidth)/orthoRatio, 
            -1 * (float)(viewHeight)/orthoRatio, 
             1 * (float)(viewHeight)/orthoRatio, 
            viewNear, viewFar
        );
    }
}

void CameraManager::set_position(glm::vec3 pos)
{
    position = pos;
}
glm::vec3 CameraManager::get_position()
{
    return position;
}
void CameraManager::set_direction(glm::vec3 dir)
{
    _set_and_match_direction(dir);
}
glm::vec3 CameraManager::get_direction()
{
    return direction;
}
void CameraManager::set_target(glm::vec3 targ)
{
    _set_and_match_direction(targ - position);
}


void CameraManager::move_foreward(float displace)
{
    position += displace * direction;
}
void CameraManager::move_vertical(float displace)
{
    position += displace * glm::normalize(glm::cross(glm::cross(direction, GLOBAL_UP), direction));
}
// move up or down along (0,1,0). Practically up/down as defined by gravity
void CameraManager::move_global_vertical(float displace)
{
    position += displace * GLOBAL_UP;
}
void CameraManager::move_horizontal(float displace)
{
    position += displace * glm::normalize(glm::cross(direction, GLOBAL_UP));
}


void CameraManager::turn_yaw(float deg)
{
    _set_and_match_euler(glm::vec3(euler.x, euler.y + deg, euler.z));
}
void CameraManager::turn_pitch(float deg)
{
    _set_and_match_euler(glm::vec3(euler.x + deg, euler.y, euler.z));
}


void CameraManager::set_view_height(unsigned int pix)
{
    viewHeight = pix;
}
unsigned int CameraManager::get_view_height()
{
    return viewHeight;
}


void CameraManager::set_view_width(unsigned int pix)
{
    viewWidth = pix;
}
unsigned int CameraManager::get_view_width()
{
    return viewWidth;
}


void CameraManager::set_view_fov(float degrees)
{
    viewFov = glm::max(degrees, 1.0f);
    viewFov = glm::min(viewFov, 120.0f);
}
float CameraManager::get_view_fov()
{
    return viewFov;
}

void CameraManager::set_use_perspective(bool enable)
{
    usePerspective = enable;
}
bool CameraManager::get_use_perspective()
{
    return usePerspective;
}

void CameraManager::set_near_plane(float near)
{
    viewNear = glm::max(0.0001f, near);
}
float CameraManager::get_near_plane()
{
    return viewNear;
}
void CameraManager::set_far_plane(float far)
{
    viewFar = far;
}
float CameraManager::get_far_plane()
{
    return viewFar;
}

void CameraManager::_set_and_match_euler(glm::vec3 newEuler)
{
    euler = newEuler;
    euler.x = glm::max(euler.x, -89.0f);
    euler.x = glm::min(euler.x,  89.0f);
    euler.y = euler.y >= 360 ? euler.y - 360 : euler.y; // cannot handle angles over 720
    euler.y = euler.y < 0 ? euler.y + 360 : euler.y;

    direction.x = cos(glm::radians(euler.y)) * cos(glm::radians(euler.x));
    direction.y = sin(glm::radians(euler.x));
    direction.z = sin(glm::radians(euler.y)) * cos(glm::radians(euler.x));
    direction = glm::normalize(direction);
}

void CameraManager::_set_and_match_direction(glm::vec3 newDirection)
{
    direction = glm::normalize(newDirection);
    // maintain same roll value;
    glm::vec3 xz = glm::normalize(glm::vec3(direction.x, 0, direction.z));
    glm::vec3 nullYaw(1.0f, 0.0f, 0.0f);

    // get absolute angle (between 0 and 180)
    euler.y = glm::degrees(acos(glm::dot(xz, nullYaw)));
    // quadrant determination
    if (direction.z < 0) euler.y = 360 - euler.y;
    if (euler.y < 0) euler.y += 360; // clamp 0 to 359.9

    euler.x = glm::degrees(acos(glm::dot(direction, xz)));
    if (direction.y < 0) euler.x *= -1;
}

#endif // CAMERA_MANAGER_H