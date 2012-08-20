#include "global.hpp"
#include "artifical.hpp"

#include "actions.hpp"
#include "pathutils.hpp"
#include "game_display.hpp"
#include "map.hpp"
#include "halo.hpp"
#include "game_preferences.hpp"
#include "resources.hpp"
#include "foreach.hpp"
#include "unit_display.hpp"
#include "play_controller.hpp"
#include "gettext.hpp"
#include "game_events.hpp"
#include "formula_string_utils.hpp"
#include "replay.hpp"

artifical::artifical(const config& cfg) :
	unit(*resources::units, *resources::heros, cfg, true, 0, true)
	, map_(*resources::game_map)
	, reside_troops_()
	, field_troops_()
	, field_arts_()
	, fresh_heros_()
	, finish_heros_()
	, wander_heros_()
	, economy_area_()
	, district_()
	, district_locs_()
	, terrain_types_list_(t_translation::t_match(type()->match_).terrain)
	, alert_rect_()	
	, mayor_(&hero_invalid)
	, fronts_(0)
{
	// ����[city]���ֶ�
	read(cfg);

	if (this_is_city()) {
		touch_dirs_.insert(map_location::NORTH);
		touch_dirs_.insert(map_location::NORTH_EAST);
		touch_dirs_.insert(map_location::SOUTH_EAST);
		touch_dirs_.insert(map_location::SOUTH);
		touch_dirs_.insert(map_location::SOUTH_WEST);
		touch_dirs_.insert(map_location::NORTH_WEST);
	}
}

artifical::artifical(const uint8_t* mem) :
	unit(*resources::units, *resources::heros, mem, true, 0, true)
	, map_(*resources::game_map)
	, reside_troops_()
	, field_troops_()
	, field_arts_()
	, fresh_heros_()
	, finish_heros_()
	, wander_heros_()
	, economy_area_()
	, district_()
	, district_locs_()
	, terrain_types_list_(t_translation::t_match(type()->match_).terrain)
	, alert_rect_()	
	, mayor_(&hero_invalid)
	, fronts_(0)
{
	// ����[city]���ֶ�
	read(mem);

	if (this_is_city()) {
		touch_dirs_.insert(map_location::NORTH);
		touch_dirs_.insert(map_location::NORTH_EAST);
		touch_dirs_.insert(map_location::SOUTH_EAST);
		touch_dirs_.insert(map_location::SOUTH);
		touch_dirs_.insert(map_location::SOUTH_WEST);
		touch_dirs_.insert(map_location::NORTH_WEST);
	}
}

artifical::artifical(const artifical& cobj) :
	unit(cobj)
	, map_(*resources::game_map)
	, reside_troops_(cobj.reside_troops_)
	, field_troops_(cobj.field_troops_)
	, field_arts_(cobj.field_arts_)
	, fresh_heros_(cobj.fresh_heros_)
	, finish_heros_(cobj.finish_heros_)
	, wander_heros_(cobj.wander_heros_)
	, economy_area_(cobj.economy_area_)
	, district_(cobj.district_)
	, district_locs_(cobj.district_locs_)
	, terrain_types_list_(cobj.terrain_types_list_)
	, alert_rect_(cobj.alert_rect_)
	, mayor_(cobj.mayor_)
	, fronts_(cobj.fronts_)
{
}

artifical::artifical(unit_map& units, hero_map& heros, type_heros_pair& t, int cityno, bool use_traits) :
	unit(units, heros, t, cityno, use_traits, true)
	, map_(*resources::game_map)
	, reside_troops_()
	, field_troops_()
	, field_arts_()
	, fresh_heros_()
	, finish_heros_()
	, wander_heros_()
	, economy_area_()
	, district_()
	, district_locs_()
	, terrain_types_list_(t_translation::t_match(type()->match_).terrain)
	, alert_rect_()
	, mayor_(&hero_invalid)
	, fronts_(0)
{
	if (this_is_city()) {
		touch_dirs_.insert(map_location::NORTH);
		touch_dirs_.insert(map_location::NORTH_EAST);
		touch_dirs_.insert(map_location::SOUTH_EAST);
		touch_dirs_.insert(map_location::SOUTH);
		touch_dirs_.insert(map_location::SOUTH_WEST);
		touch_dirs_.insert(map_location::NORTH_WEST);
	}
}

void artifical::read(const config& cfg, bool use_traits, game_state* state)
{
	// ��δ���佫
	const std::vector<std::string> fresh_heros = utils::split(cfg_["service_heros"]);
	std::vector<std::string>::const_iterator tmp;
	for (tmp = fresh_heros.begin(); tmp != fresh_heros.end(); ++ tmp) {
		fresh_heros_.push_back(&heros_[lexical_cast_default<int>(*tmp)]);
		fresh_heros_.back()->status_ = hero_status_idle;
		fresh_heros_.back()->city_ = cityno_;
		fresh_heros_.back()->side_ = side_ - 1;
	}

	// ����Ұ���佫
	const std::vector<std::string> wander_heros = utils::split(cfg_["wander_heros"]);
	for (tmp = wander_heros.begin(); tmp != wander_heros.end(); ++ tmp) {
		wander_heros_.push_back(&heros_[lexical_cast_default<int>(*tmp)]);
		wander_heros_.back()->status_ = hero_status_wander;
		wander_heros_.back()->city_ = cityno_;
	}

	// ���ꡱ�佫(�������佫,״̬(status_)�ѱ�����)
	const std::vector<std::string> finish_heros = utils::split(cfg_["finish_heros"]);
	for (tmp = finish_heros.begin(); tmp != finish_heros.end(); ++ tmp) {
		finish_heros_.push_back(&heros_[lexical_cast_default<int>(*tmp)]);
	}

	// economy_area
	// ���parenthetical_split(cfg_["economy_area"], ','), �����������ж��ǻ�ʹ��economy_area�������,��, ������ʡȥȥ��()�鷳
	const std::vector<std::string> economy_area = utils::parenthetical_split(cfg_["economy_area"]);
	for (std::vector<std::string>::const_iterator itor = economy_area.begin(); itor != economy_area.end(); ++ itor) {
		const std::vector<std::string> loc_str = utils::split(*itor);
		if (loc_str.size() == 2) {
			const map_location loc(lexical_cast_default<int>(loc_str[0]) - 1, lexical_cast_default<int>(loc_str[1]) - 1);
			economy_area_.push_back(loc);
			unit_map::economy_areas_[loc] = cityno_;
		}
	}

	// district
	const std::vector<std::string> district = utils::parenthetical_split(cfg_["district"]);
	SDL_Rect rc;
	std::stringstream err;
	for (std::vector<std::string>::const_iterator itor = district.begin(); itor != district.end(); ++ itor) {
		const std::vector<std::string> loc_str = utils::split(*itor);
		if (loc_str.size() == 4) {
			rc.x = lexical_cast_default<int>(loc_str[0]) - 1;
			if (rc.x < 0 || rc.x > map_.w()) {
				err << "invalid coordinate x: " << loc_str[0] << ", on city " << master_->name();
				throw game::load_game_failed(err.str());
			}
			rc.y = lexical_cast_default<int>(loc_str[1]) - 1;
			if (rc.y < 0 || rc.y > map_.h()) {
				err << "invalid coordinate y: " << loc_str[1] << ", on city " << master_->name();
				throw game::load_game_failed(err.str());
			}
			rc.w = lexical_cast_default<int>(loc_str[2]);
			if (rc.w < 0 || rc.x + rc.w > map_.w()) {
				err << "invalid width: " << loc_str[2] << "(map width: " << map_.w() << "), on city " << master_->name();
				throw game::load_game_failed(err.str());
			}
			rc.h = lexical_cast_default<int>(loc_str[3]);
			if (rc.h < 0 || rc.y + rc.h > map_.h()) {
				err << "invalid height: " << loc_str[3] << "(map height: " << map_.h() << "), on city " << master_->name();
				throw game::load_game_failed(err.str());
			}
			// parse std::vector<SDL_Rect> to std::set<map_location>
			for (int x = 0; x < rc.w; x ++) {
				for (int y = 0; y < rc.h; y ++) {
					district_locs_.insert(map_location(rc.x + x, rc.y + y));
				}
			}
			district_.push_back(rc);
		}
	}

	if (this_is_city()) {
		if (cfg.has_attribute("mayor")) {
			mayor_ = &heros_[cfg["mayor"].to_int()];
			mayor_->official_ = hero_official_mayor;
		}
		fronts_ = cfg["fronts"].to_int();
	}

	// ������������unit�������佫�б�
	foreach (config &i, cfg_.child_range("unit")) {
		// i["side"] = cfg_["side"];
		reside_troops_.push_back(unit(units_, heros_, i));
		// fresh_heros_.push_back(&unit_list_.back().master());
	}
}

void artifical::write(config& cfg) const
{
	unit::write(cfg);

	// ��δ���佫
	std::stringstream str;
	for (std::vector<hero*>::const_iterator h = fresh_heros_.begin(); h != fresh_heros_.end(); ++ h) {
		if (h != fresh_heros_.begin()) {
			str << ", ";
		}
		str << lexical_cast<std::string>((*h)->number_);
	}
	cfg["service_heros"] = str.str();

	// ����Ұ���佫
	str.str("");
	for (std::vector<hero*>::const_iterator h = wander_heros_.begin(); h != wander_heros_.end(); ++ h) {
		if (h != wander_heros_.begin()) {
			str << ", ";
		}
		str << lexical_cast<std::string>((*h)->number_);
	}
	cfg["wander_heros"] = str.str();

	// ���ꡱ�佫
	str.str("");
	for (std::vector<hero*>::const_iterator h = finish_heros_.begin(); h != finish_heros_.end(); ++ h) {
		if (h != finish_heros_.begin()) {
			str << ", ";
		}
		str << lexical_cast<std::string>((*h)->number_);
	}
	cfg["finish_heros"] = str.str();

	str.str("");
	// economy area
	for (std::vector<map_location>::const_iterator itor = economy_area_.begin(); itor != economy_area_.end(); ++ itor) {
		str << "(" << lexical_cast<std::string>(itor->x + 1) << "," << lexical_cast<std::string>(itor->y + 1) << ")";
	}
	cfg["economy_area"] = str.str();

	str.str("");
	// district
	for (std::vector<SDL_Rect>::const_iterator itor = district_.begin(); itor != district_.end(); ++ itor) {
		str << "(" << lexical_cast<std::string>(itor->x + 1) << "," << lexical_cast<std::string>(itor->y + 1) << "," << lexical_cast<std::string>(itor->w) << "," << lexical_cast<std::string>(itor->h) << ")";
	}
	cfg["district"] = str.str();

	if (this_is_city()) {
		cfg["fronts"] = fronts_;
		cfg["mayor"] = mayor_->number_;
	}

	for (std::vector<unit>::const_iterator iter = reside_troops_.begin(); iter != reside_troops_.end(); iter ++) {
		config& ucfg = cfg.add_child("unit");
		iter->write(ucfg);
	}
}

void artifical::write(uint8_t* mem) const
{
	unit::write(mem);
	artifical_fields_t* fields = (artifical_fields_t*)mem;

	std::stringstream str;
	int offset = fields->size_;
	int val;

	fields->mayor_ = mayor_->number_;
	fields->fronts_ = fronts_;

	str.str("");
	// fresh_heros
	for (std::vector<hero*>::const_iterator h = fresh_heros_.begin(); h != fresh_heros_.end(); ++ h) {
		if (h != fresh_heros_.begin()) {
			str << ", ";
		}
		val = (*h)->number_;
		str << val;
	}
	fields->fresh_heros_.offset_ = offset;
	fields->fresh_heros_.size_ = str.str().length();
	memcpy(mem + offset, str.str().c_str(), fields->fresh_heros_.size_);
	offset += fields->fresh_heros_.size_;

	str.str("");
	// wander_heros
	for (std::vector<hero*>::const_iterator h = wander_heros_.begin(); h != wander_heros_.end(); ++ h) {
		if (h != wander_heros_.begin()) {
			str << ", ";
		}
		val = (*h)->number_;
		str << val;
	}
	fields->wander_heros_.offset_ = offset;
	fields->wander_heros_.size_ = str.str().length();
	memcpy(mem + offset, str.str().c_str(), fields->wander_heros_.size_);
	offset += fields->wander_heros_.size_;

	str.str("");
	// finish_heros
	for (std::vector<hero*>::const_iterator h = finish_heros_.begin(); h != finish_heros_.end(); ++ h) {
		if (h != finish_heros_.begin()) {
			str << ", ";
		}
		val = (*h)->number_;
		str << val;
	}
	fields->finish_heros_.offset_ = offset;
	fields->finish_heros_.size_ = str.str().length();
	memcpy(mem + offset, str.str().c_str(), fields->finish_heros_.size_);
	offset += fields->finish_heros_.size_;

	str.str("");
	// economy area
	for (std::vector<map_location>::const_iterator itor = economy_area_.begin(); itor != economy_area_.end(); ++ itor) {
		str << "(" << lexical_cast<std::string>(itor->x + 1) << "," << lexical_cast<std::string>(itor->y + 1) << ")";
	}
	fields->economy_area_.offset_ = offset;
	fields->economy_area_.size_ = str.str().length();
	memcpy(mem + offset, str.str().c_str(), fields->economy_area_.size_);
	offset += fields->economy_area_.size_;

	str.str("");
	// district
	for (std::vector<SDL_Rect>::const_iterator itor = district_.begin(); itor != district_.end(); ++ itor) {
		str << "(" << lexical_cast<std::string>(itor->x + 1) << "," << lexical_cast<std::string>(itor->y + 1) << "," << lexical_cast<std::string>(itor->w) << "," << lexical_cast<std::string>(itor->h) << ")";
	}
	fields->district_.offset_ = offset;
	fields->district_.size_ = str.str().length();
	memcpy(mem + offset, str.str().c_str(), fields->district_.size_);
	offset += fields->district_.size_;

	// align 4
	offset = (offset + 3) & ~3;

	fields->reside_troops_.offset_ = offset;
	fields->reside_troops_.size_ = reside_troops_.size();
	for (std::vector<unit>::const_iterator iter = reside_troops_.begin(); iter != reside_troops_.end(); iter ++) {
		iter->write(mem + offset);
		unit_fields_t* fields = (unit_fields_t*)(mem + offset);
		offset += fields->size_;
	}

	fields->size_ = offset;
}

void artifical::read(const uint8_t* mem)
{
	std::string str;
	artifical_fields_t* fields = (artifical_fields_t*)mem;
	
	mayor_ = &heros_[fields->mayor_];
	fronts_ = fields->fronts_;

	// fresh_heros_
	str.assign((const char*)mem + fields->fresh_heros_.offset_, fields->fresh_heros_.size_);
	const std::vector<std::string> fresh_heros = utils::split(str);
	std::vector<std::string>::const_iterator tmp;
	for (tmp = fresh_heros.begin(); tmp != fresh_heros.end(); ++ tmp) {
		fresh_heros_.push_back(&heros_[lexical_cast_default<int>(*tmp)]);
		fresh_heros_.back()->status_ = hero_status_idle;
		fresh_heros_.back()->city_ = cityno_;
		fresh_heros_.back()->side_ = side_ - 1;
	}

	// wander_heros_
	str.assign((const char*)mem + fields->wander_heros_.offset_, fields->wander_heros_.size_);
	const std::vector<std::string> wander_heros = utils::split(str);
	for (tmp = wander_heros.begin(); tmp != wander_heros.end(); ++ tmp) {
		wander_heros_.push_back(&heros_[lexical_cast_default<int>(*tmp)]);
		wander_heros_.back()->status_ = hero_status_wander;
		wander_heros_.back()->city_ = cityno_;
	}

	// finish_heros_(to finish heros, status_ has bee set)
	str.assign((const char*)mem + fields->finish_heros_.offset_, fields->finish_heros_.size_);
	const std::vector<std::string> finish_heros = utils::split(str);
	for (tmp = finish_heros.begin(); tmp != finish_heros.end(); ++ tmp) {
		finish_heros_.push_back(&heros_[lexical_cast_default<int>(*tmp)]);
	}

	// economy_area
	// ���parenthetical_split(cfg_["economy_area"], ','), �����������ж��ǻ�ʹ��economy_area�������,��, ������ʡȥȥ��()�鷳
	str.assign((const char*)mem + fields->economy_area_.offset_, fields->economy_area_.size_);
	const std::vector<std::string> economy_area = utils::parenthetical_split(str);
	for (std::vector<std::string>::const_iterator itor = economy_area.begin(); itor != economy_area.end(); ++ itor) {
		const std::vector<std::string> loc_str = utils::split(*itor);
		if (loc_str.size() == 2) {
			const map_location loc(lexical_cast_default<int>(loc_str[0]) - 1, lexical_cast_default<int>(loc_str[1]) - 1);
			economy_area_.push_back(loc);
			unit_map::economy_areas_[loc] = cityno_;
		}
	}

	// district
	str.assign((const char*)mem + fields->district_.offset_, fields->district_.size_);
	const std::vector<std::string> district = utils::parenthetical_split(str);
	SDL_Rect rc;
	std::stringstream err;
	for (std::vector<std::string>::const_iterator itor = district.begin(); itor != district.end(); ++ itor) {
		const std::vector<std::string> loc_str = utils::split(*itor);
		if (loc_str.size() == 4) {
			rc.x = lexical_cast_default<int>(loc_str[0]) - 1;
			if (rc.x < 0 || rc.x > map_.w()) {
				err << "invalid coordinate x: " << loc_str[0] << ", on city " << master_->name();
				throw game::load_game_failed(err.str());
			}
			rc.y = lexical_cast_default<int>(loc_str[1]) - 1;
			if (rc.y < 0 || rc.y > map_.h()) {
				err << "invalid coordinate y: " << loc_str[1] << ", on city " << master_->name();
				throw game::load_game_failed(err.str());
			}
			rc.w = lexical_cast_default<int>(loc_str[2]);
			if (rc.w < 0 || rc.x + rc.w > map_.w()) {
				err << "invalid width: " << loc_str[2] << "(map width: " << map_.w() << "), on city " << master_->name();
				throw game::load_game_failed(err.str());
			}
			rc.h = lexical_cast_default<int>(loc_str[3]);
			if (rc.h < 0 || rc.y + rc.h > map_.h()) {
				err << "invalid height: " << loc_str[3] << "(map height: " << map_.h() << "), on city " << master_->name();
				throw game::load_game_failed(err.str());
			}
			// parse std::vector<SDL_Rect> to std::set<map_location>
			for (int x = 0; x < rc.w; x ++) {
				for (int y = 0; y < rc.h; y ++) {
					district_locs_.insert(map_location(rc.x + x, rc.y + y));
				}
			}
			district_.push_back(rc);
		}
	}

	// ������������unit�������佫�б�
	int offset = fields->reside_troops_.offset_;
	for (int i = 0; i < fields->reside_troops_.size_; i ++) {
		reside_troops_.push_back(unit(units_, heros_, mem + offset));

		unit_fields_t* u_fields = (unit_fields_t*)(mem + offset);
		offset += u_fields->size_;
	}
}

const std::set<map_location>& artifical::district_locs() const
{
	return district_locs_;
}

#define ALERT_RECT_RADIUS		10
void artifical::set_location(const map_location &loc)
{
	if (loc_ == loc) {
		return;
	}

	unit::set_location(loc);

	// recalculate alert-rectangle
	if (loc.x < ALERT_RECT_RADIUS) {
		alert_rect_.x = 0;
		alert_rect_.w = loc.x;
	} else {
		alert_rect_.x = loc.x - ALERT_RECT_RADIUS;
		alert_rect_.w = ALERT_RECT_RADIUS;
	}
	if (loc.x + ALERT_RECT_RADIUS >= map_.w()) {
		alert_rect_.w += map_.w() - 1 - loc.x;
	} else {
		alert_rect_.w += ALERT_RECT_RADIUS;
	}
	if (loc.y < ALERT_RECT_RADIUS) {
		alert_rect_.y = 0;
		alert_rect_.h = loc.y;
	} else {
		alert_rect_.y = loc.y - ALERT_RECT_RADIUS;
		alert_rect_.h = ALERT_RECT_RADIUS;
	}
	if (loc.y + ALERT_RECT_RADIUS >= map_.h()) {
		alert_rect_.h += map_.h() - 1 - loc.y;
	} else {
		alert_rect_.h += ALERT_RECT_RADIUS;
	}
	alert_rect_.w ++;
	alert_rect_.h ++;

	// reset loc_ of reside troops
	for (std::vector<unit>::iterator iter = reside_troops_.begin(); iter != reside_troops_.end(); iter ++) {
		iter->set_location(loc);
	}
	
	if (!touch_dirs_.empty()) {
		// �����漰������
		for (std::set<map_location::DIRECTION>::const_iterator itor = touch_dirs_.begin(); itor != touch_dirs_.end(); ++ itor) {
			map_location offset = loc.get_direction(*itor);
			if (!map_.on_board(offset)) {
				std::stringstream str;
				str << "A location of " << master_->name() << "(" << loc << ") is out of map!";
				throw game::load_game_failed(str.str());
			}
			touch_locs_.insert(offset);
		}
/*
		// multi-grid artifical's terrain must be water or falt. 
		for (std::set<map_location>::const_iterator i = touch_locs_.begin(); i != touch_locs_.end(); ++ i) {
			const std::string& group = map.get_terrain_info(map.get_terrain(*i)).editor_group();
			const std::vector<std::string> group_ver = utils::split(group);
			bool found = false;
			for (std::vector<std::string>::const_iterator g = group_ver.begin(); g != group_ver.end(); ++ g) {
				if (*g == "water" || *g == "flat") {
					found = true;
					break;
				}
			}
			if (!found) {
				std::stringstream str;
				str << "Editor group(" << group << ") of " << master_->name() << "(" << *i << ") isn't water or flat!";
				throw game::load_game_failed(str.str());
			}
		}
*/
		// recalculate adjacent grids
		map_offset* adjacent_ptr;
		size_t i, size;

		// range=1
		size = (sizeof(adjacent_2) / sizeof(map_offset)) >> 1;
		adjacent_ptr = adjacent_2[loc.x & 0x1];
		for (i = 0; i < size; i ++) {
			adjacent_[adjacent_size_].x = loc.x + adjacent_ptr[i].x;
			adjacent_[adjacent_size_].y = loc.y + adjacent_ptr[i].y;
			if (map_.on_board(adjacent_[adjacent_size_])) {
				adjacent_size_ ++;
			}
		}
		// range=2,3
		size = (sizeof(adjacent_3) / sizeof(map_offset)) >> 1;
		adjacent_ptr = adjacent_3[loc.x & 0x1];
		for (i = 0; i < size; i ++) {
			adjacent_3_[adjacent_size_3_].x = loc.x + adjacent_ptr[i].x;
			adjacent_3_[adjacent_size_3_].y = loc.y + adjacent_ptr[i].y;
			if (map_.on_board(adjacent_3_[adjacent_size_3_])) {
				adjacent_size_3_ ++;
			}
		}
		std::copy(adjacent_3_, adjacent_3_ + adjacent_size_3_, adjacent_2_);
		adjacent_size_2_ = adjacent_size_3_;
	}
}

void artifical::redraw_unit()
{
	game_display &disp = *game_display::get_singleton();
	const gamemap &map = disp.get_map();
	if (!loc_.valid() || hidden_ || disp.fogged(loc_) || (invisible(loc_) && disp.get_teams()[disp.viewing_team()].is_enemy(side()))) {
		clear_haloes();
		if(anim_) {
			anim_->update_last_draw_time();
		}
		return;
	}
	if(refreshing_) {
		return;
	}
	refreshing_ = true;

	std::vector<team>& teams = *resources::teams;
	team& t = teams[side_ - 1];
	bool expediting_from_it = ((disp.expedite_city() == this)? true: false) && t.is_human();

	if (!anim_) {
		set_standing();
	}
	anim_->update_last_draw_time();
	frame_parameters params;
	const t_translation::t_terrain terrain = map.get_terrain(loc_);
	const terrain_type& terrain_info = map.get_terrain_info(terrain);
	// do not set to 0 so we can distinguih the flying from the "not on submerge terrain"
	params.submerge= is_flying() ? 0.01 : terrain_info.unit_submerge();

	if (invisible(loc_) && params.highlight_ratio > 0.5) {
		params.highlight_ratio = 0.5;
	}
	if (loc_ == units_.center_loc(disp.selected_hex()) && params.highlight_ratio == 1.0) {
		params.highlight_ratio = 1.5;
	}
	int height_adjust = static_cast<int>(terrain_info.unit_height_adjust() * disp.get_zoom_factor());
	if (is_flying() && height_adjust < 0) {
		height_adjust = 0;
	}
	params.y -= height_adjust;
	params.halo_y -= height_adjust;
	if (get_state(STATE_POISONED)){
		params.blend_with = disp.rgb(0,255,0);
		params.blend_ratio = 0.25;
	}
	params.image_mod = image_mods();
	if (get_state(STATE_PETRIFIED)) params.image_mod +="~GS()";
	if (terrain_ == t_translation::NONE_TERRAIN) {
		params.image= absolute_image();
	}

	const frame_parameters adjusted_params = anim_->get_current_params(params);



	const map_location dst = loc_.get_direction(facing_);
	const int xsrc = disp.get_location_x(loc_);
	const int ysrc = disp.get_location_y(loc_);
	const int xdst = disp.get_location_x(dst);
	const int ydst = disp.get_location_y(dst);
	int d2 = disp.hex_size() / 2;




	const int x = static_cast<int>(adjusted_params.offset * xdst + (1.0-adjusted_params.offset) * xsrc) + d2;
	const int y = static_cast<int>(adjusted_params.offset * ydst + (1.0-adjusted_params.offset) * ysrc) + d2;


	if(unit_halo_ == halo::NO_HALO && !image_halo().empty()) {
		unit_halo_ = halo::add(0, 0, image_halo(), map_location(-1, -1));
	}
	if(unit_halo_ != halo::NO_HALO && image_halo().empty()) {
		halo::remove(unit_halo_);
		unit_halo_ = halo::NO_HALO;
	} else if(unit_halo_ != halo::NO_HALO) {
		halo::set_location(unit_halo_, x, y);
	}



	// We draw bars only if wanted, visible on the map view
	bool draw_bars = draw_bars_ ;
	if (draw_bars) {
		const int d = disp.hex_size();
		SDL_Rect unit_rect = {xsrc, ysrc +adjusted_params.y, d, d};
		draw_bars = rects_overlap(unit_rect, disp.map_outside_area());
	}

	if (!expediting_from_it && can_reside_) {
		std::stringstream str;
		str << fresh_heros_.size() + finish_heros_.size() << "(" << fresh_heros_.size() << ")/" << reside_troops_.size();
		if (display::default_zoom_ == display::ZOOM_48) {
			disp.draw_text_in_hex2(loc_, display::LAYER_BORDER, str.str(), 15, font::NORMAL_COLOR, xsrc + 24, ysrc - 22);
		} else if (display::default_zoom_ == display::ZOOM_56) {
			disp.draw_text_in_hex2(loc_, display::LAYER_BORDER, str.str(), 15, font::NORMAL_COLOR, xsrc + 27, ysrc - 26);
		} else if (display::default_zoom_ == display::ZOOM_64) {
			disp.draw_text_in_hex2(loc_, display::LAYER_BORDER, str.str(), 15, font::NORMAL_COLOR, xsrc + 30, ysrc - 30);
		} else {
			disp.draw_text_in_hex2(loc_, display::LAYER_BORDER, str.str(), 15, font::NORMAL_COLOR, xsrc + 32, ysrc - 34);
		}
	}

	if (!expediting_from_it && draw_bars) {
		std::string* energy_file;
		if (display::default_zoom_ == display::ZOOM_48) {
			energy_file = &game_config::images::bar_hrl_48;
		} else if (display::default_zoom_ == display::ZOOM_56) {
			energy_file = &game_config::images::bar_hrl_56;
		} else if (display::default_zoom_ == display::ZOOM_64) {
			energy_file = &game_config::images::bar_hrl_64;
		} else {
			energy_file = &game_config::images::bar_hrl_72;
		}

		double unit_energy = 0.0;
		if(max_hitpoints() > 0) {
			unit_energy = double(hitpoints())/double(max_hitpoints());
		}
		const int bar_shift = static_cast<int>(-5*disp.get_zoom_factor());
		const int hp_bar_height = static_cast<int>(max_hitpoints()*game_config::hp_bar_scaling);

		const fixed_t bar_alpha = (loc_ == disp.mouseover_hex() || loc_ == disp.selected_hex()) ? ftofxp(1.0): ftofxp(0.8);
		if (display::default_zoom_ == display::ZOOM_48) {
			if (is_city()) {
				disp.draw_bar(*energy_file, xsrc + 20, ysrc - 24, 
					loc_, hp_bar_height, unit_energy, hp_color(), bar_alpha, false);
			} else {
				disp.draw_bar(*energy_file, xsrc + 4, ysrc - 14, 
					loc_, hp_bar_height, unit_energy, hp_color(), bar_alpha, false);
			}
		} else if (display::default_zoom_ == display::ZOOM_56) {
			if (is_city()) {
				disp.draw_bar(*energy_file, xsrc + 20, ysrc - 28, 
					loc_, hp_bar_height, unit_energy, hp_color(), bar_alpha, false);
			} else {
				disp.draw_bar(*energy_file, xsrc + 4, ysrc - 17, 
					loc_, hp_bar_height, unit_energy, hp_color(), bar_alpha, false);
			}
		} else if (display::default_zoom_ == display::ZOOM_64) {
			if (is_city()) {
				disp.draw_bar(*energy_file, xsrc + 22, ysrc - 35, 
					loc_, hp_bar_height, unit_energy, hp_color(), bar_alpha, false);
			} else {
				disp.draw_bar(*energy_file, xsrc + 4, ysrc - 21, 
					loc_, hp_bar_height, unit_energy, hp_color(), bar_alpha, false);
			}
		} else {
			if (is_city()) {
				disp.draw_bar(*energy_file, xsrc + 24, ysrc - 44, 
					loc_, hp_bar_height, unit_energy, hp_color(), bar_alpha, false);
			} else {
				disp.draw_bar(*energy_file, xsrc + 5, ysrc - 25, 
					loc_, hp_bar_height, unit_energy, hp_color(), bar_alpha, false);
			}
		}

	}
	if (!expediting_from_it && can_reside_) {
		if (display::default_zoom_ == display::ZOOM_48) {
			disp.draw_text_in_hex2(loc_, display::LAYER_BORDER, name(), 15, font::BIGMAP_COLOR, xsrc + 24, ysrc - 10);
		} else if (display::default_zoom_ == display::ZOOM_56) {
			disp.draw_text_in_hex2(loc_, display::LAYER_BORDER, name(), 15, font::BIGMAP_COLOR, xsrc + 26, ysrc - 13);
		} else if (display::default_zoom_ == display::ZOOM_64) {
			disp.draw_text_in_hex2(loc_, display::LAYER_BORDER, name(), 15, font::BIGMAP_COLOR, xsrc + 29, ysrc - 16);
		} else {
			disp.draw_text_in_hex2(loc_, display::LAYER_BORDER, name(), 15, font::BIGMAP_COLOR, xsrc + 32, ysrc - 20);
		}
	}

	// params.highlight_ratio = 0.5;
	params.drawing_layer = display::LAYER_UNIT_FG - display::LAYER_UNIT_FIRST;
	if (can_reside_) {
		// to city, display full image always.
		params.submerge = 0.0;
	}

	anim_->redraw(params);

	refreshing_ = false;
}

void artifical::new_turn()
{
	unit::new_turn();

	std::vector<team>& teams = *resources::teams;
	team& current_team = teams[side_ - 1];
	hero& leader = *current_team.leader();

	for (std::vector<hero*>::iterator itor = fresh_heros_.begin(); itor != fresh_heros_.end();) {
		hero* h = *itor;
		if (h != rpg::h && h->loyalty(leader) < game_config::wander_loyalty_threshold) {
			// wander
			artifical* to_city = units_.city_from_seed(teams[side_ - 1].gold());
			game_events::show_hero_message(h, units_.city_from_cityno(h->city_), 
					_("In misfortune time, it is necessary to search chance that can carry out value of life."),
					game_events::INCIDENT_WANDER);
			to_city->wander_into(*h, teams[side_ - 1].is_human()? false: true);
			itor = fresh_heros_.erase(itor);
			break;
		} else {
			++ itor;
		}
	}

	// inside hero, decrease loyalty
	int decrease = 0;
	for (size_t i = 0; i < adjacent_size_; i ++) {
		unit_map::iterator itor = units_.find(adjacent_[i]);
		if (itor.valid() && teams[side_-1].is_enemy(itor->side())) {
			decrease ++;
		}
	}
	if (decrease) {
		if (decrease > (int)adjacent_size_) {
			decrease = (int)adjacent_size_;
		}
		for (std::vector<hero*>::iterator itor = fresh_heros_.begin(); itor != fresh_heros_.end(); ++ itor) {
			(*itor)->increase_loyalty(decrease * game_config::field_troop_increase_loyalty, leader);
		}
		for (std::vector<hero*>::iterator itor = finish_heros_.begin(); itor != finish_heros_.end(); ++ itor) {
			(*itor)->increase_loyalty(decrease * game_config::field_troop_increase_loyalty, leader);
		}
	}

	// select mayor
	if (this_is_city()) {
		if (!mayor_->valid()) {
			select_mayor();
		}
	}
}

void artifical::set_resting(bool rest)
{
	unit::set_resting(rest);

	if (!resting_) {
		return;
	}

	int healing = heal_;
	std::vector<unit*> executor;

	int pos_max = max_hit_points_ - hit_points_;
	int neg_max = -(hit_points_ - 1);
	if (pos_max > 0) {
		// Do not try to "heal" if HP >= max HP
		if (healing > pos_max) {
			healing = pos_max;
		} else if (healing < neg_max) {
			healing = neg_max;
		}
		unit_display::unit_healing(*this, loc_, executor, healing);
		heal(healing);
	}	

	if (!artifical_is_city(this)) {
		return;
	}

	// reside troops
	for (std::vector<unit>::iterator itor = reside_troops_.begin(); itor != reside_troops_.end(); ++ itor) {
		unit& u = *itor;
		u.set_movement(u.total_movement());
		u.set_attacks(u.attacks_total());
		u.increase_loyalty(-1 * game_config::reside_troop_increase_loyalty);
		u.increase_activity(10);

		u.set_state(unit::STATE_SLOWED, false);
		u.set_state(unit::STATE_BROKEN, false);
		u.set_state(unit::STATE_PETRIFIED, false);
		if (u.get_state(unit::STATE_POISONED)) {
			u.set_state(unit::STATE_POISONED, false);
		} else {
			u.heal(12);
		}
	}

	// reside heros
	for (std::vector<hero*>::iterator itor = fresh_heros_.begin(); itor != fresh_heros_.end(); ++ itor) {
		hero& h = **itor;
		if (h.activity_ < HEROS_FULL_ACTIVITY) {
			if (10 + h.activity_ < HEROS_FULL_ACTIVITY) {
				h.activity_ += 15;
			} else {
				h.activity_ = HEROS_FULL_ACTIVITY;
			}
		}
	}
	for (std::vector<hero*>::iterator itor = finish_heros_.begin(); itor != finish_heros_.end(); ++ itor) {
		(*itor)->status_ = hero_status_idle;
	}
	std::copy(finish_heros_.begin(), finish_heros_.end(), std::back_inserter(fresh_heros_));
	finish_heros_.clear();
	return;
}

int artifical::upkeep() const
{
	return reside_troops_.size() * 2;
}

void artifical::get_experience(int xp, bool opp_is_artifical)
{
	if (this_is_city()) {
		unit::get_experience(xp, opp_is_artifical);
	}
}

//
//  ��ʱ���¶���
//    �����佫ָֻ�����佫��һ���佫����ĳ�����Ӹ��佫�ʹ��佫�б���ȥ��
//

// ����
void artifical::troop_come_into(const unit& uobj, int pos)
{
	std::vector<unit>::iterator itor;
	// 1. �ѵ�λ����Ŀ�ĳǿ���λ�б�
	if (pos >= 0) {
		itor = reside_troops_.insert(reside_troops_.begin() + pos, uobj);
	} else {
		reside_troops_.push_back(uobj);
		itor = reside_troops_.begin() + (reside_troops_.size() - 1);
	}

	// get ride of (official_ == hero_official_mayor)
	unit& u = *itor;
	if (u.cityno() != cityno_) {
		for (int i = 0; i < 3; i ++) {
			hero* h = NULL;
			if (i == 0) {
				h = &u.master();
			} else if (i == 1) {
				h = &u.second();
			} else {
				h = &u.third();
			}
			if (!h->valid()) {
				break;
			}
			if (h->official_ == hero_official_mayor) {
				artifical* from = units_.city_from_cityno(h->city_);
				from->select_mayor(&hero_invalid);
			}
		}
	}
	// 2. �޸���Ӫ��
	u.set_side(side_);
	// 3. �޸Ķ��б���
	u.set_cityno(cityno_);
	// 4. �޸�unit����Ϊ�ǿ���������
	u.set_location(loc_);

	return;
}

// before call unit_belong_to, don't update cityno and side!
void artifical::unit_belong_to(unit* troop, bool loyalty, bool to_recorder)
{
	for (int i = 0; i < 3; i ++) {
		hero* h = NULL;
		if (i == 0) {
			h = &troop->master();
		} else if (i == 1) {
			h = &troop->second();
		} else {
			h = &troop->third();
		}
		if (!h->valid()) {
			break;
		}
		if (h->official_ == hero_official_mayor) {
			artifical* from = units_.city_from_cityno(h->city_);
			from->select_mayor(&hero_invalid);
		}
	}

	std::vector<team>& teams = *resources::teams;
	team& prevous_team = teams[troop->side() - 1];
	team& current_team = teams[side_ - 1];

	if (to_recorder) {
		recorder.add_belong_to(troop, this, loyalty);
	}
	// 1. ��troop����ԭ�����еĳ��ⲿ���б�ɾ��
	artifical* src_city = units_.city_from_cityno(troop->cityno());
	if (src_city) {
		if (!troop->is_artifical()) {
			src_city->field_troops_erase(troop);
		} else {
			src_city->field_arts_erase(unit_2_artifical(troop));
		}
	}

	// 2. ����troop����(��������,������Ӫ)
	troop->set_cityno(cityno_);
	troop->set_side(side_);

	// 3. ��troop���뽫�����еĳ��ⲿ���б�
	if (!troop->is_artifical()) {
		field_troops_add(troop);
		if (loyalty) {
			// adjust loyalty, avoid to wander immediately.
			troop->set_loyalty(game_config::wander_loyalty_threshold + 1);
		}
	} else {
		field_arts_add(unit_2_artifical(troop));
	}	

	// of course, current side of troop maybe equal prevous, but need call below.
	// must keep sort-consistent between team.field_troop and artifical[n].field_troop.
	// moved troop are added to end regardless team.field_troop or artifical[n].field_troop.
	prevous_team.erase_troop(troop);
	current_team.add_troop(troop);
}

// ����
// Attention: For iterator, go out in default_ai::do_move don't call this function, erase itor from reside_troops_ directly.
void artifical::troop_go_out(const int uidx)
{
	// 2.����λ�ӳǿ���λ�б����Ƴ�
	reside_troops_.erase(reside_troops_.begin() + uidx);
}

void artifical::field_troops_add(unit* troop)
{
	game_display* disp = game_display::get_singleton();
	if (disp && troop->human()) {
		disp->refresh_access_troops(side_ - 1, game_display::REFRESH_INSERT, NULL, troop);
	}

	field_troops_.push_back(troop);
}

void artifical::field_arts_add(artifical* troop)
{
	field_arts_.push_back(troop);
}

void artifical::field_troops_erase(const unit* troop)
{
	std::vector<unit*>::iterator itor = std::find(field_troops_.begin(), field_troops_.end(), troop);
	if (itor != field_troops_.end()) {
		game_display* disp = game_display::get_singleton();
		if (disp && troop->human()) {
			disp->refresh_access_troops(side_ - 1, game_display::REFRESH_ERASE, NULL, const_cast<unit*>(troop));
		}

		field_troops_.erase(itor);
	}
}

void artifical::field_arts_erase(const artifical* art)
{
	std::vector<artifical*>::iterator itor = std::find(field_arts_.begin(), field_arts_.end(), art);
	if (itor != field_arts_.end()) {
		field_arts_.erase(itor);
	}
}

void artifical::hero_go_out(const hero& h)
{
	for (std::vector<hero*>::iterator iter = fresh_heros_.begin(); iter != fresh_heros_.end(); ++iter) {
		if ((*iter)->number_ == h.number_) {
			fresh_heros_.erase(iter);
			break;
		}
	}
}

void artifical::fresh_into(const unit* troop)
{
	hero* h = &troop->master();
	h->status_ = hero_status_idle;
	h->city_ = cityno_;
	h->side_ = side_ - 1;
	fresh_heros_.push_back(h);
	h = &troop->second();
	if (h->valid()) {
		h->status_ = hero_status_idle;
		h->city_ = cityno_;
		h->side_ = side_ - 1;
		fresh_heros_.push_back(h);
	}
	h = &troop->third();
	if (h->valid()) {
		h->status_ = hero_status_idle;
		h->city_ = cityno_;
		h->side_ = side_ - 1;
		fresh_heros_.push_back(h);
	}
}

void artifical::wander_into(const unit* troop, bool dialog)
{
	std::string message;
	if (dialog) {
		message = _("Go here, Can live the life I would like to?");
	}

	hero* h = &troop->master();
	if (h->official_ == hero_official_mayor) {
		artifical* from = units_.city_from_cityno(h->city_);
		from->select_mayor(&hero_invalid);
	}
	h->status_ = hero_status_wander;
	h->city_ = cityno_;
	h->side_ = HEROS_INVALID_SIDE;
	h->float_catalog_ = ftofxp8(h->base_catalog_);
	wander_heros_.push_back(h);
	if (dialog) {
		game_events::show_hero_message(h, this, message, game_events::INCIDENT_ENTER);
	}
	map_location loc2(MAGIC_HERO, h->number_);
	game_events::fire("post_wander", loc_, loc2);

	h = &troop->second();
	if (h->valid()) {
		if (h->official_ == hero_official_mayor) {
			artifical* from = units_.city_from_cityno(h->city_);
			from->select_mayor(&hero_invalid);
		}
		h->status_ = hero_status_wander;
		h->city_ = cityno_;
		h->side_ = HEROS_INVALID_SIDE;
		h->float_catalog_ = ftofxp8(h->base_catalog_);
		wander_heros_.push_back(h);
		if (dialog) {
			game_events::show_hero_message(h, this, message, game_events::INCIDENT_ENTER);
		}
		loc2.y = h->number_;
		game_events::fire("post_wander", loc_, loc2);
	}
	h = &troop->third();
	if (h->valid()) {
		if (h->official_ == hero_official_mayor) {
			artifical* from = units_.city_from_cityno(h->city_);
			from->select_mayor(&hero_invalid);
		}
		h->status_ = hero_status_wander;
		h->city_ = cityno_;
		h->side_ = HEROS_INVALID_SIDE;
		h->float_catalog_ = ftofxp8(h->base_catalog_);
		wander_heros_.push_back(h);
		if (dialog) {
			game_events::show_hero_message(h, this, message, game_events::INCIDENT_ENTER);
		}
		loc2.y = h->number_;
		game_events::fire("post_wander", loc_, loc2);
	}
}

void artifical::wander_into(hero& h, bool dialog)
{
	std::vector<team>& teams = *resources::teams;

	if (h.official_ == hero_official_commercial) {
		team& t = teams[h.side_];
		t.erase_commercial(&h);
	} else if (h.official_ == hero_official_mayor) {
		artifical* from = units_.city_from_cityno(h.city_);
		from->select_mayor(&hero_invalid);
	}

	h.status_ = hero_status_wander;
	h.city_ = cityno_;
	h.side_ = HEROS_INVALID_SIDE;
	h.float_catalog_ = ftofxp8(h.base_catalog_);
	wander_heros_.push_back(&h);

	if (dialog) {
		std::string message = _("Go here, Can live the life I would like to?");
		game_events::show_hero_message(&h, this, message, game_events::INCIDENT_ENTER);
	}
	map_location loc2(MAGIC_HERO, h.number_);
	game_events::fire("post_wander", loc_, loc2);
}

void artifical::move_into(hero& h)
{
	if (h.official_ == hero_official_mayor) {
		artifical* from = units_.city_from_cityno(h.city_);
		from->select_mayor(&hero_invalid);
	}

	h.status_ = hero_status_moving;
	h.city_ = cityno_;
	h.side_ = side_ - 1;
	finish_heros_.push_back(&h);
}

// fallen
// @attacker: troop that captures this artifical.
void artifical::fallen(int a_side, const unit* attacker)
{
	std::vector<team>& teams = *resources::teams;
	if (a_side <= 0 || a_side > (int)teams.size()) {
		// BUG!!
		a_side = 1;
	}
	play_controller& controller = *resources::controller;
	team& defender_team = teams[side_ - 1];
	team& attacker_team = teams[a_side - 1];
	std::vector<artifical*> defender_citys = defender_team.holded_cities();
	hero* attacker_leader = attacker_team.leader();
	artifical* join_to_city;
	std::string message;
	utils::string_map symbols;
	std::stringstream strstr;
	int random;

	bool fallen_to_unstage = controller.fallen_to_unstage();

	// �����Ƿ��к͸ó�ͬ��Ӫ�ĳ�
	artifical* city_same_side = NULL;
	for (std::vector<artifical*>::iterator itor = defender_citys.begin(); itor != defender_citys.end(); ++ itor) {
		if (*itor != this) {
			city_same_side = *itor;
			break;
		}
	}

	// 1.����ռ���и�Ϊ��ռ����Ӫ(���ȸĵ������³���/���ⲿ�ӻ���ɢ�����ܻ�ɢ��������У���������п����Ǹ��������һ�����У�������Ч��Ӫ�ţ�
	side_ = a_side;

	// �õ���ǰ���ڵĳ����б�
	std::vector<artifical*> citys;
	for (size_t i = 0; i < (*resources::teams).size(); i ++) {
		std::vector<artifical*>& side_citys = (*resources::teams)[i].holded_cities();
		citys.insert(citys.end(), side_citys.begin(), side_citys.end());
	}
	
	// �������ڲ���: ���������ͬ��Ӫ����, ȫ��Ǩ���ó���, ������ɢ
	std::vector<unit> cache_reside_troops = reside_troops_;
	reside_troops_.clear();
	for (std::vector<unit>::iterator itor = cache_reside_troops.begin(); itor != cache_reside_troops.end() ; ++ itor) {
		// ǿ����Ϊ���ꡱ״̬
		unit* current_troop = &*itor;
		
		current_troop->set_movement(0);
		current_troop->set_attacks(0);

		random = get_random();
		join_to_city = NULL;

		if (city_same_side) {
			city_same_side->troop_come_into(*current_troop);
		} else if (fallen_to_unstage) {
			current_troop->to_unstage();
		} else if (current_troop->master().base_catalog_ == attacker_leader->base_catalog_) {
			troop_come_into(*current_troop);
			join_to_city = this;
		} else if ((random % 10 + current_troop->master().loyalty(*attacker_leader)) > HERO_MAX_LOYALTY) {
			troop_come_into(*current_troop);
			join_to_city = this;
		} else {
			artifical* cobj = citys[random % citys.size()];
			if (random % 2) {
				cobj->wander_into(current_troop, teams[cobj->side() - 1].is_human()? true: false);
			} else {
				cobj->troop_come_into(*current_troop);
				join_to_city = cobj;
			}
			resources::screen->invalidate(cobj->get_location());
		}
		if (join_to_city) {
			// leader is changed, recalculate fields
			current_troop->adjust();

			if (current_troop->second().valid()) {
				strstr.str("");
				strstr << current_troop->master().name() << ", " << current_troop->second().name();
				if (current_troop->third().valid()) {
					strstr << ", " << current_troop->third().name();
				}
				symbols["first"] = strstr.str();
				message = vgettext("Find wise leader, $first would like to lead troop join in.", symbols);
			} else {
				message = _("Find wise leader, I would like to lead troop join in.");
			}
			game_events::show_hero_message(&current_troop->master(), join_to_city, message, game_events::INCIDENT_RECOMMENDONESELF);
		}
	}
	// ��������(δ)(��)�佫: ���������ͬ��Ӫ����, ȫ��Ǩ���ó��У��佫��Ϊ�꣩, ������ɢ
	std::vector<hero*> cache_heros = fresh_heros_;
	std::copy(finish_heros_.begin(), finish_heros_.end(), std::back_inserter(cache_heros));
	fresh_heros_.clear();
	finish_heros_.clear();
	for (std::vector<hero*>::iterator itor = cache_heros.begin(); itor != cache_heros.end() ; ++ itor) {
		hero* h = *itor;

		random = get_random();
		join_to_city = NULL;

		if (city_same_side && (h == rpg::h || h->ambition_ > hero_ambition_0)) {
			city_same_side->move_into(*h);
		} else if (fallen_to_unstage) {
			h->to_unstage();
		} else if (h->base_catalog_ == attacker_leader->base_catalog_) {
			move_into(*h);
			join_to_city = this;
		} else if ((random % 10 + h->loyalty(*attacker_leader)) > HERO_MAX_LOYALTY) {
			move_into(*h);
			join_to_city = this;
		} else {
			artifical* cobj = citys[random % citys.size()];
			if (random % 2) {
				cobj->wander_into(*h, teams[cobj->side() - 1].is_human()? true: false);
			} else {
				cobj->move_into(*h);
				join_to_city = cobj;
			}
			resources::screen->invalidate(cobj->get_location());
		}
		if (join_to_city) {
			message = _("Find wise leader, let me join in.");
			game_events::show_hero_message(h, join_to_city, message, game_events::INCIDENT_RECOMMENDONESELF);
		}
	}
	// �������ⲿ��: ���������ͬ��Ӫ����, ȫ��Ǩ��Ϊ���ڸó���, ����������䵽��������
	std::vector<unit*> cache_field_troops = field_troops_;
	std::set<unit*> lobbyisted_troops;
	for (std::vector<unit*>::iterator itor = cache_field_troops.begin(); itor != cache_field_troops.end(); ++itor) {
		// ǿ����Ϊ���ꡱ״̬
		unit* current_troop = *itor;

		current_troop->set_movement(0);
		current_troop->set_attacks(0);
		current_troop->set_goto(map_location());

		random = get_random();
		join_to_city = NULL;

		if (city_same_side) {
			city_same_side->unit_belong_to(current_troop);

		} else {
			for (size_t i = 0; i < current_troop->adjacent_size_; i ++) {
				unit_map::iterator u_itor = units_.find(current_troop->adjacent_[i]);
				if (u_itor.valid() && unit_feature_val2(*u_itor, hero_feature_lobbyist) && u_itor->side() != current_troop->side()) {
					if (lobbyisted_troops.find(&*u_itor) != lobbyisted_troops.end()) {
						// a lobbyist only face down one troop
						continue;
					}
					artifical* cobj = units_.city_from_cityno(u_itor->cityno());
					lobbyisted_troops.insert(&*u_itor);

					join_to_city = cobj;
					break;
				}
			}
			if (join_to_city) {
				join_to_city->unit_belong_to(current_troop);

			} else if (fallen_to_unstage) {
				current_troop->to_unstage();
				units_.erase(current_troop);

			} else if (current_troop->master().base_catalog_ == attacker_leader->base_catalog_) {
				unit_belong_to(current_troop);
				join_to_city = this;

			} else if ((random % 10 + current_troop->master().loyalty(*attacker_leader)) > HERO_MAX_LOYALTY) {
				unit_belong_to(current_troop);
				join_to_city = this;

			} else {
				// ���ȡһ������
				artifical* cobj = citys[random % citys.size()];
				if (random % 2) {
					cobj->wander_into(current_troop, teams[cobj->side() - 1].is_human()? true: false);
					units_.erase(current_troop);
				} else {
					cobj->unit_belong_to(current_troop);
					join_to_city = cobj;

				}
			}
		}
		if (join_to_city) {
			// leader is changed, recalculate fields
			current_troop->adjust();

			if (current_troop->second().valid()) {
				strstr.str("");
				strstr << current_troop->master().name() << ", " << current_troop->second().name();
				if (current_troop->third().valid()) {
					strstr << ", " << current_troop->third().name();
				}
				symbols["first"] = strstr.str();
				message = vgettext("Find wise leader, $first would like to lead troop join in.", symbols);
			} else {
				message = _("Find wise leader, I would like to lead troop join in.");
			}
			game_events::show_hero_message(&current_troop->master(), join_to_city, message, game_events::INCIDENT_RECOMMENDONESELF);
		}
	}

	// �������⽨����: ���������ͬ��Ӫ����, ȫ��Ǩ��Ϊ���ڸó���, ����������䵽��������
	std::vector<artifical*> cache_field_arts = field_arts_;
	for (std::vector<artifical*>::iterator itor = cache_field_arts.begin(); itor != cache_field_arts.end(); ++itor) {
		artifical* current_art = *itor;
		if (current_art->wall() || current_art->type() == unit_types.find_keep()) {
			units_.erase(current_art);

		} else if (city_same_side) {
			city_same_side->unit_belong_to(current_art);

		} else {
			// ���ȡһ������
			artifical* cobj = citys[get_random() % citys.size()];
			cobj->unit_belong_to(current_art);

			// leader is changed, recalculate fields
			current_art->adjust();
		}
	}

	// 3.�����;öȻָ���50%
	hit_points_ = std::max(max_hit_points_ / 2, 1);

	// 4.��ռ�߽������
	if (attacker) {
		troop_come_into(*attacker);
	}

	// 5.�ı����������Ӫӵ�еĳ�����
	defender_team.erase_city(this);
	attacker_team.add_city(this);
	
	if (!attacker_team.is_human()) {
		symbols["first"] = attacker_team.name();
		symbols["second"] = name();
		game_events::show_hero_message(&heros_[214], NULL, vgettext("$first occupy $second.", symbols), 5);
	}
	if (defender_team.holded_cities().empty()) {
		symbols["first"] = defender_team.name();
		game_events::show_hero_message(&heros_[214], NULL, vgettext("$first is defeated.", symbols), 6);
	}

	// side is changed, set master_'s side_. city_ is unthouched. Only when is city. don't set when artifical.
	if (this_is_city()) {
		master_->side_ = side_ - 1;
		if (second_->valid()) {
			second_->side_ = side_ - 1;
		}
		if (third_->valid()) {
			third_->side_ = side_ - 1;
		}
	}

	// leader is changed, recalculate fields
	adjust();

	// get one card random
	if (!controller.is_replaying()) {
		get_random_card(attacker_team, *resources::screen, units_, heros_);
	}

	// erase all strategy that target is this.
	if (this_is_city()) {
		for (size_t i = 0; i < teams.size(); i ++) {
			teams[i].erase_strategy(cityno_);
		}

		if (rpg::stratum != hero_stratum_leader && attacker && attacker->human()) {
			units_.ai_capture_aggressed(*this, side_, !controller.is_replaying());
		}
	}

	return;
}

bool artifical::is_surrounded() const
{
	unit_map::const_iterator itor;
	team  &city_team = (*resources::teams)[side_ - 1];

	for (size_t i = 0; i < adjacent_size_; i ++) {
		itor = units_.find(adjacent_[i]);
		if ((itor == units_.end()) || !city_team.is_enemy(itor->side())) {
			return false;
		}
	}
	return true;
}

void artifical::extract_heros_number()
{
	unit::extract_heros_number();

	fresh_heros_number_.clear();
	for (std::vector<hero*>::iterator itor = fresh_heros_.begin(); itor != fresh_heros_.end(); ++ itor) {
		fresh_heros_number_.push_back((*itor)->number_);
	}
	finish_heros_number_.clear();
	for (std::vector<hero*>::iterator itor = finish_heros_.begin(); itor != finish_heros_.end(); ++ itor) {
		finish_heros_number_.push_back((*itor)->number_);
	}
}

void artifical::recalculate_heros_pointer()
{
	unit::recalculate_heros_pointer();

	fresh_heros_.clear();
	for (std::vector<uint16_t>::iterator itor = fresh_heros_number_.begin(); itor != fresh_heros_number_.end(); ++ itor) {
		fresh_heros_.push_back(&heros_[*itor]);
	}
	finish_heros_.clear();
	for (std::vector<uint16_t>::iterator itor = finish_heros_number_.begin(); itor != finish_heros_number_.end(); ++ itor) {
		finish_heros_.push_back(&heros_[*itor]);
	}
}

bool compare_for_mayor(const hero* a, const hero* b, const hero* leader)
{
	if (b == rpg::h) {
		return true;
	}
	if (a == rpg::h) {
		return false;
	}
	if (!b->valid()) {
		return true;
	}
	if (!a->valid()) {
		return false;
	}
	if (b->official_ != HEROS_NO_OFFICIAL) {
		return true;
	}
	if (a->official_ != HEROS_NO_OFFICIAL) {
		return false;
	}
	int a_loyality = a->loyalty(*leader);
	int b_loyality = b->loyalty(*leader);
	if (a_loyality < game_config::move_loyalty_threshold || b_loyality < game_config::move_loyalty_threshold) {
		return a_loyality >= b_loyality;
	}
	return (a->politics_ > b->politics_) || (a->politics_ == b->politics_ && a_loyality >= b_loyality);
}

void artifical::select_mayor(hero* commend)
{
	std::vector<team>& teams = *resources::teams;
	if (commend) {
		if (mayor_ == commend) {
			return;
		}
		if (mayor_->valid()) {
			mayor_->official_ = HEROS_NO_OFFICIAL;
		}
		if (mayor_ == rpg::h) {
			rpg::stratum = hero_stratum_citizen;
			teams[rpg::h->side_].rpg_changed();
			
			utils::string_map symbols;
			symbols["city"] = name();
			std::string message = vgettext("Your official of $city's mayor is terminated.", symbols);
			game_events::show_hero_message(&heros_[229], this, message, game_events::INCIDENT_INVALID);
		}
		mayor_ = commend;
		if (mayor_->valid()) {
			mayor_->official_ = hero_official_mayor;
			if (mayor_ == rpg::h && rpg::stratum != hero_stratum_mayor) {
				rpg::stratum = hero_stratum_mayor;
				teams[rpg::h->side_].rpg_changed();
			}
		}
		return;
	}

	team& t = teams[side_ - 1];
	const hero* leader = t.leader();
	hero* max = &hero_invalid;
	for (std::vector<hero*>::iterator it = fresh_heros_.begin(); it != fresh_heros_.end(); ++ it) {
		hero* h = *it;
		if (compare_for_mayor(h, max, leader)) {
			max = h;
		}
	}
	for (std::vector<hero*>::iterator it = finish_heros_.begin(); it != finish_heros_.end(); ++ it) {
		hero* h = *it;
		if (compare_for_mayor(h, max, leader)) {
			max = h;
		}
	}
	for (std::vector<unit>::iterator it = reside_troops_.begin(); it != reside_troops_.end(); ++ it) {
		unit& u = *it;
		hero* h = &u.master();
		if (compare_for_mayor(h, max, leader)) {
			max = h;
		}
		if (u.second().valid()) {
			h = &u.second();
			if (compare_for_mayor(h, max, leader)) {
				max = h;
			}
		}
		if (u.third().valid()) {
			h = &u.third();
			if (compare_for_mayor(h, max, leader)) {
				max = h;
			}
		}
	}
	for (std::vector<unit*>::iterator it = field_troops_.begin(); it != field_troops_.end(); ++ it) {
		unit& u = **it;
		hero* h = &u.master();
		if (compare_for_mayor(h, max, leader)) {
			max = h;
		}
		if (u.second().valid()) {
			h = &u.second();
			if (compare_for_mayor(h, max, leader)) {
				max = h;
			}
		}
		if (u.third().valid()) {
			h = &u.third();
			if (compare_for_mayor(h, max, leader)) {
				max = h;
			}
		}
	}
	if (max->official_ == HEROS_NO_OFFICIAL && max != rpg::h) {
		mayor_ = max;
		mayor_->official_ = hero_official_mayor;
	} else {
		mayor_ = &hero_invalid;
	}
}

int artifical::office_heros() const
{
	// estimate
	return int(fresh_heros_.size() + finish_heros_.size() + (reside_troops_.size() + field_troops_.size()) * 2);
}

void artifical::inching_fronts(bool increase)
{
	if (increase) {
		fronts_ ++;
		if (fronts_ > mr_data::max_fronts) {
			fronts_ = mr_data::max_fronts;
		}
	} else if (fronts_ > 0) {
		fronts_ --;
		if (fronts_ == 0) {
			team& t = (*resources::teams)[side_ -1];
			t.erase_strategy(cityno_);
		}
	}
}

void artifical::set_fronts(int fronts)
{
	fronts_ = fronts;
	if (fronts_ < 0) {
		fronts_ = 0;
	} else if (fronts_ > mr_data::max_fronts) {
		fronts_ = mr_data::max_fronts;
	}
}