
#include "guiButtonAnimator.h"
#include "guiButton.h"
#include "log.h"


GUIButtonAnimator::GUIButtonAnimator(const string& tname, const array<s32, 2> default_size, GUIButton* but)
    : m_button(but), m_default_size(default_size)
{
    m_thread = new ButtonSmoothScaleThread(tname, this);
    
    infostream << "default_size[0]: " << default_size[0] << "\n";
    infostream << "default_size[1]: " << default_size[1] << "\n";
    m_end_size[0] = default_size[0]/SIZE_DIV + default_size[0];
    m_end_size[1] = default_size[1]/SIZE_DIV + default_size[1];
        
    infostream << "m_end_size[0] - m_default_size[1]: " << (m_end_size[0] - m_default_size[0]) << "\n";
    m_size_change[0] = (m_end_size[0] - m_default_size[0])/SIZE_CHANGE_DIV;
    m_size_change[1] = (m_end_size[1] - m_default_size[1])/SIZE_CHANGE_DIV;
    
    infostream << "m_size_change[0]: " << m_size_change[0] << "\n";
    infostream << "m_size_change[1]: " << m_size_change[1] << "\n";
}

GUIButtonAnimator::~GUIButtonAnimator()
{
    m_button = nullptr;
    m_thread = nullptr;
}

 void GUIButtonAnimator::smoothScale(AnimationType type) {
        m_anim_type = type;
        m_thread->start();
}

void* ButtonSmoothScaleThread::run() {
    auto& iter = m_button_anim->m_iter;
    auto& end_iter = m_button_anim->m_end_iter;
    auto& anim_type = m_button_anim->m_anim_type;
    auto& size_change = m_button_anim->m_size_change;
            
    while (iter != end_iter) {
        static s32 cur_w = 0;
        static s32 cur_h = 0;
        
        auto rel_pos = m_button_anim->m_button->getRelativePosition();
        cur_w = rel_pos.getWidth();
        cur_h = rel_pos.getHeight();
                
        s32 new_w = (anim_type == m_button_anim->ANIMATION_SCALEUP ? cur_w + size_change[0] : cur_w - size_change[0]);
        s32 new_h = (anim_type == m_button_anim->ANIMATION_SCALEUP ? cur_h + size_change[1] : cur_h - size_change[1]);
        
        infostream << "new_w: " << new_w << "\n";
        infostream << "new_h: " << new_h << "\n";
        m_button_anim->m_button->setRelRectSize(new_w, new_h);
                
        ++iter;
        infostream << iter << "\n";
        infostream << "current width: " << rel_pos.getWidth() << ", current height: " << rel_pos.getHeight() << "\n";
                
        this_thread::sleep_for(chrono::milliseconds{20});
                
                
    }
            
    iter = 0;
    anim_type = m_button_anim->ANIMATION_NONE;
            
        
    bool* finished = new bool(true);
    return finished;
}
 
