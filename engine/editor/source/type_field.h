#pragma once

#include "z1engine.h"
#include "imgui/imgui.h"

using namespace z1;

void accept_payload(std::string const& data_type, std::function<void(void*)> callback);

void show_value(void* ptr, std::type_info const& type, std::string const& name, FieldInfo const& field, const EnumInfo* enum_info = nullptr);

void show_type_field(void* instance, FieldInfo const& field);

void show_type_fields(void* instance, const std::string& name, bool group = false);

void show_properties(std::shared_ptr<Entity> const& selected_entity);

void show_properties_context_menu(std::shared_ptr<Entity> const& selected_entity);
