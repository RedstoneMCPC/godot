/*************************************************************************/
/*  tab_container.cpp                                                    */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "tab_container.h"

#include "scene/gui/box_container.h"
#include "scene/gui/label.h"
#include "scene/gui/texture_rect.h"

int TabContainer::_get_top_margin() const {
	int height = 0;
	if (tabs_visible && get_tab_count() > 0) {
		height = tab_bar->get_minimum_size().height;
	}

	return height;
}

void TabContainer::gui_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	Ref<InputEventMouseButton> mb = p_event;

	Popup *popup = get_popup();

	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT) {
		Point2 pos = mb->get_position();
		Size2 size = get_size();

		// Click must be on tabs in the tab header area.
		if (pos.y > _get_top_margin()) {
			return;
		}

		// Handle menu button.
		Ref<Texture2D> menu = get_theme_icon(SNAME("menu"));

		if (is_layout_rtl()) {
			if (popup && pos.x < menu->get_width()) {
				emit_signal(SNAME("pre_popup_pressed"));

				Vector2 popup_pos = get_screen_position();
				popup_pos.y += menu->get_height();

				popup->set_position(popup_pos);
				popup->popup();
				return;
			}
		} else {
			if (popup && pos.x > size.width - menu->get_width()) {
				emit_signal(SNAME("pre_popup_pressed"));

				Vector2 popup_pos = get_screen_position();
				popup_pos.x += size.width - popup->get_size().width;
				popup_pos.y += menu->get_height();

				popup->set_position(popup_pos);
				popup->popup();
				return;
			}
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;

	if (mm.is_valid()) {
		Point2 pos = mm->get_position();
		Size2 size = get_size();

		// Mouse must be on tabs in the tab header area.
		if (pos.y > _get_top_margin()) {
			if (menu_hovered) {
				menu_hovered = false;
				update();
			}
			return;
		}

		Ref<Texture2D> menu = get_theme_icon(SNAME("menu"));
		if (popup) {
			if (is_layout_rtl()) {
				if (pos.x <= menu->get_width()) {
					if (!menu_hovered) {
						menu_hovered = true;
						update();
						return;
					}
				} else if (menu_hovered) {
					menu_hovered = false;
					update();
				}
			} else {
				if (pos.x >= size.width - menu->get_width()) {
					if (!menu_hovered) {
						menu_hovered = true;
						update();
						return;
					}
				} else if (menu_hovered) {
					menu_hovered = false;
					update();
				}
			}

			if (menu_hovered) {
				return;
			}
		}
	}
}

void TabContainer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			// If some nodes happen to be renamed outside the tree, the tab names need to be updated manually.
			if (get_tab_count() > 0) {
				_refresh_tab_names();
			}
		} break;

		case NOTIFICATION_READY:
		case NOTIFICATION_RESIZED: {
			_update_margins();
		} break;

		case NOTIFICATION_DRAW: {
			RID canvas = get_canvas_item();
			Size2 size = get_size();

			// Draw only the tab area if the header is hidden.
			Ref<StyleBox> panel = get_theme_stylebox(SNAME("panel"));
			if (!tabs_visible) {
				panel->draw(canvas, Rect2(0, 0, size.width, size.height));
				return;
			}

			int header_height = _get_top_margin();

			panel->draw(canvas, Rect2(0, header_height, size.width, size.height - header_height));

			// Draw the popup menu.
			if (get_popup()) {
				Ref<Texture2D> menu = get_theme_icon(SNAME("menu"));
				Ref<Texture2D> menu_hl = get_theme_icon(SNAME("menu_highlight"));

				int x = is_layout_rtl() ? 0 : get_size().width - menu->get_width();

				if (menu_hovered) {
					menu_hl->draw(get_canvas_item(), Size2(x, (header_height - menu_hl->get_height()) / 2));
				} else {
					menu->draw(get_canvas_item(), Size2(x, (header_height - menu->get_height()) / 2));
				}
			}
		} break;

		case NOTIFICATION_TRANSLATION_CHANGED:
		case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
		case NOTIFICATION_THEME_CHANGED: {
			theme_changing = true;
			call_deferred(SNAME("_on_theme_changed")); // Wait until all changed theme.
		} break;
	}
}

void TabContainer::_on_theme_changed() {
	if (!theme_changing) {
		return;
	}

	tab_bar->add_theme_style_override(SNAME("tab_unselected"), get_theme_stylebox(SNAME("tab_unselected")));
	tab_bar->add_theme_style_override(SNAME("tab_selected"), get_theme_stylebox(SNAME("tab_selected")));
	tab_bar->add_theme_style_override(SNAME("tab_disabled"), get_theme_stylebox(SNAME("tab_disabled")));
	tab_bar->add_theme_icon_override(SNAME("increment"), get_theme_icon(SNAME("increment")));
	tab_bar->add_theme_icon_override(SNAME("increment_highlight"), get_theme_icon(SNAME("increment_highlight")));
	tab_bar->add_theme_icon_override(SNAME("decrement"), get_theme_icon(SNAME("decrement")));
	tab_bar->add_theme_icon_override(SNAME("decrement_highlight"), get_theme_icon(SNAME("decrement_highlight")));
	tab_bar->add_theme_color_override(SNAME("font_selected_color"), get_theme_color(SNAME("font_selected_color")));
	tab_bar->add_theme_color_override(SNAME("font_unselected_color"), get_theme_color(SNAME("font_unselected_color")));
	tab_bar->add_theme_color_override(SNAME("font_disabled_color"), get_theme_color(SNAME("font_disabled_color")));
	tab_bar->add_theme_color_override(SNAME("font_outline_color"), get_theme_color(SNAME("font_outline_color")));
	tab_bar->add_theme_font_override(SNAME("font"), get_theme_font(SNAME("font")));
	tab_bar->add_theme_constant_override(SNAME("font_size"), get_theme_constant(SNAME("font_size")));
	tab_bar->add_theme_constant_override(SNAME("icon_separation"), get_theme_constant(SNAME("icon_separation")));
	tab_bar->add_theme_constant_override(SNAME("outline_size"), get_theme_constant(SNAME("outline_size")));

	_update_margins();
	if (get_tab_count() > 0) {
		_repaint();
	} else {
		update_minimum_size();
	}
	update();

	theme_changing = false;
}

void TabContainer::_repaint() {
	Ref<StyleBox> sb = get_theme_stylebox(SNAME("panel"));
	Vector<Control *> controls = _get_tab_controls();
	int current = get_current_tab();

	for (int i = 0; i < controls.size(); i++) {
		Control *c = controls[i];

		if (i == current) {
			c->show();
			c->set_anchors_and_offsets_preset(Control::PRESET_WIDE);

			if (tabs_visible) {
				c->set_offset(SIDE_TOP, _get_top_margin());
			}

			c->set_offset(SIDE_TOP, c->get_offset(SIDE_TOP) + sb->get_margin(SIDE_TOP));
			c->set_offset(SIDE_LEFT, c->get_offset(SIDE_LEFT) + sb->get_margin(SIDE_LEFT));
			c->set_offset(SIDE_RIGHT, c->get_offset(SIDE_RIGHT) - sb->get_margin(SIDE_RIGHT));
			c->set_offset(SIDE_BOTTOM, c->get_offset(SIDE_BOTTOM) - sb->get_margin(SIDE_BOTTOM));
		} else {
			c->hide();
		}
	}

	update_minimum_size();
}

void TabContainer::_update_margins() {
	int menu_width = get_theme_icon(SNAME("menu"))->get_width();
	int side_margin = get_theme_constant(SNAME("side_margin"));

	// Directly check for validity, to avoid errors when quitting.
	bool has_popup = popup_obj_id.is_valid();

	if (get_tab_count() == 0) {
		tab_bar->set_offset(SIDE_LEFT, 0);
		tab_bar->set_offset(SIDE_RIGHT, has_popup ? -menu_width : 0);

		return;
	}

	switch (get_tab_alignment()) {
		case TabBar::ALIGNMENT_LEFT: {
			tab_bar->set_offset(SIDE_LEFT, side_margin);
			tab_bar->set_offset(SIDE_RIGHT, has_popup ? -menu_width : 0);
		} break;

		case TabBar::ALIGNMENT_CENTER: {
			tab_bar->set_offset(SIDE_LEFT, 0);
			tab_bar->set_offset(SIDE_RIGHT, has_popup ? -menu_width : 0);
		} break;

		case TabBar::ALIGNMENT_RIGHT: {
			tab_bar->set_offset(SIDE_LEFT, 0);

			if (has_popup) {
				tab_bar->set_offset(SIDE_RIGHT, -menu_width);
				return;
			}

			int first_tab_pos = tab_bar->get_tab_rect(0).position.x;
			Rect2 last_tab_rect = tab_bar->get_tab_rect(get_tab_count() - 1);
			int total_tabs_width = last_tab_rect.position.x - first_tab_pos + last_tab_rect.size.width;

			// Calculate if all the tabs would still fit if the margin was present.
			if (get_clip_tabs() && (tab_bar->get_offset_buttons_visible() || (get_tab_count() > 1 && (total_tabs_width + side_margin) > get_size().width))) {
				tab_bar->set_offset(SIDE_RIGHT, has_popup ? -menu_width : 0);
			} else {
				tab_bar->set_offset(SIDE_RIGHT, -side_margin);
			}
		} break;

		case TabBar::ALIGNMENT_MAX:
			break; // Can't happen, but silences warning.
	}
}

void TabContainer::_on_mouse_exited() {
	if (menu_hovered) {
		menu_hovered = false;
		update();
	}
}

Vector<Control *> TabContainer::_get_tab_controls() const {
	Vector<Control *> controls;
	for (int i = 0; i < get_child_count(); i++) {
		Control *control = Object::cast_to<Control>(get_child(i));
		if (!control || control->is_set_as_top_level() || control == tab_bar) {
			continue;
		}

		controls.push_back(control);
	}

	return controls;
}

Variant TabContainer::_get_drag_data_fw(const Point2 &p_point, Control *p_from_control) {
	if (!drag_to_rearrange_enabled) {
		return Variant();
	}

	int tab_over = get_tab_idx_at_point(p_point);
	if (tab_over < 0) {
		return Variant();
	}

	HBoxContainer *drag_preview = memnew(HBoxContainer);

	Ref<Texture2D> icon = get_tab_icon(tab_over);
	if (!icon.is_null()) {
		TextureRect *tf = memnew(TextureRect);
		tf->set_texture(icon);
		drag_preview->add_child(tf);
	}

	Label *label = memnew(Label(get_tab_title(tab_over)));
	set_drag_preview(drag_preview);
	drag_preview->add_child(label);

	Dictionary drag_data;
	drag_data["type"] = "tabc_element";
	drag_data["tabc_element"] = tab_over;
	drag_data["from_path"] = get_path();

	return drag_data;
}

bool TabContainer::_can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from_control) const {
	if (!drag_to_rearrange_enabled) {
		return false;
	}

	Dictionary d = p_data;
	if (!d.has("type")) {
		return false;
	}

	if (String(d["type"]) == "tabc_element") {
		NodePath from_path = d["from_path"];
		NodePath to_path = get_path();
		if (from_path == to_path) {
			return true;
		} else if (get_tabs_rearrange_group() != -1) {
			// Drag and drop between other TabContainers.
			Node *from_node = get_node(from_path);
			TabContainer *from_tabc = Object::cast_to<TabContainer>(from_node);
			if (from_tabc && from_tabc->get_tabs_rearrange_group() == get_tabs_rearrange_group()) {
				return true;
			}
		}
	}

	return false;
}

void TabContainer::_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from_control) {
	if (!drag_to_rearrange_enabled) {
		return;
	}

	int hover_now = get_tab_idx_at_point(p_point);

	Dictionary d = p_data;
	if (!d.has("type")) {
		return;
	}

	if (String(d["type"]) == "tabc_element") {
		int tab_from_id = d["tabc_element"];
		NodePath from_path = d["from_path"];
		NodePath to_path = get_path();
		if (from_path == to_path) {
			if (hover_now < 0) {
				hover_now = get_tab_count() - 1;
			}

			move_child(get_tab_control(tab_from_id), get_tab_control(hover_now)->get_index(false));
			set_current_tab(hover_now);
		} else if (get_tabs_rearrange_group() != -1) {
			// Drag and drop between TabContainers.
			Node *from_node = get_node(from_path);
			TabContainer *from_tabc = Object::cast_to<TabContainer>(from_node);
			if (from_tabc && from_tabc->get_tabs_rearrange_group() == get_tabs_rearrange_group()) {
				Control *moving_tabc = from_tabc->get_tab_control(tab_from_id);
				from_tabc->remove_child(moving_tabc);
				add_child(moving_tabc, true);

				if (hover_now < 0) {
					hover_now = get_tab_count() - 1;
				}

				move_child(moving_tabc, get_tab_control(hover_now)->get_index(false));

				set_current_tab(hover_now);
			}
		}
	}
}

void TabContainer::_on_tab_changed(int p_tab) {
	call_deferred(SNAME("_repaint"));

	emit_signal(SNAME("tab_changed"), p_tab);
}

void TabContainer::_on_tab_selected(int p_tab) {
	if (p_tab != get_previous_tab()) {
		call_deferred(SNAME("_repaint"));
	}

	emit_signal(SNAME("tab_selected"), p_tab);
}

void TabContainer::_refresh_tab_names() {
	Vector<Control *> controls = _get_tab_controls();
	for (int i = 0; i < controls.size(); i++) {
		if (!controls[i]->has_meta("_tab_name") && String(controls[i]->get_name()) != get_tab_title(i)) {
			tab_bar->set_tab_title(i, controls[i]->get_name());
		}
	}
}

void TabContainer::add_child_notify(Node *p_child) {
	if (p_child == tab_bar) {
		return;
	}

	Container::add_child_notify(p_child);

	Control *c = Object::cast_to<Control>(p_child);
	if (!c || c->is_set_as_top_level()) {
		return;
	}
	c->hide();

	tab_bar->add_tab(p_child->get_name());

	_update_margins();

	p_child->connect("renamed", callable_mp(this, &TabContainer::_refresh_tab_names));

	// TabBar won't emit the "tab_changed" signal when not inside the tree.
	if (!is_inside_tree()) {
		call_deferred("_repaint");
	}
}

void TabContainer::move_child_notify(Node *p_child) {
	if (p_child == tab_bar) {
		return;
	}

	Container::move_child_notify(p_child);

	Control *c = Object::cast_to<Control>(p_child);
	if (c && !c->is_set_as_top_level()) {
		int old_idx = -1;
		String tab_name = c->has_meta("_tab_name") ? String(c->get_meta("_tab_name")) : String(c->get_name());

		// Find the previous tab index of the control.
		for (int i = 0; i < get_tab_count(); i++) {
			if (get_tab_title(i) == tab_name) {
				old_idx = i;
				break;
			}
		}

		tab_bar->move_tab(old_idx, get_tab_idx_from_control(c));
	}
}

void TabContainer::remove_child_notify(Node *p_child) {
	if (p_child == tab_bar) {
		return;
	}

	Container::remove_child_notify(p_child);

	Control *c = Object::cast_to<Control>(p_child);
	if (!c || c->is_set_as_top_level()) {
		return;
	}

	tab_bar->remove_tab(get_tab_idx_from_control(c));

	_update_margins();

	if (p_child->has_meta("_tab_name")) {
		p_child->remove_meta("_tab_name");
	}
	p_child->disconnect("renamed", callable_mp(this, &TabContainer::_refresh_tab_names));

	// TabBar won't emit the "tab_changed" signal when not inside the tree.
	if (!is_inside_tree()) {
		call_deferred("_repaint");
	}
}

int TabContainer::get_tab_count() const {
	return tab_bar->get_tab_count();
}

void TabContainer::set_current_tab(int p_current) {
	tab_bar->set_current_tab(p_current);
}

int TabContainer::get_current_tab() const {
	return tab_bar->get_current_tab();
}

int TabContainer::get_previous_tab() const {
	return tab_bar->get_previous_tab();
}

Control *TabContainer::get_tab_control(int p_idx) const {
	Vector<Control *> controls = _get_tab_controls();
	if (p_idx >= 0 && p_idx < controls.size()) {
		return controls[p_idx];
	} else {
		return nullptr;
	}
}

Control *TabContainer::get_current_tab_control() const {
	return get_tab_control(tab_bar->get_current_tab());
}

int TabContainer::get_tab_idx_at_point(const Point2 &p_point) const {
	return tab_bar->get_tab_idx_at_point(p_point);
}

int TabContainer::get_tab_idx_from_control(Control *p_child) const {
	ERR_FAIL_NULL_V(p_child, -1);
	ERR_FAIL_COND_V(p_child->get_parent() != this, -1);

	Vector<Control *> controls = _get_tab_controls();
	for (int i = 0; i < controls.size(); i++) {
		if (controls[i] == p_child) {
			return i;
		}
	}

	return -1;
}

void TabContainer::set_tab_alignment(TabBar::AlignmentMode p_alignment) {
	tab_bar->set_tab_alignment(p_alignment);
	_update_margins();
}

TabBar::AlignmentMode TabContainer::get_tab_alignment() const {
	return tab_bar->get_tab_alignment();
}

void TabContainer::set_clip_tabs(bool p_clip_tabs) {
	tab_bar->set_clip_tabs(p_clip_tabs);
}

bool TabContainer::get_clip_tabs() const {
	return tab_bar->get_clip_tabs();
}

void TabContainer::set_tabs_visible(bool p_visible) {
	if (p_visible == tabs_visible) {
		return;
	}

	tabs_visible = p_visible;
	tab_bar->set_visible(tabs_visible);

	Vector<Control *> controls = _get_tab_controls();
	for (int i = 0; i < controls.size(); i++) {
		Control *c = controls[i];
		if (tabs_visible) {
			c->set_offset(SIDE_TOP, _get_top_margin());
		} else {
			c->set_offset(SIDE_TOP, 0);
		}
	}

	update();
	update_minimum_size();
}

bool TabContainer::are_tabs_visible() const {
	return tabs_visible;
}

void TabContainer::set_all_tabs_in_front(bool p_in_front) {
	if (p_in_front == all_tabs_in_front) {
		return;
	}

	all_tabs_in_front = p_in_front;

	remove_child(tab_bar);
	add_child(tab_bar, false, all_tabs_in_front ? INTERNAL_MODE_BACK : INTERNAL_MODE_FRONT);
}

bool TabContainer::is_all_tabs_in_front() const {
	return all_tabs_in_front;
}

void TabContainer::set_tab_title(int p_tab, const String &p_title) {
	Control *child = get_tab_control(p_tab);
	ERR_FAIL_COND(!child);

	if (p_title.is_empty()) {
		tab_bar->set_tab_title(p_tab, String(child->get_name()));

		if (child->has_meta("_tab_name")) {
			child->remove_meta("_tab_name");
		}
	} else {
		tab_bar->set_tab_title(p_tab, p_title);
		child->set_meta("_tab_name", p_title);
	}

	_update_margins();
	if (!get_clip_tabs()) {
		update_minimum_size();
	}
}

String TabContainer::get_tab_title(int p_tab) const {
	return tab_bar->get_tab_title(p_tab);
}

void TabContainer::set_tab_icon(int p_tab, const Ref<Texture2D> &p_icon) {
	tab_bar->set_tab_icon(p_tab, p_icon);

	_update_margins();
	_repaint();
}

Ref<Texture2D> TabContainer::get_tab_icon(int p_tab) const {
	return tab_bar->get_tab_icon(p_tab);
}

void TabContainer::set_tab_disabled(int p_tab, bool p_disabled) {
	tab_bar->set_tab_disabled(p_tab, p_disabled);

	_update_margins();
	if (!get_clip_tabs()) {
		update_minimum_size();
	}
}

bool TabContainer::is_tab_disabled(int p_tab) const {
	return tab_bar->is_tab_disabled(p_tab);
}

void TabContainer::set_tab_hidden(int p_tab, bool p_hidden) {
	Control *child = get_tab_control(p_tab);
	ERR_FAIL_COND(!child);

	tab_bar->set_tab_hidden(p_tab, p_hidden);
	child->hide();

	_update_margins();
	if (!get_clip_tabs()) {
		update_minimum_size();
	}
}

bool TabContainer::is_tab_hidden(int p_tab) const {
	return tab_bar->is_tab_hidden(p_tab);
}

void TabContainer::get_translatable_strings(List<String> *p_strings) const {
	Vector<Control *> controls = _get_tab_controls();
	for (int i = 0; i < controls.size(); i++) {
		Control *c = controls[i];

		if (!c->has_meta("_tab_name")) {
			continue;
		}

		String name = c->get_meta("_tab_name");
		if (!name.is_empty()) {
			p_strings->push_back(name);
		}
	}
}

Size2 TabContainer::get_minimum_size() const {
	Size2 ms;

	if (tabs_visible) {
		ms = tab_bar->get_minimum_size();

		if (!get_clip_tabs()) {
			if (get_popup()) {
				ms.x += get_theme_icon(SNAME("menu"))->get_width();
			}

			int side_margin = get_theme_constant(SNAME("side_margin"));
			if (side_margin > 0 && get_tab_alignment() != TabBar::ALIGNMENT_CENTER &&
					(get_tab_alignment() != TabBar::ALIGNMENT_RIGHT || !get_popup())) {
				ms.x += side_margin;
			}
		}
	}

	Vector<Control *> controls = _get_tab_controls();
	int max_control_height = 0;
	for (int i = 0; i < controls.size(); i++) {
		Control *c = controls[i];

		if (!c->is_visible_in_tree() && !use_hidden_tabs_for_min_size) {
			continue;
		}

		Size2 cms = c->get_combined_minimum_size();
		ms.x = MAX(ms.x, cms.x);
		max_control_height = MAX(max_control_height, cms.y);
	}
	ms.y += max_control_height;

	Size2 panel_ms = get_theme_stylebox(SNAME("panel"))->get_minimum_size();
	ms.x = MAX(ms.x, panel_ms.x);
	ms.y += panel_ms.y;

	return ms;
}

void TabContainer::set_popup(Node *p_popup) {
	bool had_popup = get_popup();

	Popup *popup = Object::cast_to<Popup>(p_popup);
	popup_obj_id = popup ? popup->get_instance_id() : ObjectID();

	if (had_popup != bool(popup)) {
		update();
		_update_margins();
		if (!get_clip_tabs()) {
			update_minimum_size();
		}
	}
}

Popup *TabContainer::get_popup() const {
	if (popup_obj_id.is_valid()) {
		Popup *popup = Object::cast_to<Popup>(ObjectDB::get_instance(popup_obj_id));
		if (popup) {
			return popup;
		} else {
#ifdef DEBUG_ENABLED
			ERR_PRINT("Popup assigned to TabContainer is gone!");
#endif
			popup_obj_id = ObjectID();
		}
	}

	return nullptr;
}

void TabContainer::set_drag_to_rearrange_enabled(bool p_enabled) {
	drag_to_rearrange_enabled = p_enabled;
}

bool TabContainer::get_drag_to_rearrange_enabled() const {
	return drag_to_rearrange_enabled;
}

void TabContainer::set_tabs_rearrange_group(int p_group_id) {
	tab_bar->set_tabs_rearrange_group(p_group_id);
}

int TabContainer::get_tabs_rearrange_group() const {
	return tab_bar->get_tabs_rearrange_group();
}

void TabContainer::set_use_hidden_tabs_for_min_size(bool p_use_hidden_tabs) {
	use_hidden_tabs_for_min_size = p_use_hidden_tabs;
	update_minimum_size();
}

bool TabContainer::get_use_hidden_tabs_for_min_size() const {
	return use_hidden_tabs_for_min_size;
}

Vector<int> TabContainer::get_allowed_size_flags_horizontal() const {
	return Vector<int>();
}

Vector<int> TabContainer::get_allowed_size_flags_vertical() const {
	return Vector<int>();
}

void TabContainer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_tab_count"), &TabContainer::get_tab_count);
	ClassDB::bind_method(D_METHOD("set_current_tab", "tab_idx"), &TabContainer::set_current_tab);
	ClassDB::bind_method(D_METHOD("get_current_tab"), &TabContainer::get_current_tab);
	ClassDB::bind_method(D_METHOD("get_previous_tab"), &TabContainer::get_previous_tab);
	ClassDB::bind_method(D_METHOD("get_current_tab_control"), &TabContainer::get_current_tab_control);
	ClassDB::bind_method(D_METHOD("get_tab_control", "tab_idx"), &TabContainer::get_tab_control);
	ClassDB::bind_method(D_METHOD("set_tab_alignment", "alignment"), &TabContainer::set_tab_alignment);
	ClassDB::bind_method(D_METHOD("get_tab_alignment"), &TabContainer::get_tab_alignment);
	ClassDB::bind_method(D_METHOD("set_clip_tabs", "clip_tabs"), &TabContainer::set_clip_tabs);
	ClassDB::bind_method(D_METHOD("get_clip_tabs"), &TabContainer::get_clip_tabs);
	ClassDB::bind_method(D_METHOD("set_tabs_visible", "visible"), &TabContainer::set_tabs_visible);
	ClassDB::bind_method(D_METHOD("are_tabs_visible"), &TabContainer::are_tabs_visible);
	ClassDB::bind_method(D_METHOD("set_all_tabs_in_front", "is_front"), &TabContainer::set_all_tabs_in_front);
	ClassDB::bind_method(D_METHOD("is_all_tabs_in_front"), &TabContainer::is_all_tabs_in_front);
	ClassDB::bind_method(D_METHOD("set_tab_title", "tab_idx", "title"), &TabContainer::set_tab_title);
	ClassDB::bind_method(D_METHOD("get_tab_title", "tab_idx"), &TabContainer::get_tab_title);
	ClassDB::bind_method(D_METHOD("set_tab_icon", "tab_idx", "icon"), &TabContainer::set_tab_icon);
	ClassDB::bind_method(D_METHOD("get_tab_icon", "tab_idx"), &TabContainer::get_tab_icon);
	ClassDB::bind_method(D_METHOD("set_tab_disabled", "tab_idx", "disabled"), &TabContainer::set_tab_disabled);
	ClassDB::bind_method(D_METHOD("is_tab_disabled", "tab_idx"), &TabContainer::is_tab_disabled);
	ClassDB::bind_method(D_METHOD("set_tab_hidden", "tab_idx", "hidden"), &TabContainer::set_tab_hidden);
	ClassDB::bind_method(D_METHOD("is_tab_hidden", "tab_idx"), &TabContainer::is_tab_hidden);
	ClassDB::bind_method(D_METHOD("get_tab_idx_at_point", "point"), &TabContainer::get_tab_idx_at_point);
	ClassDB::bind_method(D_METHOD("get_tab_idx_from_control", "control"), &TabContainer::get_tab_idx_from_control);
	ClassDB::bind_method(D_METHOD("set_popup", "popup"), &TabContainer::set_popup);
	ClassDB::bind_method(D_METHOD("get_popup"), &TabContainer::get_popup);
	ClassDB::bind_method(D_METHOD("set_drag_to_rearrange_enabled", "enabled"), &TabContainer::set_drag_to_rearrange_enabled);
	ClassDB::bind_method(D_METHOD("get_drag_to_rearrange_enabled"), &TabContainer::get_drag_to_rearrange_enabled);
	ClassDB::bind_method(D_METHOD("set_tabs_rearrange_group", "group_id"), &TabContainer::set_tabs_rearrange_group);
	ClassDB::bind_method(D_METHOD("get_tabs_rearrange_group"), &TabContainer::get_tabs_rearrange_group);
	ClassDB::bind_method(D_METHOD("set_use_hidden_tabs_for_min_size", "enabled"), &TabContainer::set_use_hidden_tabs_for_min_size);
	ClassDB::bind_method(D_METHOD("get_use_hidden_tabs_for_min_size"), &TabContainer::get_use_hidden_tabs_for_min_size);

	ClassDB::bind_method(D_METHOD("_repaint"), &TabContainer::_repaint);
	ClassDB::bind_method(D_METHOD("_on_theme_changed"), &TabContainer::_on_theme_changed);
	ClassDB::bind_method(D_METHOD("_get_drag_data_fw"), &TabContainer::_get_drag_data_fw);
	ClassDB::bind_method(D_METHOD("_can_drop_data_fw"), &TabContainer::_can_drop_data_fw);
	ClassDB::bind_method(D_METHOD("_drop_data_fw"), &TabContainer::_drop_data_fw);

	ADD_SIGNAL(MethodInfo("tab_changed", PropertyInfo(Variant::INT, "tab")));
	ADD_SIGNAL(MethodInfo("tab_selected", PropertyInfo(Variant::INT, "tab")));
	ADD_SIGNAL(MethodInfo("pre_popup_pressed"));

	ADD_PROPERTY(PropertyInfo(Variant::INT, "tab_alignment", PROPERTY_HINT_ENUM, "Left,Center,Right"), "set_tab_alignment", "get_tab_alignment");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "current_tab", PROPERTY_HINT_RANGE, "-1,4096,1", PROPERTY_USAGE_EDITOR), "set_current_tab", "get_current_tab");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "clip_tabs"), "set_clip_tabs", "get_clip_tabs");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "tabs_visible"), "set_tabs_visible", "are_tabs_visible");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "all_tabs_in_front"), "set_all_tabs_in_front", "is_all_tabs_in_front");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "drag_to_rearrange_enabled"), "set_drag_to_rearrange_enabled", "get_drag_to_rearrange_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_hidden_tabs_for_min_size"), "set_use_hidden_tabs_for_min_size", "get_use_hidden_tabs_for_min_size");
}

TabContainer::TabContainer() {
	tab_bar = memnew(TabBar);
	tab_bar->set_drag_forwarding(this);
	add_child(tab_bar, false, INTERNAL_MODE_FRONT);
	tab_bar->set_anchors_and_offsets_preset(Control::PRESET_TOP_WIDE);
	tab_bar->connect("tab_changed", callable_mp(this, &TabContainer::_on_tab_changed));
	tab_bar->connect("tab_selected", callable_mp(this, &TabContainer::_on_tab_selected));

	connect("mouse_exited", callable_mp(this, &TabContainer::_on_mouse_exited));
}
