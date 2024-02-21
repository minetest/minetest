#include "remoteinputhandler.h"
#include "client/keycode.h"
#include "hud.h"
#include "remoteclient.capnp.h"

#include <cassert>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sstream>

#include <capnp/blob.h>
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <kj/array.h>
#include <zmqpp/zmqpp.hpp>


RemoteInputHandler::RemoteInputHandler(const std::string &endpoint,
                                       RenderingEngine *rendering_engine,
                                       MyEventReceiver *receiver)
    : m_rendering_engine(rendering_engine), m_context(),
      m_socket(m_context, zmqpp::socket_type::reply), m_receiver(receiver) {
  infostream << "RemoteInputHandler: Binding to " << endpoint << '\n';
  m_socket.bind(endpoint);
  // wait for client
  zmqpp::message message;
  m_socket.receive(message);
  // TODO(https://github.com/Astera-org/minetest/issues/22):
  // extract Action deserialization into a helper function.
  std::string data;
  for(size_t part = 0; part < message.parts(); ++part) {
      data += message.get(part);
  }
  kj::ArrayPtr<const capnp::word> words(
      reinterpret_cast<const capnp::word *>(data.data()),
      data.size() / sizeof(capnp::word));

  capnp::FlatArrayMessageReader reader(words);
  auto action = reader.getRoot<Action>();
  if (action.hasKeyEvents()) {
    throw std::runtime_error(
        "INVALID HANDSHAKE: Got key events in handshake, expected "
        "'action.hasKeyEvents() == false'");
  }
};

void RemoteInputHandler::step(float dtime) {
  // skip first loop, because we don't have an observation yet
  // as draw is called after step
  if (m_is_first_loop) {
    m_is_first_loop = false;
    return;
  }

  // send current observation
  irr::video::IImage *image;
  if (m_rendering_engine->headless) {
    image = m_rendering_engine->get_screenshot();
  } else {
    irr::video::IVideoDriver *driver = m_rendering_engine->get_video_driver();
    image = driver->createScreenShot(video::ECF_R8G8B8);
  }

  ::capnp::MallocMessageBuilder obs_msg_builder;
  Observation::Builder obs_builder = obs_msg_builder.initRoot<Observation>();

  // parse reward from hud
  // during game startup, the hud is not yet initialized, so there'll be no
  // reward for the first 1-2 steps
  assert(m_player && "Player is null");
  for (u32 i = 0; i < m_player->maxHudId(); ++i) {
    auto hud_element = m_player->getHud(i);
    if (hud_element->name == "reward") {
      // parse 'Reward: <reward>' from hud
      constexpr char kRewardHUDPrefix[] = "Reward: ";
      std::string_view reward_string = hud_element->text;
      reward_string.remove_prefix(std::size(kRewardHUDPrefix) - 1); // -1 for null terminator
      // I'd rather use std::from_chars, but it's not available in libc++ yet.
      std::stringstream ss{std::string(reward_string)};
      ss >> reward;
      break;
    }
  }

  obs_builder.initImage();
  auto image_builder = obs_builder.getImage();
  image_builder.setWidth(image->getDimension().Width);
  image_builder.setHeight(image->getDimension().Height);
  image_builder.setData(
      capnp::Data::Reader(reinterpret_cast<const uint8_t *>(image->getData()),
                          image->getImageDataSizeInBytes()));

  auto msg_data = capnp::messageToFlatArray(obs_msg_builder);
  zmqpp::message obs_msg;
  obs_msg.add_raw(msg_data.begin(), msg_data.size() * sizeof(capnp::word));
  m_socket.send(obs_msg);

  if (image)
    image->drop();

  // receive next key
  zmqpp::message message;
  // TODO(https://github.com/Astera-org/minetest/issues/22):
  // extract Action deserialization into a helper function.
  m_socket.receive(message);

  std::string data;
  for(size_t part = 0; part < message.parts(); ++part) {
      data += message.get(part);
  }
  kj::ArrayPtr<const capnp::word> words(
      reinterpret_cast<const capnp::word *>(data.data()),
      data.size() / sizeof(capnp::word));

  capnp::FlatArrayMessageReader reader(words);
  Action::Reader action = reader.getRoot<Action>();

  // We don't model key release events, keys need to be re-pressed every step.
  // Rationale: there's only one place in the engine were keyRelease events are
  // used, and it doesn't seem important.
  clearInput();

  KeyPress new_key_code;

  for (auto keyEvent : action.getKeyEvents()) {
    new_key_code = keycache.key[static_cast<int>(keyEvent)];
    if (!m_key_is_down[new_key_code]) {
      m_key_was_pressed.set(new_key_code);
    }
    m_key_is_down.set(new_key_code);
    m_key_was_down.set(new_key_code);
  }
  m_mouse_speed = v2s32(action.getMouseDx(), action.getMouseDy());
  // mousepos is reset to (WIDTH/2, HEIGHT/2) after every iteration of main game
  // loop unit is pixels, origin is top left corner, bounds is (0,0) to (WIDTH,
  // HEIGHT)
  m_mouse_pos += m_mouse_speed;
};

void RemoteInputHandler::clearInput() {
  m_key_is_down.clear();
  m_key_was_pressed.clear();
  m_mouse_wheel = 0;
}
