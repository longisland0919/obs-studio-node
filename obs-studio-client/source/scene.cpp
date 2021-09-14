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

#include "scene.hpp"
#include <condition_variable>
#include <mutex>
#include <string>
#include "controller.hpp"
#include "error.hpp"
#include "input.hpp"
#include "ipc-value.hpp"
#include "sceneitem.hpp"
#include "shared.hpp"
#include "utility.hpp"
#include "server/osn-scene.hpp"

Napi::FunctionReference osn::Scene::constructor;

Napi::Object osn::Scene::Init(Napi::Env env, Napi::Object exports) {
	Napi::HandleScope scope(env);
	Napi::Function func =
		DefineClass(env,
		"Scene",
		{
			StaticMethod("create", &osn::Scene::Create),
			StaticMethod("createPrivate", &osn::Scene::CreatePrivate),
			StaticMethod("fromName", &osn::Scene::FromName),

			InstanceAccessor("source", &osn::Scene::AsSource, nullptr),

			InstanceMethod("duplicate", &osn::Scene::Duplicate),
			InstanceMethod("add", &osn::Scene::AddSource),
			InstanceMethod("findItem", &osn::Scene::FindItem),
			InstanceMethod("moveItem", &osn::Scene::MoveItem),
			InstanceMethod("orderItems", &osn::Scene::OrderItems),
			InstanceMethod("getItemAtIdx", &osn::Scene::GetItemAtIndex),
			InstanceMethod("getItems", &osn::Scene::GetItems),
			InstanceMethod("getItemsInRange", &osn::Scene::GetItemsInRange),

			InstanceAccessor("configurable", &osn::Scene::CallIsConfigurable, nullptr),
			InstanceAccessor("properties", &osn::Scene::CallGetProperties, nullptr),
			InstanceAccessor("settings", &osn::Scene::CallGetSettings, nullptr),
			InstanceAccessor("type", &osn::Scene::CallGetType, nullptr),
			InstanceAccessor("name", &osn::Scene::CallGetName, &osn::Scene::CallSetName),
			InstanceAccessor("outputFlags", &osn::Scene::CallGetOutputFlags, nullptr),
			InstanceAccessor("flags", &osn::Scene::CallGetFlags, &osn::Scene::CallSetFlags),
			InstanceAccessor("status", &osn::Scene::CallGetStatus, nullptr),
			InstanceAccessor("id", &osn::Scene::CallGetId, nullptr),
			InstanceAccessor("muted", &osn::Scene::CallGetMuted, &osn::Scene::CallSetMuted),
			InstanceAccessor("enabled", &osn::Scene::CallGetEnabled, &osn::Scene::CallSetEnabled),

			InstanceMethod("release", &osn::Scene::CallRelease),
			InstanceMethod("remove", &osn::Scene::CallRemove),
			InstanceMethod("update", &osn::Scene::CallUpdate),
			InstanceMethod("load", &osn::Scene::CallLoad),
			InstanceMethod("save", &osn::Scene::CallSave),
			InstanceMethod("sendMouseClick", &osn::Scene::CallSendMouseClick),
			InstanceMethod("sendMouseMove", &osn::Scene::CallSendMouseMove),
			InstanceMethod("sendMouseWheel", &osn::Scene::CallSendMouseWheel),
			InstanceMethod("sendFocus", &osn::Scene::CallSendFocus),
			InstanceMethod("sendKeyClick", &osn::Scene::CallSendKeyClick),
		});
	exports.Set("Scene", func);
	osn::Scene::constructor = Napi::Persistent(func);
	osn::Scene::constructor.SuppressDestruct();
	return exports;
}

osn::Scene::Scene(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<osn::Scene>(info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    int length = info.Length();

    if (length <= 0 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
        return;
    }

	this->sourceId = (uint64_t)info[0].ToNumber().Int64Value();
}


Napi::Value osn::Scene::Create(const Napi::CallbackInfo& info)
{
	std::string name = info[0].ToString().Utf8Value();
	uint64_t sourceId = obs::Scene::Create(name);

	SceneInfo* si       = new SceneInfo();
	si->name            = name;
	si->id              = sourceId;
	SourceDataInfo* sdi = new SourceDataInfo;
	sdi->name           = name;
	sdi->obs_sourceId   = "scene";
	sdi->id             = sourceId;

	CacheManager<SourceDataInfo*>::getInstance().Store(sourceId, name, sdi);
	CacheManager<SceneInfo*>::getInstance().Store(sourceId, name, si);

    auto instance =
        osn::Scene::constructor.New({
            Napi::Number::New(info.Env(), sourceId)
            });

    return instance;
}

Napi::Value osn::Scene::CreatePrivate(const Napi::CallbackInfo& info)
{
	std::string name = info[0].ToString().Utf8Value();
	uint64_t sourceId = obs::Scene::CreatePrivate(name);

	SceneInfo* si       = new SceneInfo();
	si->name            = name;
	si->id              = sourceId;
	SourceDataInfo* sdi = new SourceDataInfo;
	sdi->name           = name;
	sdi->obs_sourceId   = "scene";
	sdi->id             = sourceId;

	CacheManager<SourceDataInfo*>::getInstance().Store(sourceId, name, sdi);
	CacheManager<SceneInfo*>::getInstance().Store(sourceId, name, si);

    auto instance =
        osn::Scene::constructor.New({
            Napi::Number::New(info.Env(), sourceId)
            });

    return instance;
}

Napi::Value osn::Scene::FromName(const Napi::CallbackInfo& info)
{
	std::string name = info[0].ToString().Utf8Value();
	SceneInfo* si = CacheManager<SceneInfo*>::getInstance().Retrieve(name);

	if (!si) {
		uint64_t uid = obs::Scene::FromName(name);
		if (uid == UINT64_MAX)
			return info.Env().Undefined();
		si = new SceneInfo;
		si->id = uid;
		si->name = name;
		CacheManager<SceneInfo*>::getInstance().Store(uid, name, si);
	}
    auto instance =
        osn::Scene::constructor.New({
            Napi::Number::New(info.Env(), si->id)
            });

    return instance;
}

Napi::Value osn::Scene::Release(const Napi::CallbackInfo& info)
{
	obs::Scene::Release(this->sourceId);

	return info.Env().Undefined();
}

Napi::Value osn::Scene::Remove(const Napi::CallbackInfo& info)
{
	obs::Scene::Remove(this->sourceId);

	return info.Env().Undefined();
}

Napi::Value osn::Scene::AsSource(const Napi::CallbackInfo& info)
{
    auto instance =
        osn::Input::constructor.New({
            Napi::Number::New(info.Env(), this->sourceId)
            });

    return instance;
}

Napi::Value osn::Scene::Duplicate(const Napi::CallbackInfo& info)
{
	std::string name = info[0].ToString().Utf8Value();
	int duplicate_type = info[1].ToNumber().Int64Value();

	uint64_t sourceId = obs::Scene::Duplicate(this->sourceId, name, duplicate_type);

	SourceDataInfo* sdi = new SourceDataInfo;
	sdi->name           = name;
	sdi->obs_sourceId   = "scene";
	sdi->id             = sourceId;

	CacheManager<SourceDataInfo*>::getInstance().Store(sourceId, name, sdi);
	CacheManager<SceneInfo*>::getInstance().Store(sourceId, name, new SceneInfo);

    auto instance =
        osn::Input::constructor.New({
            Napi::Number::New(info.Env(), sourceId)
            });

    return instance;
}

Napi::Value osn::Scene::AddSource(const Napi::CallbackInfo& info)
{
	osn::Input* input = Napi::ObjectWrap<osn::Input>::Unwrap(info[0].ToObject());

	Napi::Object transform = Napi::Object::New(info.Env());
	Napi::Object crop = Napi::Object::New(info.Env());
	obs::TransformInfo transformInfo;

	std::pair<obs_sceneitem_t *, int64_t> res;
	if (info.Length() >= 2) {
		transform = info[1].ToObject();
		transformInfo.scaleX = transform.Get("scaleX").ToNumber().DoubleValue();
		transformInfo.scaleY = transform.Get("scaleY").ToNumber().DoubleValue();
		transformInfo.visible = transform.Get("visible").ToBoolean().Value();
		transformInfo.positionX = transform.Get("x").ToNumber().DoubleValue();
		transformInfo.positionY = transform.Get("y").ToNumber().DoubleValue();
		transformInfo.rotation = transform.Get("rotation").ToNumber().DoubleValue();
		crop = transform.Get("crop").ToObject();
		transformInfo.cropLeft = crop.Get("left").ToNumber().Int64Value();
		transformInfo.cropTop = crop.Get("top").ToNumber().Int64Value();
		transformInfo.cropRight = crop.Get("right").ToNumber().Int64Value();
		transformInfo.cropBottom = crop.Get("bottom").ToNumber().Int64Value();
		transformInfo.streamVisible = transform.Get("streamVisible").ToBoolean().Value();
		transformInfo.recordingVisible = transform.Get("recordingVisible").ToBoolean().Value();

		res = obs::Scene::AddSource(this->sourceId, input->sourceId, transformInfo);
	} else {
		res = obs::Scene::AddSource(this->sourceId, input->sourceId);
	}

    auto instance =
        osn::SceneItem::constructor.New({
			Napi::External<obs_sceneitem_t*>::New(info.Env(), &res.first)
		});
    return instance;
}

Napi::Value osn::Scene::FindItem(const Napi::CallbackInfo& info)
{
	bool        hasName = false;
	std::string name;
	int64_t     position;

	if (info[0].IsNumber()) {
		hasName = false;
		position = info[0].ToNumber().Int64Value();
	} else if (info[0].IsString()) {
		hasName = true;
		name = info[0].ToString().Utf8Value();
	} else {
		Napi::TypeError::New(info.Env(), "Expected string or number").ThrowAsJavaScriptException();
		return info.Env().Undefined();
	}

	obs_sceneitem_t* item = nullptr;
	if (hasName) item = obs::Scene::FindItem(this->sourceId, name);
	else item  = obs::Scene::FindItem(this->sourceId, position);

	if (!item)
		return info.Env().Undefined();

	auto instance =
		osn::SceneItem::constructor.New({
			Napi::External<obs_sceneitem_t*>::New(info.Env(), &item)
		});

	return instance;
}

Napi::Value osn::Scene::MoveItem(const Napi::CallbackInfo& info)
{
	int from = info[0].ToNumber().Int64Value();
	int to = info[1].ToNumber().Int64Value();

	if(!obs::Scene::MoveItem(this->sourceId, from, to))
		Napi::TypeError::New(
			info.Env(),
			"Unable to move the specified item.").ThrowAsJavaScriptException();

	return info.Env().Undefined();
}

Napi::Value osn::Scene::OrderItems(const Napi::CallbackInfo& info)
{
	std::vector<int64_t> order;
	std::vector<char> order_char;

	Napi::Array array = info[0].As<Napi::Array>();

	order.resize(array.Length());
	for (unsigned int i = 0; i < array.Length(); i++ ) {
		if (array.Has(i))
			order[i] = array.Get(i).ToNumber().Int64Value();
	}
	order_char.resize(order.size()*sizeof(int64_t));
	memcpy(order_char.data(), order.data(), order_char.size());

	obs::Scene::OrderItems(this->sourceId, order_char);

	return info.Env().Undefined();
}

Napi::Value osn::Scene::GetItemAtIndex(const Napi::CallbackInfo& info)
{
	int32_t index = info[0].ToNumber().Int32Value();

	auto item = obs::Scene::GetItem(this->sourceId, (int64_t)index);

    auto instance =
        osn::SceneItem::constructor.New({
            Napi::External<obs_sceneitem_t*>::New(info.Env(), &item)
		});

    return instance;
}

Napi::Value osn::Scene::GetItems(const Napi::CallbackInfo& info)
{
	auto res = obs::Scene::GetItems(this->sourceId);
	Napi::Array array = Napi::Array::New(info.Env(), res.size());
	uint32_t index = 0;

	for (auto item: res) {
		auto instance =
			osn::SceneItem::constructor.New({
				Napi::External<obs_sceneitem_t*>::New(info.Env(), &item)
			});
		array.Set(index++, instance);
	}

	return array;
}

Napi::Value osn::Scene::GetItemsInRange(const Napi::CallbackInfo& info)
{
	int32_t from = info[0].ToNumber().Int32Value();
	int32_t to = info[1].ToNumber().Int32Value();

	auto res = obs::Scene::GetItemsInRange(this->sourceId, from, to);
	Napi::Array array = Napi::Array::New(info.Env(), res.size());
	uint32_t index = 0;

	for (auto item: res) {
		auto instance =
			osn::SceneItem::constructor.New({
				Napi::External<obs_sceneitem_t*>::New(info.Env(), &item)
			});
		array.Set(index++, instance);
	}

	return array;
}

Napi::Value osn::Scene::CallIsConfigurable(const Napi::CallbackInfo& info)
{
	return osn::ISource::IsConfigurable(info, this->sourceId);
}

Napi::Value osn::Scene::CallGetProperties(const Napi::CallbackInfo& info)
{
	return osn::ISource::GetProperties(info, this->sourceId);
}

Napi::Value osn::Scene::CallGetSettings(const Napi::CallbackInfo& info)
{
	return osn::ISource::GetSettings(info, this->sourceId);
}


Napi::Value osn::Scene::CallGetType(const Napi::CallbackInfo& info)
{
	return osn::ISource::GetType(info, this->sourceId);
}

Napi::Value osn::Scene::CallGetName(const Napi::CallbackInfo& info)
{
	return osn::ISource::GetName(info, this->sourceId);
}

void osn::Scene::CallSetName(const Napi::CallbackInfo& info, const Napi::Value &value)
{
	osn::ISource::SetName(info, value, this->sourceId);
}

Napi::Value osn::Scene::CallGetOutputFlags(const Napi::CallbackInfo& info)
{
	return osn::ISource::GetOutputFlags(info, this->sourceId);
}

Napi::Value osn::Scene::CallGetFlags(const Napi::CallbackInfo& info)
{
	return osn::ISource::GetFlags(info, this->sourceId);
}

void osn::Scene::CallSetFlags(const Napi::CallbackInfo& info, const Napi::Value &value)
{
	osn::ISource::SetFlags(info, value, this->sourceId);
}

Napi::Value osn::Scene::CallGetStatus(const Napi::CallbackInfo& info)
{
	return osn::ISource::GetStatus(info, this->sourceId);
}

Napi::Value osn::Scene::CallGetId(const Napi::CallbackInfo& info)
{
	return osn::ISource::GetId(info, this->sourceId);
}

Napi::Value osn::Scene::CallGetMuted(const Napi::CallbackInfo& info)
{
	return osn::ISource::GetMuted(info, this->sourceId);
}

void osn::Scene::CallSetMuted(const Napi::CallbackInfo& info, const Napi::Value &value)
{
	osn::ISource::SetMuted(info, value, this->sourceId);
}

Napi::Value osn::Scene::CallGetEnabled(const Napi::CallbackInfo& info)
{
	return osn::ISource::GetEnabled(info, this->sourceId);
}

void osn::Scene::CallSetEnabled(const Napi::CallbackInfo& info, const Napi::Value &value)
{
	osn::ISource::SetEnabled(info, value, this->sourceId);
}

Napi::Value osn::Scene::CallRelease(const Napi::CallbackInfo& info)
{
	osn::ISource::Release(info, this->sourceId);

	return info.Env().Undefined();
}

Napi::Value osn::Scene::CallRemove(const Napi::CallbackInfo& info)
{
	osn::ISource::Remove(info, this->sourceId);
	this->sourceId = UINT64_MAX;

	return info.Env().Undefined();
}

Napi::Value osn::Scene::CallUpdate(const Napi::CallbackInfo& info)
{
	osn::ISource::Update(info, this->sourceId);

	return info.Env().Undefined();
}

Napi::Value osn::Scene::CallLoad(const Napi::CallbackInfo& info)
{
	osn::ISource::Load(info, this->sourceId);

	return info.Env().Undefined();
}

Napi::Value osn::Scene::CallSave(const Napi::CallbackInfo& info)
{
	osn::ISource::Save(info, this->sourceId);

	return info.Env().Undefined();
}

Napi::Value osn::Scene::CallSendMouseClick(const Napi::CallbackInfo& info)
{
	osn::ISource::SendMouseClick(info, this->sourceId);

	return info.Env().Undefined();
}

Napi::Value osn::Scene::CallSendMouseMove(const Napi::CallbackInfo& info)
{
	osn::ISource::SendMouseMove(info, this->sourceId);

	return info.Env().Undefined();
}

Napi::Value osn::Scene::CallSendMouseWheel(const Napi::CallbackInfo& info)
{
	osn::ISource::SendMouseWheel(info, this->sourceId);

	return info.Env().Undefined();
}

Napi::Value osn::Scene::CallSendFocus(const Napi::CallbackInfo& info)
{
	osn::ISource::SendFocus(info, this->sourceId);

	return info.Env().Undefined();
}

Napi::Value osn::Scene::CallSendKeyClick(const Napi::CallbackInfo& info)
{
	osn::ISource::SendKeyClick(info, this->sourceId);

	return info.Env().Undefined();
}