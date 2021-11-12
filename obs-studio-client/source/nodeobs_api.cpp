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

#include "controller.hpp"
#include "error.hpp"
#include "nodeobs_api.hpp"
#include <sstream>
#include <string>
#include "shared.hpp"
#include "utility.hpp"
#include "volmeter.hpp"
#include "callback-manager.hpp"

//api::Worker* worker = nullptr;

Napi::ThreadSafeFunction js_thread;

Napi::Value api::OBS_API_initAPI(const Napi::CallbackInfo& info)
{
	std::string path;
	std::string language;
	std::string version;
	std::string crashserverurl;

	ASSERT_GET_VALUE(info, info[0], language);
	ASSERT_GET_VALUE(info, info[1], path);
	ASSERT_GET_VALUE(info, info[2], version);
	if (info.Length()>3)
		ASSERT_GET_VALUE(info, info[3], crashserverurl);

	auto conn = GetConnection(info);
	if (!conn)
		return info.Env().Undefined();

	conn->set_freez_callback(ipc_freez_callback, path);

	std::vector<ipc::value> response = conn->call_synchronous_helper(
	    "API", "OBS_API_initAPI", {ipc::value(path), ipc::value(language), ipc::value(version), ipc::value(crashserverurl)});

	// The API init method will return a response error + graphical error
	// If there is a problem with the IPC the number of responses here will be zero so we must validate the
	// response.
	// If the method call was sucessfull we will have 2 arguments, also there is no need to validate the
	// response
	if (response.size() < 2) {
		if (!ValidateResponse(info, response)) {
			return info.Env().Undefined();
		}
	}

	return Napi::Number::New(info.Env(), response[1].value_union.i32);
}

Napi::Value api::OBS_API_destroyOBS_API(const Napi::CallbackInfo& info)
{
	auto conn = GetConnection(info);
	if (!conn)
		return info.Env().Undefined();

	conn->call("API", "OBS_API_destroyOBS_API", {});

#ifdef __APPLE__
	if (js_thread)
		js_thread.Release();
#endif

	return info.Env().Undefined();
}

Napi::Value api::OBS_API_getPerformanceStatistics(const Napi::CallbackInfo& info)
{
	auto conn = GetConnection(info);
	if (!conn)
		return info.Env().Undefined();

	std::vector<ipc::value> response = conn->call_synchronous_helper("API", "OBS_API_getPerformanceStatistics", {});

	if (!ValidateResponse(info, response))
		return info.Env().Undefined();

	Napi::Object statistics = Napi::Object::New(info.Env());

	statistics.Set(
		Napi::String::New(info.Env(), "CPU"),
		Napi::Number::New(info.Env(), response[1].value_union.fp64));
	statistics.Set(
		Napi::String::New(info.Env(), "numberDroppedFrames"),
		Napi::Number::New(info.Env(), response[2].value_union.i32));
	statistics.Set(
		Napi::String::New(info.Env(), "percentageDroppedFrames"),
		Napi::Number::New(info.Env(), response[3].value_union.fp64));
	statistics.Set(
		Napi::String::New(info.Env(), "streamingBandwidth"),
		Napi::Number::New(info.Env(), response[4].value_union.fp64));
	statistics.Set(
		Napi::String::New(info.Env(), "streamingDataOutput"),
		Napi::Number::New(info.Env(), response[5].value_union.fp64));
	statistics.Set(
		Napi::String::New(info.Env(), "recordingBandwidth"),
		Napi::Number::New(info.Env(), response[6].value_union.fp64));
	statistics.Set(
		Napi::String::New(info.Env(), "recordingDataOutput"),
		Napi::Number::New(info.Env(), response[7].value_union.fp64));
	statistics.Set(
		Napi::String::New(info.Env(), "frameRate"),
		Napi::Number::New(info.Env(), response[8].value_union.fp64));
	statistics.Set(
		Napi::String::New(info.Env(), "averageTimeToRenderFrame"),
		Napi::Number::New(info.Env(), response[9].value_union.fp64));
	statistics.Set(
		Napi::String::New(info.Env(), "memoryUsage"),
		Napi::Number::New(info.Env(), response[10].value_union.fp64));

	std::string diskSpaceAvailable; // workaround for a strlen crash
	if (response.size() < 12
	 || response[11].type != ipc::type::String
	 || response[11].value_str.c_str() == nullptr 
	 || response[11].value_str.empty()) {
		diskSpaceAvailable = "0 MB";
	} else {
		diskSpaceAvailable = response[11].value_str;
	}
	statistics.Set(
		Napi::String::New(info.Env(), "diskSpaceAvailable"),
		Napi::String::New(info.Env(),diskSpaceAvailable));

	return statistics;
}

Napi::Value api::SetWorkingDirectory(const Napi::CallbackInfo& info)
{
	std::string path = info[0].ToString().Utf8Value();

	auto conn = GetConnection(info);
	if (!conn)
		return info.Env().Undefined();

	conn->call("API", "SetWorkingDirectory", {ipc::value(path)});
	return info.Env().Undefined();
}

Napi::Value api::InitShutdownSequence(const Napi::CallbackInfo& info)
{
	globalCallback::m_all_workers_stop = true;

	auto conn = GetConnection(info);
	if (!conn)
		return info.Env().Undefined();

	std::vector<ipc::value> response = conn->call_synchronous_helper("API", "StopCrashHandler", {});

	// This is a shutdown operation, no response validation needed
	// ValidateResponse(info, response);

	return info.Env().Undefined();
}

Napi::Value api::OBS_API_QueryHotkeys(const Napi::CallbackInfo& info)
{
	auto conn = GetConnection(info);
	if (!conn)
		return info.Env().Undefined();

	std::vector<ipc::value> response = conn->call_synchronous_helper("API", "OBS_API_QueryHotkeys", {});

	if (!ValidateResponse(info, response))
		return info.Env().Undefined();

	Napi::Array hotkeyInfos = Napi::Array::New(info.Env());

	// For each hotkey info that we need to fill
	for (int i = 0; i < (response.size() - 1) / 5; i++) {
		int                   responseIndex = i * 5 + 1;
		Napi::Object object     = Napi::Object::New(info.Env());
		std::string  objectName = response[responseIndex + 0].value_str;
		uint32_t     objectType = response[responseIndex + 1].value_union.ui32;
		std::string  hotkeyName = response[responseIndex + 2].value_str;
		std::string  hotkeyDesc = response[responseIndex + 3].value_str;
		uint64_t     hotkeyId   = response[responseIndex + 4].value_union.ui64;

		object.Set(
			Napi::String::New(info.Env(), "ObjectName"),
			Napi::String::New(info.Env(), objectName));

		object.Set(
			Napi::String::New(info.Env(), "ObjectType"),
			Napi::Number::New(info.Env(), objectType));

		object.Set(
			Napi::String::New(info.Env(), "HotkeyName"),
			Napi::String::New(info.Env(), hotkeyName));

		object.Set(
			Napi::String::New(info.Env(), "HotkeyDesc"),
			Napi::String::New(info.Env(), hotkeyDesc));

		object.Set(
			Napi::String::New(info.Env(), "HotkeyId"),
			Napi::Number::New(info.Env(), hotkeyId));

		hotkeyInfos.Set(i, object);
	}

	return hotkeyInfos;
}

Napi::Value api::OBS_API_ProcessHotkeyStatus(const Napi::CallbackInfo& info)
{
	uint64_t    hotkeyId;
	bool        press;

	ASSERT_GET_VALUE(info, info[0], hotkeyId);
	ASSERT_GET_VALUE(info, info[1], press);

	auto conn = GetConnection(info);
	if (!conn)
		return info.Env().Undefined();

	conn->call("API", "OBS_API_ProcessHotkeyStatus", {ipc::value(hotkeyId), ipc::value(press)});

	return info.Env().Undefined();
}

Napi::Value api::SetUsername(const Napi::CallbackInfo& info)
{
	std::string username;

	ASSERT_GET_VALUE(info, info[0], username);

	auto conn = GetConnection(info);
	if (!conn)
		return info.Env().Undefined();

	conn->call("API", "SetUsername", {ipc::value(username)});

	return info.Env().Undefined();
}

Napi::Value api::GetPermissionsStatus(const Napi::CallbackInfo& info)
{
#ifdef __APPLE__
	bool webcam, mic;
	g_util_osx->getPermissionsStatus(webcam, mic);

	Napi::Object perms = Napi::Object::New(info.Env());
	perms.Set(
		Napi::String::New(info.Env(), "webcamPermission"),
		Napi::Boolean::New(info.Env(), webcam));
	perms.Set(
		Napi::String::New(info.Env(), "micPermission"),
		Napi::Boolean::New(info.Env(), mic));

	return perms;
#endif
	return info.Env().Undefined();
}

Napi::Value api::LONGISLAND_GetPermissionsStatus(const Napi::CallbackInfo& info)
{
#ifdef __APPLE__
	int webcam, mic;
	g_util_osx->LONGISLAND_getPermissionsStatus(webcam, mic);

	Napi::Object perms = Napi::Object::New(info.Env());
	perms.Set(
		Napi::String::New(info.Env(), "webcamPermission"),
		Napi::Number::New(info.Env(), webcam));
	perms.Set(
		Napi::String::New(info.Env(), "micPermission"),
		Napi::Number::New(info.Env(), mic));

	return perms;
#endif
	return info.Env().Undefined();
}

Napi::Value api::LONGISLAND_GetScreenRecordPermissionStatus(const Napi::CallbackInfo& info)
{
#ifdef __APPLE__
	bool permission;
	g_util_osx->LONGISLAND_getScreenRecordPermissionStatus(permission);
	return Napi::Boolean::New(info.Env(), permission);
#endif
	return info.Env().Undefined();
}


Napi::Value api::RequestPermissions(const Napi::CallbackInfo& info)
{
#ifdef __APPLE__
	Napi::Function async_callback = info[0].As<Napi::Function>();
	js_thread = Napi::ThreadSafeFunction::New(
        info.Env(),
		async_callback,
		"RequestPermissionsThread",
		0,
		1,
		[]( Napi::Env ) {} );

	auto cb = [](void* data, bool webcam, bool mic) {
		Napi::ThreadSafeFunction* worker =
			reinterpret_cast<Napi::ThreadSafeFunction*>(data);

		auto callback = []( Napi::Env env, Napi::Function jsCallback, Permissions* data ) {
			Napi::Object result = Napi::Object::New(env);

			result.Set(
				Napi::String::New(env, "webcamPermission"),
				Napi::Boolean::New(env, data->webcam)
			);
			result.Set(
				Napi::String::New(env, "micPermission"),
				Napi::Boolean::New(env, data->mic)
			);

			jsCallback.Call({ result });
			delete data;
		};
		Permissions* perms_status = new Permissions();
		perms_status->webcam = webcam;
		perms_status->mic = mic;
		napi_status status = js_thread.BlockingCall( perms_status, callback );
		if (status != napi_ok) {
			delete perms_status;
		}
	};

	g_util_osx->requestPermissions(js_thread, cb);
#endif
	return info.Env().Undefined();
}

Napi::Value api::LONGISLAND_GetMallocNum(const Napi::CallbackInfo& info) {

	auto conn = GetConnection(info);
	if (!conn)
		return info.Env().Undefined();

	std::vector<ipc::value> response = conn->call_synchronous_helper("API", "LONGISLAND_GetMallocNum", {});

	if (!ValidateResponse(info, response))
		return info.Env().Undefined();

	return Napi::Number::New(info.Env(), response[1].value_union.ui32);
}

void api::Init(Napi::Env env, Napi::Object exports)
{
	exports.Set(Napi::String::New(env, "OBS_API_initAPI"), Napi::Function::New(env, api::OBS_API_initAPI));
	exports.Set(Napi::String::New(env, "OBS_API_destroyOBS_API"), Napi::Function::New(env, api::OBS_API_destroyOBS_API));
	exports.Set(Napi::String::New(env, "OBS_API_getPerformanceStatistics"), Napi::Function::New(env, api::OBS_API_getPerformanceStatistics));
	exports.Set(Napi::String::New(env, "SetWorkingDirectory"), Napi::Function::New(env, api::SetWorkingDirectory));
	exports.Set(Napi::String::New(env, "InitShutdownSequence"), Napi::Function::New(env, api::InitShutdownSequence));
	exports.Set(Napi::String::New(env, "OBS_API_QueryHotkeys"), Napi::Function::New(env, api::OBS_API_QueryHotkeys));
	exports.Set(Napi::String::New(env, "OBS_API_ProcessHotkeyStatus"), Napi::Function::New(env, api::OBS_API_ProcessHotkeyStatus));
	exports.Set(Napi::String::New(env, "SetUsername"), Napi::Function::New(env, api::SetUsername));
	exports.Set(Napi::String::New(env, "GetPermissionsStatus"), Napi::Function::New(env, api::GetPermissionsStatus));
	exports.Set(Napi::String::New(env, "RequestPermissions"), Napi::Function::New(env, api::RequestPermissions));
	exports.Set(Napi::String::New(env, "LONGISLAND_GetPermissionsStatus"), Napi::Function::New(env, api::LONGISLAND_GetPermissionsStatus));
	exports.Set(Napi::String::New(env, "LONGISLAND_GetScreenRecordPermissionStatus"), Napi::Function::New(env, api::LONGISLAND_GetScreenRecordPermissionStatus));
	exports.Set(Napi::String::New(env, "LONGISLAND_GetMallocNum"), Napi::Function::New(env, api::LONGISLAND_GetMallocNum));
}
