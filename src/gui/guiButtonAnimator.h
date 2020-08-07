#pragma once


#include "irrlichttypes.h"
#include "threading/thread.h"
#include <string>
#include <array>
#include <cstddef>
#include <chrono>


#define SIZE_DIV  6                       // Divide default button size by this value for specifying maximum change of the button size when hovering. The division is necessary for keeping the proportions.
#define MAX_WIDTH(w)  (w / SIZE_DIV)      // Maximum change of the button width.
#define MAX_HEIGHT(h)  (h / SIZE_DIV)     // Maximum change of the button height.
#define SIZE_CHANGE_DIV  4
#define SIZE_CHANGE_WIDTH(w)  (w / SIZE_CHANGE_DIV)
#define SIZE_CHANGE_HEIGHT(h)  (h / SIZE_CHANGE_DIV)


class GUIButton;
class ButtonSmoothScaleThread;

using namespace std;

class GUIButtonAnimator {
public:
    GUIButtonAnimator(const string& tname, const array<s32, 2> default_size, GUIButton* but);
    
    ~GUIButtonAnimator();
    
    
    enum AnimationType {
        ANIMATION_SCALEUP,
        ANIMATION_SCALEDOWN,
        ANIMATION_NONE
    };
    
    void smoothScale(AnimationType type);
    
private:
    size_t m_iter = 0;
    const size_t m_end_iter = SIZE_CHANGE_DIV;
    GUIButton* m_button;
    ButtonSmoothScaleThread* m_thread;
    
    AnimationType m_anim_type = ANIMATION_NONE;
    
    array<s32, 2> m_default_size;
    array<s32, 2> m_end_size;
    array<s32, 2> m_size_change;
    
    friend class ButtonSmoothScaleThread;
    friend class GUIButton;
};

class ButtonSmoothScaleThread : public Thread {
public:
    ButtonSmoothScaleThread(const string& name = "", GUIButtonAnimator* but_anim=nullptr) : Thread(name), m_button_anim(but_anim)
    {
    }
    
protected:
    virtual void* run();
private:
    GUIButtonAnimator* m_button_anim;
};
