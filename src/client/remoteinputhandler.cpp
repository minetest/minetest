#include "remoteinputhandler.h"
#include "client/keycode.h"
#include "hud.h"
#include "server.h"
#include <cassert>
#include <string>
#include <string_view>
#include <sstream>

#include <capnp/message.h>
#include <capnp/serialize.h>
#include <capnp/rpc.h>
#include <kj/array.h>


namespace detail {

kj::Promise<void> MinetestImpl::init(InitContext context) {
  std::unique_lock<std::mutex> lock(m_chan->m_action_mutex);
  m_chan->m_did_init = true;
  m_chan->m_action_cv.notify_one();
  return kj::READY_NOW;
}

kj::Promise<void> MinetestImpl::step(StepContext context) {
  Action::Reader action = context.getParams().getAction();
  {
    std::unique_lock<std::mutex> lock(m_chan->m_action_mutex);
    m_chan->m_action = &action;
    m_chan->m_action_cv.notify_one();
  }

  {
    std::unique_lock<std::mutex> lock(m_chan->m_obs_mutex);
    m_chan->m_obs_cv.wait(lock, [this] { return m_chan->m_has_obs; });
    context.getResults().setObservation(m_chan->m_obs_builder.asReader());
    m_chan->m_has_obs = false;
    m_chan->m_obs_cv.notify_one();
  }

  std::unique_lock<std::mutex> lock(m_chan->m_action_mutex);
  // We can't return until action has been consumed, because it will be destroyed
  // when the context is destroyed.
  m_chan->m_action_cv.wait(lock, [this] { return m_chan->m_action == nullptr; });
  return kj::READY_NOW;
}

} // namespace detail

RemoteInputHandler::RemoteInputHandler(const std::string &endpoint,
                                       RenderingEngine *rendering_engine,
                                       MyEventReceiver *receiver)
    : m_rendering_engine(rendering_engine),
      m_receiver(receiver) {

  std::unique_lock<std::mutex> lock(m_chan.m_action_mutex);
  std::thread([chan = &m_chan, endpoint]() {
    kj::AsyncIoContext capnp_io_context(kj::setupAsyncIo());
    kj::Network& network = capnp_io_context.provider->getNetwork();
    kj::Own<kj::NetworkAddress> addr = network.parseAddress(endpoint).wait(capnp_io_context.waitScope);
    kj::Own<kj::ConnectionReceiver> listener = addr->listen();
    // Ask the listener for the port rather than just logging endpoint,
    // in case it was chosen automatically.
    uint port = listener->getPort();
    if (port) {
      infostream << "RemoteInputHandler: Listening on port TCP port " << port << "...";
    } else {
      infostream << "RemoteInputHandler: Listening on " << endpoint << "...";
    }
    capnp::TwoPartyServer server(kj::heap<detail::MinetestImpl>(chan));
    server.listen(*listener).wait(capnp_io_context.waitScope);
  }).detach();
  m_chan.m_action_cv.wait(lock, [this] { return m_chan.m_did_init; });
};

void RemoteInputHandler::step(float dtime) {
  // skip first loop, because we don't have an observation yet
  // as draw is called after step
  if (m_is_first_loop) {
    m_is_first_loop = false;
    return;
  }

  KeyList new_key_is_down;
  v2s32 mouse_movement;

  // Receive next action.
  {
    std::unique_lock<std::mutex> lock(m_chan.m_action_mutex);
    m_chan.m_action_cv.wait(lock, [this] { return m_chan.m_action; });

    // Copy data out of action.
    for (auto keyEvent : m_chan.m_action->getKeyEvents()) {
      KeyPress key_code = keycache.key[static_cast<int>(keyEvent)];
      new_key_is_down.set(key_code);
    }
    mouse_movement = v2s32(m_chan.m_action->getMouseDx(), m_chan.m_action->getMouseDy());

    m_chan.m_action = nullptr;
    m_chan.m_action_cv.notify_one();
  }

  // Process action.
  {
    // Compute pressed keys.
    m_key_was_pressed.clear();
    for (const auto& key_code: new_key_is_down) {
      if (!m_key_is_down[key_code]) {
        m_key_was_pressed.set(key_code);
      }
    }

    // Compute released keys.
    m_key_was_released.clear();
    for (const auto& key_code: m_key_is_down) {
      if (!new_key_is_down[key_code]) {
        m_key_was_released.set(key_code);
      }
    }

    // Apply new key state.
    m_key_is_down.clear();
    m_key_is_down.append(new_key_is_down);
    m_key_was_down.clear();
    m_key_was_down.append(new_key_is_down);

    // Apply mouse state.
    // mousepos is reset to (WIDTH/2, HEIGHT/2) after every iteration of main game
    // loop unit is pixels, origin is top left corner, bounds is (0,0) to (WIDTH,
    // HEIGHT)
    m_mouse_speed = mouse_movement;
    m_mouse_pos += m_mouse_speed;
    m_mouse_wheel = 0;
  }

  float remote_input_handler_time_step = 0.0f;
  g_settings->getFloatNoEx("remote_input_handler_time_step", remote_input_handler_time_step);
  if(remote_input_handler_time_step > 0.0f) {
    gServer->AsyncRunStep(remote_input_handler_time_step);
  }

  m_should_send_observation = true;
};

void RemoteInputHandler::step_post_render() {
  if (m_should_send_observation) {
    m_should_send_observation = false;

    // send current observation
    irr::video::IVideoDriver *driver = m_rendering_engine->get_video_driver();
    irr::video::IImage *image = driver->createScreenShot(video::ECF_R8G8B8);

    // parse score from hud
    // during game startup, the hud is not yet initialized, so there'll be no
    // score for the first 1-2 steps
    float score{};
    std::map<std::string, float> aux{};
    for (u32 i = 0; i < m_player->maxHudId(); ++i) {
      auto hud_element = m_player->getHud(i);
      std::string_view elem_text = hud_element->text;
      // find the index of the first :
      const auto colon_pos = elem_text.find(':');
      if (colon_pos == std::string_view::npos) {
        continue;
      }
      elem_text.remove_prefix(colon_pos + 1); // +1 for space
      // I'd rather use std::from_chars, but it's not available in libc++ yet.
      std::stringstream ss{std::string(elem_text)};
      if (hud_element->name == "score") {
        ss >> score;
      } else {
        float value{};
        ss >> value;
        aux[hud_element->name] = value;
      }
    }

    std::unique_lock<std::mutex> lock(m_chan.m_obs_mutex);
    m_chan.m_obs_cv.wait(lock, [this] { return !m_chan.m_has_obs; });

    m_chan.m_obs_builder.setReward(score);

    // We need to keep around the underlying data until the capnp message is serialized. We saw no
    // way to add this drop after the serialization occurs for the generated server implementation,
    // and so we decided to drop the previous image, if any, here.
    if (m_chan.m_image_builder_data != nullptr) {
      m_chan.m_image_builder_data->drop();
    }
    m_chan.m_image_builder_data = image;

    m_chan.m_image_builder.setWidth(image->getDimension().Width);
    m_chan.m_image_builder.setHeight(image->getDimension().Height);
    m_chan.m_image_builder.setData(
        capnp::Data::Reader(reinterpret_cast<const uint8_t *>(image->getData()),
                            image->getImageDataSizeInBytes()));

    auto entries = m_chan.m_aux_map_builder.initEntries(aux.size());
    auto entry_it = entries.begin();
    for (const auto& [key, value] : aux) {
      entry_it->setKey(key);
      entry_it->setValue(value);
	    ++entry_it;
    }
    m_chan.m_has_obs = true;
    m_chan.m_obs_cv.notify_one();
  }
}
