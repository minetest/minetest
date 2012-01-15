/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "content_cao.h"

/*
	MobV2CAO
*/

class MobV2CAO : public ClientActiveObject
{
public:

	MobV2CAO(IGameDef *gamedef, ClientEnvironment *env):
		ClientActiveObject(0, gamedef, env),
		m_selection_box(-0.4*BS,-0.4*BS,-0.4*BS, 0.4*BS,0.8*BS,0.4*BS),
		m_node(NULL),
		m_position(v3f(0,10*BS,0)),
		m_yaw(0),
		m_walking(false),
		m_walking_unset_timer(0),
		m_walk_timer(0),
		m_walk_frame(0),
		m_damage_visual_timer(0),
		m_last_light(0),
		m_shooting(0),
		m_shooting_unset_timer(0),
		m_sprite_size(BS,BS),
		m_sprite_y(0),
		m_bright_shooting(false),
		m_lock_full_brightness(false),
		m_player_hit_timer(0)
	{
		ClientActiveObject::registerType(getType(), create);

		m_properties = new Settings;
	}

	~MobV2CAO()
	{
		delete m_properties;
	}

	inline u8 getType() const	{	return ACTIVEOBJECT_TYPE_MOBV2;	}

	static ClientActiveObject* create(IGameDef *gamedef, ClientEnvironment *env)
	{
		return new MobV2CAO(gamedef, env);
	}

	void addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
				IrrlichtDevice *irr)
	{
		if(m_node != NULL)
			return;

		/*infostream<<"MobV2CAO::addToScene using texture_name="<<
				m_texture_name<<std::endl;*/
		std::string texture_string = m_texture_name +
				"^[makealpha:128,0,0^[makealpha:128,128,0";

		scene::IBillboardSceneNode *bill = smgr->addBillboardSceneNode(
				NULL, v2f(1, 1), v3f(0,0,0), -1);
		bill->setMaterialTexture(0, tsrc->getTextureRaw(texture_string));
		bill->setMaterialFlag(video::EMF_LIGHTING, false);
		bill->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
		bill->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF);
		bill->setMaterialFlag(video::EMF_FOG_ENABLE, true);
		bill->setColor(video::SColor(255,0,0,0));
		bill->setVisible(false); /* Set visible when brightness is known */
		bill->setSize(m_sprite_size);
		if(m_sprite_type == "humanoid_1"){
			const float txp = 1./192;
			const float txs = txp*32;
			const float typ = 1./240;
			const float tys = typ*48;
			setBillboardTextureMatrix(bill, txs, tys, 0, 0);
		} else if(m_sprite_type == "simple"){
			const float txs = 1.0;
			const float tys = 1.0 / m_simple_anim_frames;
			setBillboardTextureMatrix(bill, txs, tys, 0, 0);
		} else {
			infostream<<"MobV2CAO: Unknown sprite type \""<<m_sprite_type<<"\""
					<<std::endl;
		}

		m_node = bill;

		updateNodePos();
	}

	void removeFromScene()
	{
		if(m_node == NULL)
			return;

		m_node->remove();
		m_node = NULL;
	}

	void updateLight(u8 light_at_pos)
	{
		if(m_lock_full_brightness)
			light_at_pos = 15;

		m_last_light = light_at_pos;

		if(m_node == NULL)
			return;

		if(m_damage_visual_timer > 0)
			return;

		if(m_shooting && m_bright_shooting)
			return;

		/*if(light_at_pos <= 2){
			m_node->setVisible(false);
			return;
		}*/

		m_node->setVisible(true);

		u8 li = decode_light(light_at_pos);
		video::SColor color(255,li,li,li);
		m_node->setColor(color);
	}

	v3s16 getLightPosition()
	{
		return floatToInt(m_position+v3f(0,0,0), BS);
	}

	void updateNodePos()
	{
		if(m_node == NULL)
			return;

		m_node->setPosition(pos_translator.vect_show + v3f(0,m_sprite_y,0));
	}

	void step(float dtime, ClientEnvironment *env)
	{
		scene::IBillboardSceneNode *bill = m_node;
		if(!bill)
			return;

		pos_translator.translate(dtime);

		if(m_sprite_type == "humanoid_1"){
			scene::ICameraSceneNode* camera = m_node->getSceneManager()->getActiveCamera();
			if(!camera)
				return;
			v3f cam_to_mob = m_node->getAbsolutePosition() - camera->getAbsolutePosition();
			cam_to_mob.normalize();
			int col = 0;
			if(cam_to_mob.Y > 0.75)
				col = 5;
			else if(cam_to_mob.Y < -0.75)
				col = 4;
			else{
				float mob_dir = atan2(cam_to_mob.Z, cam_to_mob.X) / PI * 180.;
				float dir = mob_dir - m_yaw;
				dir = wrapDegrees_180(dir);
				//infostream<<"id="<<m_id<<" dir="<<dir<<std::endl;
				if(fabs(wrapDegrees_180(dir - 0)) <= 45.1)
					col = 2;
				else if(fabs(wrapDegrees_180(dir - 90)) <= 45.1)
					col = 3;
				else if(fabs(wrapDegrees_180(dir - 180)) <= 45.1)
					col = 0;
				else if(fabs(wrapDegrees_180(dir + 90)) <= 45.1)
					col = 1;
				else
					col = 4;
			}

			int row = 0;
			if(m_shooting){
				row = 3;
			} else if(m_walking){
				m_walk_timer += dtime;
				if(m_walk_timer >= 0.5){
					m_walk_frame = (m_walk_frame + 1) % 2;
					m_walk_timer = 0;
				}
				if(m_walk_frame == 0)
					row = 1;
				else
					row = 2;
			}

			const float txp = 1./192;
			const float txs = txp*32;
			const float typ = 1./240;
			const float tys = typ*48;
			setBillboardTextureMatrix(bill, txs, tys, col, row);
		} else if(m_sprite_type == "simple"){
			m_walk_timer += dtime;
			if(m_walk_timer >= m_simple_anim_frametime){
				m_walk_frame = (m_walk_frame + 1) % m_simple_anim_frames;
				m_walk_timer = 0;
			}
			int col = 0;
			int row = m_walk_frame;
			const float txs = 1.0;
			const float tys = 1.0 / m_simple_anim_frames;
			setBillboardTextureMatrix(bill, txs, tys, col, row);
		} else {
			infostream<<"MobV2CAO::step(): Unknown sprite type \""
					<<m_sprite_type<<"\""<<std::endl;
		}

		updateNodePos();

		/* Damage local player */
		if(m_player_hit_damage && m_player_hit_timer <= 0.0){
			LocalPlayer *player = env->getLocalPlayer();
			assert(player);

			v3f playerpos = player->getPosition();
			v2f playerpos_2d(playerpos.X,playerpos.Z);
			v2f objectpos_2d(m_position.X,m_position.Z);

			if(fabs(m_position.Y - playerpos.Y) < m_player_hit_distance*BS &&
			objectpos_2d.getDistanceFrom(playerpos_2d) < m_player_hit_distance*BS)
			{
				env->damageLocalPlayer(m_player_hit_damage);
				m_player_hit_timer = m_player_hit_interval;
			}
		}

		/* Run timers */

		m_player_hit_timer -= dtime;

		if(m_damage_visual_timer >= 0){
			m_damage_visual_timer -= dtime;
			if(m_damage_visual_timer <= 0){
				infostream<<"id="<<m_id<<" damage visual ended"<<std::endl;
			}
		}

		m_walking_unset_timer += dtime;
		if(m_walking_unset_timer >= 1.0){
			m_walking = false;
		}

		m_shooting_unset_timer -= dtime;
		if(m_shooting_unset_timer <= 0.0){
			if(m_bright_shooting){
				u8 li = decode_light(m_last_light);
				video::SColor color(255,li,li,li);
				bill->setColor(color);
				m_bright_shooting = false;
			}
			m_shooting = false;
		}

	}

	void processMessage(const std::string &data)
	{
		//infostream<<"MobV2CAO: Got message"<<std::endl;
		std::istringstream is(data, std::ios::binary);
		// command
		u8 cmd = readU8(is);

		// Move
		if(cmd == AO_Message_type::SetPosition)
		{
			// pos
			m_position = readV3F1000(is);
			pos_translator.update(m_position);
			// yaw
			m_yaw = readF1000(is);

			m_walking = true;
			m_walking_unset_timer = 0;

			updateNodePos();
		}
		// Damage
		else if(cmd == AO_Message_type::TakeDamage)
		{
			//u16 damage = readU16(is);

			/*u8 li = decode_light(m_last_light);
			if(li >= 100)
				li = 30;
			else
				li = 255;*/

			/*video::SColor color(255,255,0,0);
			m_node->setColor(color);

			m_damage_visual_timer = 0.2;*/
		}
		// Trigger shooting
		else if(cmd == AO_Message_type::Shoot)
		{
			// length
			m_shooting_unset_timer = readF1000(is);
			// bright?
			m_bright_shooting = readU8(is);
			if(m_bright_shooting){
				u8 li = 255;
				video::SColor color(255,li,li,li);
				m_node->setColor(color);
			}

			m_shooting = true;
		}
	}

	void initialize(const std::string &data)
	{
		//infostream<<"MobV2CAO: Got init data"<<std::endl;

		{
			std::istringstream is(data, std::ios::binary);
			// version
			u8 version = readU8(is);
			// check version
			if(version != 0){
				infostream<<__FUNCTION_NAME<<": Invalid version"<<std::endl;
				return;
			}

			std::ostringstream tmp_os(std::ios::binary);
			decompressZlib(is, tmp_os);
			std::istringstream tmp_is(tmp_os.str(), std::ios::binary);
			m_properties->parseConfigLines(tmp_is, "MobArgsEnd");

			infostream<<"MobV2CAO::initialize(): got properties:"<<std::endl;
			m_properties->writeLines(infostream);

			m_properties->setDefault("looks", "dummy_default");
			m_properties->setDefault("yaw", "0");
			m_properties->setDefault("pos", "(0,0,0)");
			m_properties->setDefault("player_hit_damage", "0");
			m_properties->setDefault("player_hit_distance", "1.5");
			m_properties->setDefault("player_hit_interval", "1.5");

			setLooks(m_properties->get("looks"));
			m_yaw = m_properties->getFloat("yaw");
			m_position = m_properties->getV3F("pos");
			m_player_hit_damage = m_properties->getS32("player_hit_damage");
			m_player_hit_distance = m_properties->getFloat("player_hit_distance");
			m_player_hit_interval = m_properties->getFloat("player_hit_interval");

			pos_translator.init(m_position);
		}

		updateNodePos();
	}

	core::aabbox3d<f32>* getSelectionBox()
		{return &m_selection_box;}

	v3f getPosition()
		{return pos_translator.vect_show;}

	bool directReportPunch(const std::string &toolname, v3f dir)
	{
		video::SColor color(255,255,0,0);
		m_node->setColor(color);

		m_damage_visual_timer = 0.05;

		m_position += dir * BS;
		pos_translator.sharpen();
		pos_translator.update(m_position);
		updateNodePos();

		return false;
	}

private:
	void setLooks(const std::string &looks)
		{
			v2f selection_size = v2f(0.4, 0.4) * BS;
			float selection_y = 0 * BS;

			if(looks == "dungeon_master"){
				m_texture_name = "dungeon_master.png";
				m_sprite_type = "humanoid_1";
				m_sprite_size = v2f(2, 3) * BS;
				m_sprite_y = 0.85 * BS;
				selection_size = v2f(0.4, 2.6) * BS;
				selection_y = -0.4 * BS;
			}
			else if(looks == "fireball"){
				m_texture_name = "fireball.png";
				m_sprite_type = "simple";
				m_sprite_size = v2f(1, 1) * BS;
				m_simple_anim_frames = 3;
				m_simple_anim_frametime = 0.1;
				m_lock_full_brightness = true;
			}
			else{
				m_texture_name = "stone.png";
				m_sprite_type = "simple";
				m_sprite_size = v2f(1, 1) * BS;
				m_simple_anim_frames = 3;
				m_simple_anim_frametime = 0.333;
				selection_size = v2f(0.4, 0.4) * BS;
				selection_y = 0 * BS;
			}

			m_selection_box = core::aabbox3d<f32>(
					-selection_size.X, selection_y, -selection_size.X,
					selection_size.X, selection_y+selection_size.Y,
					selection_size.X);
		}

	IntervalLimiter m_attack_interval;
	core::aabbox3d<f32> m_selection_box;
	scene::IBillboardSceneNode *m_node;
	v3f m_position;
	std::string m_texture_name;
	float m_yaw;
	SmoothTranslator pos_translator;
	bool m_walking;
	float m_walking_unset_timer;
	float m_walk_timer;
	int m_walk_frame;
	float m_damage_visual_timer;
	u8 m_last_light;
	bool m_shooting;
	float m_shooting_unset_timer;
	v2f m_sprite_size;
	float m_sprite_y;
	bool m_bright_shooting;
	std::string m_sprite_type;
	int m_simple_anim_frames;
	float m_simple_anim_frametime;
	bool m_lock_full_brightness;
	int m_player_hit_damage;
	float m_player_hit_distance;
	float m_player_hit_interval;
	float m_player_hit_timer;

	Settings *m_properties;
};

// Prototype
MobV2CAO proto_MobV2CAO(NULL, NULL);
