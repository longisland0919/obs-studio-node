/******************************************************************************
    Copyright (C) 2016-2019 by Streamlabs (General Workings Inc)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

******************************************************************************/

#include "isource.hpp"
#include <error.hpp>
#include <functional>
#include <future>

#include "controller.hpp"
#include "shared.hpp"
#include "utility-v8.hpp"
#include "utility.hpp"
#include "server/osn-source.hpp"

std::vector<std::future<void>> async_cbs;

void osn::ISource::Release(const Napi::CallbackInfo& info, uint64_t id)
{
	async_cbs.push_back(std::async(std::launch::async, obs::Source::Release, id));
}

void osn::ISource::Remove(const Napi::CallbackInfo& info, uint64_t id)
{
	CacheManager<SourceDataInfo*>::getInstance().Remove(id);

	obs::Source::Remove(id);
}

Napi::Value osn::ISource::IsConfigurable(const Napi::CallbackInfo& info, uint64_t id)
{
	osn::ISource* source =
		Napi::ObjectWrap<osn::ISource>::Unwrap(info.This().ToObject());
	if (!source)
		return info.Env().Undefined();

	return Napi::Boolean::New(info.Env(), obs::Source::IsConfigurable(id));
}

Napi::Value osn::ISource::GetProperties(const Napi::CallbackInfo& info, uint64_t id)
{
	osn::ISource* source =
		Napi::ObjectWrap<osn::ISource>::Unwrap(info.This().ToObject());
	if (!source)
		return info.Env().Undefined();

	SourceDataInfo* sdi =
		CacheManager<SourceDataInfo*>::getInstance().Retrieve(id);

	if (sdi && !sdi->propertiesChanged && sdi->properties.size() > 0) {
		std::shared_ptr<property_map_t> pSomeObject = std::make_shared<property_map_t>(sdi->properties);
		auto prop_ptr = Napi::External<property_map_t>::New(info.Env(), pSomeObject.get());
		auto instance =
			osn::Properties::constructor.New({
				prop_ptr,
				Napi::Number::New(info.Env(), (uint32_t)id)
				});
		return instance;
	}

	auto res = obs::Source::GetProperties(id);
	osn::property_map_t pmap;
	for (size_t idx = 0; idx < res.size(); ++idx) {
		auto raw_property = obs::Property::deserialize(res[idx]);

		std::shared_ptr<osn::Property> pr;

		switch (raw_property->type()) {
		case obs::Property::Type::Boolean: {
			std::shared_ptr<obs::BooleanProperty> cast_property =
			    std::dynamic_pointer_cast<obs::BooleanProperty>(raw_property);
			std::shared_ptr<osn::NumberProperty> pr2 = std::make_shared<osn::NumberProperty>();
			pr2->bool_value.value                    = cast_property->value;
			pr                                       = std::static_pointer_cast<osn::Property>(pr2);
			break;
		}
		case obs::Property::Type::Integer: {
			std::shared_ptr<obs::IntegerProperty> cast_property =
			    std::dynamic_pointer_cast<obs::IntegerProperty>(raw_property);
			std::shared_ptr<osn::NumberProperty> pr2 = std::make_shared<osn::NumberProperty>();
			pr2->field_type                          = osn::NumberProperty::Type(cast_property->field_type);
			pr2->int_value.min                       = cast_property->minimum;
			pr2->int_value.max                       = cast_property->maximum;
			pr2->int_value.step                      = cast_property->step;
			pr2->int_value.value                     = cast_property->value;
			pr                                       = std::static_pointer_cast<osn::Property>(pr2);
			break;
		}
		case obs::Property::Type::Color: {
			std::shared_ptr<obs::ColorProperty> cast_property =
			    std::dynamic_pointer_cast<obs::ColorProperty>(raw_property);
			std::shared_ptr<osn::NumberProperty> pr2 = std::make_shared<osn::NumberProperty>();
			pr2->field_type                          = osn::NumberProperty::Type(cast_property->field_type);
			pr2->int_value.value                     = cast_property->value;
			pr                                       = std::static_pointer_cast<osn::Property>(pr2);
			break;
		}
		case obs::Property::Type::Float: {
			std::shared_ptr<obs::FloatProperty> cast_property =
			    std::dynamic_pointer_cast<obs::FloatProperty>(raw_property);
			std::shared_ptr<osn::NumberProperty> pr2 = std::make_shared<osn::NumberProperty>();
			pr2->field_type                          = osn::NumberProperty::Type(cast_property->field_type);
			pr2->float_value.min                     = cast_property->minimum;
			pr2->float_value.max                     = cast_property->maximum;
			pr2->float_value.step                    = cast_property->step;
			pr2->float_value.value                   = cast_property->value;
			pr                                       = std::static_pointer_cast<osn::Property>(pr2);
			break;
		}
		case obs::Property::Type::Text: {
			std::shared_ptr<obs::TextProperty> cast_property =
			    std::dynamic_pointer_cast<obs::TextProperty>(raw_property);
			std::shared_ptr<osn::TextProperty> pr2 = std::make_shared<osn::TextProperty>();
			pr2->field_type                        = osn::TextProperty::Type(cast_property->field_type);
			pr2->value                             = cast_property->value;
			pr                                     = std::static_pointer_cast<osn::Property>(pr2);
			break;
		}
		case obs::Property::Type::Path: {
			std::shared_ptr<obs::PathProperty> cast_property =
			    std::dynamic_pointer_cast<obs::PathProperty>(raw_property);
			std::shared_ptr<osn::PathProperty> pr2 = std::make_shared<osn::PathProperty>();
			pr2->field_type                        = osn::PathProperty::Type(cast_property->field_type);
			pr2->filter                            = cast_property->filter;
			pr2->default_path                      = cast_property->default_path;
			pr2->value                             = cast_property->value;
			pr                                     = std::static_pointer_cast<osn::Property>(pr2);
			break;
		}
		case obs::Property::Type::List: {
			std::shared_ptr<obs::ListProperty> cast_property =
			    std::dynamic_pointer_cast<obs::ListProperty>(raw_property);
			std::shared_ptr<osn::ListProperty> pr2 = std::make_shared<osn::ListProperty>();
			pr2->field_type                        = osn::ListProperty::Type(cast_property->field_type);
			pr2->item_format                       = osn::ListProperty::Format(cast_property->format);

			switch (cast_property->format) {
			case obs::ListProperty::Format::Integer:
				pr2->current_value_int = cast_property->current_value_int;
				break;
			case obs::ListProperty::Format::Float:
				pr2->current_value_float = cast_property->current_value_float;
				break;
			case obs::ListProperty::Format::String:
				pr2->current_value_str = cast_property->current_value_str;
				break;
			}

			for (auto& item : cast_property->items) {
				osn::ListProperty::Item item2;
				item2.name     = item.name;
				item2.disabled = !item.enabled;
				switch (cast_property->format) {
				case obs::ListProperty::Format::Integer:
					item2.value_int = item.value_int;
					break;
				case obs::ListProperty::Format::Float:
					item2.value_float = item.value_float;
					break;
				case obs::ListProperty::Format::String:
					item2.value_str = item.value_string;
					break;
				}
				pr2->items.push_back(std::move(item2));
			}
			pr = std::static_pointer_cast<osn::Property>(pr2);
			break;
		}
		case obs::Property::Type::Font: {
			std::shared_ptr<obs::FontProperty> cast_property =
			    std::dynamic_pointer_cast<obs::FontProperty>(raw_property);
			std::shared_ptr<osn::FontProperty> pr2 = std::make_shared<osn::FontProperty>();
			pr2->face                              = cast_property->face;
			pr2->style                             = cast_property->style;
			pr2->path                              = cast_property->path;
			pr2->sizeF                             = cast_property->sizeF;
			pr2->flags                             = cast_property->flags;
			pr                                     = std::static_pointer_cast<osn::Property>(pr2);
			break;
		}
		case obs::Property::Type::EditableList: {
			std::shared_ptr<obs::EditableListProperty> cast_property =
			    std::dynamic_pointer_cast<obs::EditableListProperty>(raw_property);
			std::shared_ptr<osn::EditableListProperty> pr2 = std::make_shared<osn::EditableListProperty>();
			pr2->field_type                                = osn::EditableListProperty::Type(cast_property->field_type);
			pr2->filter                                    = cast_property->filter;
			pr2->default_path                              = cast_property->default_path;

			for (auto& item : cast_property->values) {
				pr2->values.push_back(item);
			}
			pr = std::static_pointer_cast<osn::Property>(pr2);
			break;
		}
		case obs::Property::Type::FrameRate: {
			std::shared_ptr<obs::FrameRateProperty> cast_property =
			    std::dynamic_pointer_cast<obs::FrameRateProperty>(raw_property);
			std::shared_ptr<osn::ListProperty> pr2 = std::make_shared<osn::ListProperty>();
			pr2->field_type                        = osn::ListProperty::Type::LIST;
			pr2->item_format                       = osn::ListProperty::Format::STRING;

			nlohmann::json fps;
			fps["numerator"] = cast_property->current_numerator;
			fps["denominator"] = cast_property->current_denominator;
			pr2->current_value_str = fps.dump();

			for (auto& option : cast_property->ranges) {
				nlohmann::json fps;
				fps["numerator"] = option.maximum.first;
				fps["denominator"] = option.maximum.second;
				osn::ListProperty::Item item2;
				item2.name     = std::to_string(option.maximum.first / option.maximum.second);
				item2.disabled = false;
				item2.value_str = fps.dump();
				pr2->items.push_back(std::move(item2));
			}

			pr = std::static_pointer_cast<osn::Property>(pr2);
			break;
		}
		default: {
			pr = std::make_shared<osn::Property>();
			break;
		}
		}

		if (pr) {
			pr->name             = raw_property->name;
			pr->description      = raw_property->description;
			pr->long_description = raw_property->long_description;
			pr->type             = osn::Property::Type(raw_property->type());
			if (pr->type == osn::Property::Type::FRAMERATE)
				pr->type = osn::Property::Type::LIST;
			pr->enabled          = raw_property->enabled;
			pr->visible          = raw_property->visible;

			pmap.emplace(idx - 1, pr);
		}
	}

	if (sdi) {
		sdi->properties        = pmap;
		sdi->propertiesChanged = false;
	}
	std::shared_ptr<property_map_t> pSomeObject = std::make_shared<property_map_t>(pmap);
	auto prop_ptr = Napi::External<property_map_t>::New(info.Env(), pSomeObject.get());
	auto instance =
		osn::Properties::constructor.New({
			prop_ptr,
			Napi::Number::New(info.Env(), (uint32_t)id)
			});
	return instance;
}

Napi::Value osn::ISource::GetSettings(const Napi::CallbackInfo& info, uint64_t id)
{
	Napi::Object json = info.Env().Global().Get("JSON").As<Napi::Object>();
	Napi::Function parse = json.Get("parse").As<Napi::Function>();

	SourceDataInfo* sdi = CacheManager<SourceDataInfo*>::getInstance().Retrieve(id);

	if (sdi && !sdi->settingsChanged && sdi->setting.size() > 0) {
		Napi::String jsondata = Napi::String::New(info.Env(), sdi->setting);
		Napi::Object jsonObj = parse.Call(json, {jsondata}).As<Napi::Object>();
		return jsonObj;
	}

	auto res = obs::Source::GetSettings(id);
	Napi::String jsondata = Napi::String::New(info.Env(), res);
	Napi::Object jsonObj = parse.Call(json, {jsondata}).As<Napi::Object>();

	if (sdi) {
		sdi->setting         = res;
		sdi->settingsChanged = false;
	}

	return jsonObj;
}

void osn::ISource::Update(const Napi::CallbackInfo& info, uint64_t id)
{
	Napi::Object jsonObj = info[0].ToObject();
	bool shouldUpdate = true;

	Napi::Object json = info.Env().Global().Get("JSON").As<Napi::Object>();
	Napi::Function stringify = json.Get("stringify").As<Napi::Function>();

	std::string jsondata = stringify.Call(json, { jsonObj }).As<Napi::String>();

	SourceDataInfo* sdi = CacheManager<SourceDataInfo*>::getInstance().Retrieve(id);

	if (sdi && sdi->setting.size() > 0) {
		auto newSettings = nlohmann::json::parse(jsondata);
		auto settings    = nlohmann::json::parse(sdi->setting);

		nlohmann::json::iterator it = newSettings.begin();
		while (!shouldUpdate && it != newSettings.end()) {
			nlohmann::json::iterator item = settings.find(it.key());
			if (item != settings.end()) {
				if (it.value() != item.value()) {
					shouldUpdate = false;
				}
			}
			it++;
		}
	}

	if (shouldUpdate) {
		auto res = obs::Source::Update(id, jsondata);
		if (sdi) {
			sdi->setting           = res;
			sdi->settingsChanged   = false;
			sdi->propertiesChanged = true;
		}
	}
}

void osn::ISource::Load(const Napi::CallbackInfo& info, uint64_t id)
{
	obs::Source::Load(id);
}

void osn::ISource::Save(const Napi::CallbackInfo& info, uint64_t id)
{
	obs::Source::Save(id);
}

Napi::Value osn::ISource::GetType(const Napi::CallbackInfo& info, uint64_t id)
{
	return Napi::Number::New(info.Env(), obs::Source::GetType(id));
}

Napi::Value osn::ISource::GetName(const Napi::CallbackInfo& info, uint64_t id)
{
	SourceDataInfo* sdi =
		CacheManager<SourceDataInfo*>::getInstance().Retrieve(id);

	if (sdi) {
		if (sdi->name.size() > 0) {
			return Napi::String::New(info.Env(), sdi->name);
		}
	}

	auto name = obs::Source::GetName(id);
	if (sdi)
		sdi->name = name.c_str();

	return Napi::String::New(info.Env(), name);
}

void osn::ISource::SetName(const Napi::CallbackInfo& info, const Napi::Value &value, uint64_t id)
{
	std::string name = value.ToString().Utf8Value();

	obs::Source::SetName(id, name);
}

Napi::Value osn::ISource::GetOutputFlags(const Napi::CallbackInfo& info, uint64_t id)
{
	return Napi::Number::New(info.Env(), obs::Source::GetOutputFlags(id));
}

Napi::Value osn::ISource::GetFlags(const Napi::CallbackInfo& info, uint64_t id)
{
	return Napi::Number::New(info.Env(), obs::Source::GetFlags(id));
}

void osn::ISource::SetFlags(const Napi::CallbackInfo& info, const Napi::Value &value, uint64_t id)
{
	uint32_t flags = value.ToNumber().Uint32Value();

	obs::Source::SetFlags(id, flags);
}

Napi::Value osn::ISource::GetStatus(const Napi::CallbackInfo& info, uint64_t id)
{
	return Napi::Number::New(info.Env(), obs::Source::GetStatus(id));
}

Napi::Value osn::ISource::GetId(const Napi::CallbackInfo& info, uint64_t id)
{
	SourceDataInfo* sdi =
		CacheManager<SourceDataInfo*>::getInstance().Retrieve(id);

	if (sdi) {
		if (sdi->obs_sourceId.size() > 0)
			return Napi::String::New(info.Env(), sdi->obs_sourceId);
	}

	auto res = obs::Source::GetId(id);
	if (sdi) {
		sdi->obs_sourceId = res;
	}

	return Napi::String::New(info.Env(), res);
}

Napi::Value osn::ISource::GetMuted(const Napi::CallbackInfo& info, uint64_t id)
{
	SourceDataInfo* sdi =
		CacheManager<SourceDataInfo*>::getInstance().Retrieve(id);

	if (sdi) {
		if (sdi && !sdi->mutedChanged)
			return Napi::Boolean::New(info.Env(), sdi->isMuted);
	}

	auto muted = obs::Source::GetMuted(id);
	if (sdi) {
		sdi->isMuted      = muted;
		sdi->mutedChanged = false;
	}

	return Napi::Boolean::New(info.Env(), muted);
}

void osn::ISource::SetMuted(const Napi::CallbackInfo& info, const Napi::Value &value, uint64_t id)
{
	bool muted = value.ToBoolean().Value();

	obs::Source::SetMuted(id, muted);

	SourceDataInfo* sdi =
		CacheManager<SourceDataInfo*>::getInstance().Retrieve(id);
	if (sdi)
		sdi->mutedChanged = true;
}

Napi::Value osn::ISource::GetEnabled(const Napi::CallbackInfo& info, uint64_t id)
{
	return Napi::Boolean::New(info.Env(), obs::Source::GetEnabled(id));
}

void osn::ISource::SetEnabled(const Napi::CallbackInfo& info, const Napi::Value &value, uint64_t id)
{
	bool enabled = value.ToBoolean().Value();

	obs::Source::SetEnabled(id, enabled);
}

void osn::ISource::SendMouseClick(const Napi::CallbackInfo& info, uint64_t id)
{
	auto conn = GetConnection(info);
	if (!conn)
		return;

	Napi::Object mouse_event_obj = info[0].ToObject();
	uint32_t type = info[1].ToNumber().Uint32Value();
	bool mouse_up = info[2].ToBoolean().Value();
	uint32_t click_count = info[3].ToNumber().Uint32Value();

	uint32_t modifiers = mouse_event_obj.Get("modifiers").ToNumber().Uint32Value();
	uint32_t x = mouse_event_obj.Get("x").ToNumber().Uint32Value();
	uint32_t y = mouse_event_obj.Get("y").ToNumber().Uint32Value();

	obs::Source::SendMouseClick(
		id,
		modifiers,
		x,
		y,
		type,
		mouse_up,
		click_count
	);
}

void osn::ISource::SendMouseMove(const Napi::CallbackInfo& info, uint64_t id)
{
	auto conn = GetConnection(info);
	if (!conn)
		return;

	Napi::Object mouse_event_obj = info[0].ToObject();
	bool mouse_leave = info[1].ToBoolean().Value();

	uint32_t modifiers = mouse_event_obj.Get("modifiers").ToNumber().Uint32Value();
	uint32_t x = mouse_event_obj.Get("x").ToNumber().Uint32Value();
	uint32_t y = mouse_event_obj.Get("y").ToNumber().Uint32Value();

	obs::Source::SendMouseMove(
		id,
		modifiers,
		x,
		y,
		mouse_leave
	);
}

void osn::ISource::SendMouseWheel(const Napi::CallbackInfo& info, uint64_t id)
{
	auto conn = GetConnection(info);
	if (!conn)
		return;

	Napi::Object mouse_event_obj = info[0].ToObject();
	int32_t x_delta = info[1].ToNumber().Int32Value();
	int32_t y_delta = info[2].ToNumber().Int32Value();

	uint32_t modifiers = mouse_event_obj.Get("modifiers").ToNumber().Uint32Value();
	uint32_t x = mouse_event_obj.Get("x").ToNumber().Uint32Value();
	uint32_t y = mouse_event_obj.Get("y").ToNumber().Uint32Value();

	obs::Source::SendMouseWheel(
		id,
		modifiers,
		x,
		y,
		x_delta,
		y_delta
	);
}

void osn::ISource::SendFocus(const Napi::CallbackInfo& info, uint64_t id)
{
	bool focus = info[0].ToBoolean().Value();

	obs::Source::SendFocus(id, focus);
}

void osn::ISource::SendKeyClick(const Napi::CallbackInfo& info, uint64_t id)
{
	auto conn = GetConnection(info);
	if (!conn)
		return;
	Napi::Object key_event_obj = info[0].ToObject();
	bool key_up = info[1].ToBoolean().Value();

	uint32_t modifiers = key_event_obj.Get("modifiers").ToNumber().Uint32Value();
	uint32_t native_modifiers = key_event_obj.Get("nativeModifiers").ToNumber().Uint32Value();
	uint32_t native_scancode = key_event_obj.Get("nativeScancode").ToNumber().Uint32Value();
	uint32_t native_vkey = key_event_obj.Get("nativeVkey").ToNumber().Uint32Value();
	std::string text = key_event_obj.Get("text").ToString().Utf8Value();

	obs::Source::SendKeyClick(
		id,
		text,
		modifiers,
		native_modifiers,
		native_scancode,
		native_vkey,
		key_up
	);
}