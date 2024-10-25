#pragma once
#include "gui/mainmenumanager.h"
#include "inputhandler.h"
#include "remoteclient.capnp.h"

#include <condition_variable>
#include <map>
#include <mutex>
#include <string>

#include <capnp/message.h>
#include <capnp/rpc-twoparty.h>
#include <capnp/serialize-packed.h>
#include <kj/async-io.h>

namespace detail {
struct Channel {
  std::condition_variable m_action_cv;
  std::condition_variable m_obs_cv;
  std::mutex m_action_mutex;
  std::mutex m_obs_mutex;
  ::capnp::MallocMessageBuilder m_obs_msg_builder;
  Observation::Builder m_obs_builder; // GUARDED_BY(m_obs_mutex)
  Image::Builder m_image_builder; // GUARDED_BY(m_obs_mutex)
  AuxMap::Builder m_aux_map_builder; // GUARDED_BY(m_obs_mutex)
  Action::Reader *m_action; // GUARDED_BY(m_action_mutex)
  bool m_has_obs{}; // GUARDED_BY(m_obs_mutex)
  bool m_did_init{}; // GUARDED_BY(m_action_mutex)

  Channel() : m_obs_builder{m_obs_msg_builder.initRoot<Observation>()},
              m_image_builder{nullptr},
              m_aux_map_builder{nullptr},
              m_action{nullptr} {
    m_obs_builder.initImage();
    m_image_builder = m_obs_builder.getImage();
    m_obs_builder.initAux();
    m_aux_map_builder = m_obs_builder.getAux();
  }
};

class MinetestImpl : public Minetest::Server {
public:
  MinetestImpl(Channel *chan) : m_chan(chan) {}
  kj::Promise<void> init(InitContext context) override;
  kj::Promise<void> step(StepContext context) override;
private:
  Channel *m_chan;
};
} // namespace detail

class RemoteInputHandler : public InputHandler {
public:
  RemoteInputHandler(const std::string &endpoint,
                     RenderingEngine *rendering_engine,
                     MyEventReceiver *receiver);

  virtual ~RemoteInputHandler() noexcept override = default;

  bool isDetached() const override { return true; }

  virtual bool isKeyDown(GameKeyType k) override {
    return m_key_is_down[keycache.key[k]];
  }
  virtual bool wasKeyDown(GameKeyType k) override {
    bool b = m_key_was_down[keycache.key[k]];
    if (b)
      m_key_was_down.unset(keycache.key[k]);
    return b;
  }
  virtual bool wasKeyPressed(GameKeyType k) override {
    return m_key_was_pressed[keycache.key[k]];
  }
  virtual bool wasKeyReleased(GameKeyType k) override {
    return m_key_was_released[keycache.key[k]];
  }
  virtual bool cancelPressed() override {
    return m_key_was_down[keycache.key[KeyType::ESC]];
  }
  virtual float getMovementSpeed() override {
    bool f = m_key_is_down[keycache.key[KeyType::FORWARD]],
         b = m_key_is_down[keycache.key[KeyType::BACKWARD]],
         l = m_key_is_down[keycache.key[KeyType::LEFT]],
         r = m_key_is_down[keycache.key[KeyType::RIGHT]];
    if (f || b || l || r) {
      // if contradictory keys pressed, stay still
      if (f && b && l && r)
        return 0.0f;
      else if (f && b && !l && !r)
        return 0.0f;
      else if (!f && !b && l && r)
        return 0.0f;
      return 1.0f; // If there is a keyboard event, assume maximum speed
    }
    return 0.0f;
  }
  virtual float getMovementDirection() override {
    float x = 0, z = 0;

    /* Check keyboard for input */
    if (m_key_is_down[keycache.key[KeyType::FORWARD]])
      z += 1;
    if (m_key_is_down[keycache.key[KeyType::BACKWARD]])
      z -= 1;
    if (m_key_is_down[keycache.key[KeyType::RIGHT]])
      x += 1;
    if (m_key_is_down[keycache.key[KeyType::LEFT]])
      x -= 1;

    if (x != 0 || z != 0) /* If there is a keyboard event, it takes priority */
      return atan2(x, z);
    return m_movement_direction;
  }
  virtual v2s32 getMousePos() override { return m_mouse_pos; }
  virtual void setMousePos(s32 x, s32 y) override { m_mouse_pos = v2s32(x, y); }

  virtual s32 getMouseWheel() override {
    s32 prev = m_mouse_wheel;
    m_mouse_wheel = 0;
    return prev;
  }

  virtual void clearWasKeyPressed() override { m_key_was_pressed.clear(); }
  virtual void clearWasKeyReleased() override { m_key_was_released.clear(); }
  void clearInput();

  virtual void step(float dtime) override;
  virtual void step_post_render() override;

  void simulateEvent(const SEvent &event) {
    if (event.EventType == EET_MOUSE_INPUT_EVENT) {
      // we need this call to trigger GUIEvents
      // e.g. for updating selected/hovered elements
      // in the inventory
      // BUT somehow only simulating with this call
      // does not trigger any mouse movement at all..
      guienv->postEventFromUser(event);
    }
    // .. which is why we need this second call
    // TODO is it possible to have all behaviors with one call?
    m_receiver->OnEvent(event);
  }

private:
  void fill_observation(irr::video::IImage *image, float reward, std::map<std::string, float> aux);

  RenderingEngine *m_rendering_engine;

  detail::Channel m_chan;

  // Event receiver to simulate events
  MyEventReceiver *m_receiver = nullptr;

  // The state of the mouse wheel
  s32 m_mouse_wheel = 0;

  // The current state of keys
  KeyList m_key_is_down;

  // Like keyIsDown but only reset when that key is read
  KeyList m_key_was_down;

  // Whether a key has just been pressed
  KeyList m_key_was_pressed;

  // Whether a key has just been released
  KeyList m_key_was_released;

  // Mouse observables
  v2s32 m_mouse_pos;
  v2s32 m_mouse_speed;

  float m_movement_direction;

  bool m_is_first_loop = true;

  bool m_should_send_observation = false;
};
